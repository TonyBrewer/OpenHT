/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "Htv.h"
#include "HtDesign.h"

namespace ht {

	class HtMirrorDesign
	{
	public:
		HtMirrorDesign(HtDesign * pHtDesign, ASTContext &Context) 
			: m_pHtDesign(pHtDesign), m_context(Context), m_diags(Context.getDiagnostics())
		{
		}

		void mirrorDesign();

		HtDecl * visitDecl(Decl const * pDecl);
		HtStmt * visitStmt(Stmt const * pStmt, bool allowReturnStmt=false );
		HtExpr * visitExpr(Stmt const * pStmt, bool isRhs);
		HtType * visitType(Type const * pType);
		HtQualType visitQualType(QualType & qualType);
		HtTemplateArgument * visitTemplateArg( TemplateArgument const * pArg );
		HtBaseSpecifier * visitBaseSpecifier( CXXBaseSpecifier const * pBase );
		void initializeBuiltinType(HtType * pHtType);
		int FindDeclSize(HtQualType qualType, vector<int> & dimenList, int & typeBitSize, int & alignBitSize);
		void PreFindRecordPosAndWidth(HtDecl * pHtDecl, int & savedRecordBitPos,
			bool & bSavedRecordPackedAttr, bool & bSavedRecordIsUnion, int & savedRecordAlignBitSize);
		void PostFindRecordPosAndWidth(HtDecl * pHtDecl, int savedRecordBitPos,
			bool bSavedRecordPackedAttr, bool bSavedRecordIsUnion, int savedRecordAlignBitSize);

	private:
		HtDesign * m_pHtDesign;
		ASTContext & m_context;
		DiagnosticsEngine & m_diags;

		int m_recordBitPos;
		bool m_bRecordPackedAttr;
		bool m_bRecordIsUnion;
		int m_recordPrevTypeBitSize;
		int m_recordAlignBitSize;

		HtDeclMap m_declMap;
		HtTypeMap m_typeMap;
	};

	void HtDesign::mirrorDesign(ASTContext &Context)
	{
		HtMirrorDesign transform( this, Context );

		transform.mirrorDesign();

		ExitIfError(Context);
	}

	void HtMirrorDesign::mirrorDesign()
	{
		HtLineInfo::setSourceManager( &m_context.getSourceManager() );

		TranslationUnitDecl * pTranslationUnitDecl = m_context.getTranslationUnitDecl();

		HtAssert( pTranslationUnitDecl->getKind() == Decl::TranslationUnit );

		DeclContext * pDeclContext = cast<DeclContext>(pTranslationUnitDecl);

		m_pHtDesign->setTopDeclList( new HtDeclList_t );

		int skipCnt = 0;
		for (DeclContext::decl_iterator declIter = pDeclContext->decls_begin();
			declIter != pDeclContext->decls_end();
			declIter++)
		{
			if (skipCnt++ >= 4)
				m_pHtDesign->addDecl( visitDecl(*declIter) );
		}
	}

	HtDecl * HtMirrorDesign::visitDecl(Decl const * pDecl)
	{
		if (pDecl == 0)
			return 0;

		HtDecl * pHtDecl = m_declMap.findDecl( pDecl );
		if (pHtDecl)
			return pHtDecl;

		pHtDecl = new HtDecl(pDecl);
		m_declMap.addDecl( pDecl, pHtDecl );

		Decl::Kind declKind = pDecl->getKind();
		switch (declKind) {
		case Decl::TranslationUnit:
			break;
		case Decl::Typedef:
			{
				TypedefDecl const * pTypedefDecl = cast<TypedefDecl>(pDecl);
				pHtDecl->setName( pTypedefDecl->getNameAsString() );

				pHtDecl->setQualType( visitQualType( pTypedefDecl->getUnderlyingType() ));
			}
			break;
		case Decl::CXXRecord:
			{
				CXXRecordDecl const * pCXXRecordDecl = cast<CXXRecordDecl>(pDecl);
				pHtDecl->setName( pCXXRecordDecl->getNameAsString() );

				//pDecl->dump();

				pHtDecl->isCompleteDefinition( pCXXRecordDecl->isCompleteDefinition() );
				pHtDecl->setParentDecl( visitDecl( cast<Decl>(pCXXRecordDecl->getParent()) ));
				pHtDecl->setTagKind( pCXXRecordDecl->getTagKind() );

				// handle base classes
				if (pCXXRecordDecl->isCompleteDefinition()) {
					for (CXXRecordDecl::base_class_const_iterator I = pCXXRecordDecl->bases_begin(),
						E = pCXXRecordDecl->bases_end();
						I != E; ++I)
					{
						HtBaseSpecifier * pBaseSpec = visitBaseSpecifier( &(*I) );
						pHtDecl->addBaseSpecifier( pBaseSpec );
					}
				}

				Type const * pType = pCXXRecordDecl->getTypeForDecl();

				HtType * pHtType = visitType( pType );
				pHtDecl->setType( pHtType );
				if (pHtType->getHtDecl() == 0)
					pHtType->setHtDecl( pHtDecl );

				for (Decl::attr_iterator AI = pCXXRecordDecl->attr_begin(); AI != pCXXRecordDecl->attr_end(); ++AI) {
					Attr *pAttr = *AI;
					switch (pAttr->getKind()) {
					case attr::Packed:
						pHtType->setPackedAttr();
						break;
					default:
						unsigned DiagID = m_diags.getCustomDiagID(DiagnosticsEngine::Error,
							"htt/htv unsupported attribute");
						m_diags.Report(pHtDecl->getLineInfo().getLoc(), DiagID);
						break;
					}
				}

				int savedRecordBitPos;
				bool bSavedRecordPackedAttr;
				bool bSavedRecordIsUnion;
				int savedRecordAlignBitSize;

				PreFindRecordPosAndWidth(pHtDecl, savedRecordBitPos, bSavedRecordPackedAttr,
					bSavedRecordIsUnion, savedRecordAlignBitSize);

				DeclContext const * DC = cast<DeclContext>(pCXXRecordDecl);
				for (DeclContext::decl_iterator DI = DC->decls_begin(); DI != DC->decls_end(); DI++)
					pHtDecl->addSubDecl( visitDecl( *DI) );

				PostFindRecordPosAndWidth(pHtDecl, savedRecordBitPos, bSavedRecordPackedAttr,
					bSavedRecordIsUnion, savedRecordAlignBitSize);
			}
			break;
		case Decl::Field:
			{
				FieldDecl const * pFieldDecl = cast<FieldDecl>(pDecl);
				pHtDecl->setName( pFieldDecl->getNameAsString() );

				//if (pHtDecl->getName() == "aa2")
				//	pDecl->dump();

				pHtDecl->setQualType( visitQualType( pFieldDecl->getType() ));

				SplitQualType splitDesugaredQualType = pFieldDecl->getType().getSplitDesugaredType();
				pHtDecl->setDesugaredQualType( visitQualType( QualType( splitDesugaredQualType.Ty, 0) ));

				vector<int> dimenList;
				int typeBitSize, alignBitSize;
				int elemCnt = FindDeclSize(pHtDecl->getDesugaredQualType(), dimenList, typeBitSize, alignBitSize);

				if (!m_bRecordPackedAttr)
					m_recordAlignBitSize = max(m_recordAlignBitSize, alignBitSize);

				int fieldBitWidth = 0;
				bool bFieldWidth = false;
				if (pFieldDecl->isBitField()) {
					HtExpr * pExpr = visitExpr( pFieldDecl->getBitWidth(), true );
					if (pExpr->isConstExpr())
						fieldBitWidth = (int)pExpr->getEvalResult().Val.getInt().getZExtValue();
					pHtDecl->setFieldBitWidth( fieldBitWidth );
					bFieldWidth = true;
				} else
					fieldBitWidth = typeBitSize;

				int totalBitSize = fieldBitWidth * elemCnt;

				pHtDecl->setDimenList(dimenList);
				pHtDecl->setBitSize(fieldBitWidth);

				if (m_bRecordIsUnion) {
					pHtDecl->setBitPos(0);
					m_recordBitPos = max(m_recordBitPos, totalBitSize);
				} else {
					// first allign to appropriate boundary
					if (!m_bRecordPackedAttr) {
						if (bFieldWidth) {
							// see if field would cross typeBitSize boundary
							int remainingBits = typeBitSize - m_recordBitPos % typeBitSize;
							remainingBits %= typeBitSize;

							if (m_recordPrevTypeBitSize != typeBitSize || remainingBits < fieldBitWidth)
								m_recordBitPos += remainingBits;
						} else {
							m_recordBitPos += alignBitSize - 1;
							m_recordBitPos &= ~(alignBitSize - 1);
						}
					}

					pHtDecl->setBitPos(m_recordBitPos);

					m_recordBitPos += totalBitSize;
					if (bFieldWidth)
						m_recordPrevTypeBitSize = typeBitSize;
					else
						m_recordPrevTypeBitSize = -1;
				}

				if (g_htvArgs.isDumpPosAndWidth()) {
					printf("%s", pHtDecl->getName().c_str());
					for (size_t i = 0; i < dimenList.size(); i += 1)
						printf("[%d]", (int)dimenList[i]);
					printf(" pos=%d, fieldSize=%d, totalSize=%d\n",
						pHtDecl->getBitPos(), pHtDecl->getBitSize(), totalBitSize);
				}
			}
			break;
		case Decl::IndirectField:
			{
				IndirectFieldDecl const * pFieldDecl = cast<IndirectFieldDecl>(pDecl);
				pHtDecl->setName( pFieldDecl->getNameAsString() );
			}
			break;
		case Decl::Var:
			{
				VarDecl const * pVarDecl = cast<VarDecl>(pDecl);
				pHtDecl->setName( pVarDecl->getNameAsString() );

				if (pHtDecl->getName() == "y")
					bool stop = true;

				pHtDecl->setQualType( visitQualType( pVarDecl->getType() ));

				pHtDecl->setInitExpr( visitExpr( pVarDecl->getInit(), true ));
			}
			break;
		case Decl::ParmVar:
			{
				ParmVarDecl const * pParmVarDecl = cast<ParmVarDecl>(pDecl);
				pHtDecl->setName( pParmVarDecl->getNameAsString() );

				pHtDecl->setQualType( visitQualType( pParmVarDecl->getType() ));

				SplitQualType splitDesugaredQualType = pParmVarDecl->getType().getSplitDesugaredType();
				pHtDecl->setDesugaredQualType( visitQualType( QualType( splitDesugaredQualType.Ty, 0) ));
			}
			break;
		case Decl::CXXConstructor:
			{
				CXXConstructorDecl const * pCXXCtorDecl = cast<CXXConstructorDecl>(pDecl);
				pHtDecl->setName( pCXXCtorDecl->getNameAsString() );

				pHtDecl->setPreviousDecl( visitDecl( pCXXCtorDecl->getPreviousDecl() ));
				pHtDecl->setParentDecl( visitDecl( pCXXCtorDecl->getParent() ));

				// handle param declarations
				uint32_t numParams = pCXXCtorDecl->getNumParams();
				for (uint32_t i = 0; i < numParams; i += 1)
					pHtDecl->addParamDecl( visitDecl( pCXXCtorDecl->getParamDecl(i) ));

				// handle body
				pHtDecl->doesThisDeclarationHaveABody( pCXXCtorDecl->doesThisDeclarationHaveABody() );
				if (pCXXCtorDecl->doesThisDeclarationHaveABody())
					pHtDecl->setBody( visitStmt( pCXXCtorDecl->getBody() ) );

				// handle Ctor initializers
				for (CXXConstructorDecl::init_const_iterator initIter = pCXXCtorDecl->init_begin();
					initIter != pCXXCtorDecl->init_end(); initIter++ )
				{
					const CXXCtorInitializer * pInit = *initIter;

					if (pInit->isAnyMemberInitializer()) {

						HtDecl * pMemberDecl = visitDecl( pInit->getAnyMember() );

						HtExpr * pThisExpr = new HtExpr( Stmt::CXXThisExprClass, pMemberDecl->getQualType(), HtLineInfo(pInit->getSourceRange()) );

						HtExpr * pMemberExpr = new HtExpr( Stmt::MemberExprClass, 
							pMemberDecl->getQualType(), HtLineInfo(pInit->getSourceRange()) );
						pMemberExpr->setMemberDecl( pMemberDecl );
						pMemberExpr->setBase( pThisExpr );

						HtExpr * pAssignExpr = new HtExpr( Stmt::BinaryOperatorClass, pMemberDecl->getQualType(), HtLineInfo(pInit->getSourceRange()) );
						pAssignExpr->isAssignStmt(true);
						pAssignExpr->isCtorMemberInit(true);
						pAssignExpr->setBinaryOperator( BO_Assign );
						pAssignExpr->setLhs( pMemberExpr );
						pAssignExpr->setRhs( visitExpr( pInit->getInit(), true ));

						assert (pHtDecl->hasBody());
						assert (pHtDecl->getBody().asStmt()->getStmtClass() == Stmt::CompoundStmtClass);
						pHtDecl->getBody().asStmt()->addStmt( pAssignExpr );
					}
				}
			}
			break;
		case Decl::CXXDestructor:
			{
				CXXDestructorDecl const * pCXXDtorDecl = cast<CXXDestructorDecl>(pDecl);
				pHtDecl->setName( pCXXDtorDecl->getNameAsString() );

				pHtDecl->setParentDecl( visitDecl( pCXXDtorDecl->getParent() ));
				pHtDecl->setPreviousDecl( visitDecl( pCXXDtorDecl->getPreviousDecl() ));

				// handle param declarations
				uint32_t numParams = pCXXDtorDecl->getNumParams();
				for (uint32_t i = 0; i < numParams; i += 1)
					pHtDecl->addParamDecl( visitDecl( pCXXDtorDecl->getParamDecl(i) ));

				// handle body
				pHtDecl->doesThisDeclarationHaveABody( pCXXDtorDecl->doesThisDeclarationHaveABody() );
				if (pCXXDtorDecl->doesThisDeclarationHaveABody())
					pHtDecl->setBody( visitStmt( pCXXDtorDecl->getBody() ) );
			}
			break;
		case Decl::CXXMethod:
			{
				CXXMethodDecl const * pCXXMethodDecl = cast<CXXMethodDecl>(pDecl);
				pHtDecl->setName( pCXXMethodDecl->getNameAsString() );

				pHtDecl->setGeneralTemplateDecl( visitDecl( pCXXMethodDecl->getInstantiatedFromMemberFunction() ));

				pHtDecl->setParentDecl( visitDecl( pCXXMethodDecl->getParent() ));
				pHtDecl->setPreviousDecl( visitDecl( pCXXMethodDecl->getPreviousDecl() ));

				pHtDecl->isDeclaredInMethod( pCXXMethodDecl->getDeclContext() == pCXXMethodDecl->getLexicalDeclContext() );

				pHtDecl->setQualType( visitQualType( pCXXMethodDecl->getType() ));

				// handle param declarations
				uint32_t numParams = pCXXMethodDecl->getNumParams();
				for (uint32_t i = 0; i < numParams; i += 1) {
					ParmVarDecl const * pParmVarDecl = pCXXMethodDecl->getParamDecl(i);
					HtDecl * pHtParamDecl = visitDecl( pParmVarDecl );
					pHtDecl->addParamDecl( pHtParamDecl );

					if (pHtDecl->getPreviousDecl())
						pHtParamDecl->setPreviousDecl( pHtDecl->getPreviousDecl()->getParamDecl(i) );
				}

				// handle body
				pHtDecl->doesThisDeclarationHaveABody( pCXXMethodDecl->doesThisDeclarationHaveABody() );
				if (pCXXMethodDecl->doesThisDeclarationHaveABody()) {
					bool allowReturnStmt = true;
					pHtDecl->setBody( visitStmt( pCXXMethodDecl->getBody(), allowReturnStmt ));
				}
			}
			break;
		case Decl::CXXConversion:
			{
				CXXConversionDecl const * pCXXConversionDecl = cast<CXXConversionDecl>(pDecl);

				pHtDecl->setParentDecl( visitDecl( pCXXConversionDecl->getParent() ));
				pHtDecl->setPreviousDecl( visitDecl( pCXXConversionDecl->getPreviousDecl() ));

				// handle return type
				pHtDecl->setQualType( visitQualType( pCXXConversionDecl->getConversionType() ));

				switch (pHtDecl->getType()->getTypeClass()) {
				case Type::Builtin:
				case Type::Typedef:
					pHtDecl->setName( pHtDecl->getType()->getHtDecl()->getName() );
					break;
				case Type::TemplateTypeParm:
					pHtDecl->setName( pHtDecl->getType()->getTemplateTypeParmName() );
					break;
				case Type::SubstTemplateTypeParm:
					pHtDecl->setName( pHtDecl->getType()->getQualType().getType()->getHtDecl()->getName() );
					break;
				default:
					assert(0);
				}

				// handle param declarations
				uint32_t numParams = pCXXConversionDecl->getNumParams();
				for (uint32_t i = 0; i < numParams; i += 1)
					pHtDecl->addParamDecl( visitDecl( pCXXConversionDecl->getParamDecl(i) ));

				// handle body
				// body will be missing for template even when doesThisDeclarationHaveABody=true
				pHtDecl->doesThisDeclarationHaveABody( pCXXConversionDecl->doesThisDeclarationHaveABody() );

				bool allowReturnStmt = true;
				pHtDecl->setBody( visitStmt( pCXXConversionDecl->getBody(), allowReturnStmt ));
			}
			break;
		case Decl::Function:
			{
				FunctionDecl const * pFuncDecl = cast<FunctionDecl>(pDecl);
				pHtDecl->setName( pFuncDecl->getNameAsString() );

				// handle return type
				pHtDecl->setQualType( visitQualType( pFuncDecl->getType() ));

				// handle param declarations
				uint32_t numParams = pFuncDecl->getNumParams();
				for (uint32_t i = 0; i < numParams; i += 1)
					pHtDecl->addParamDecl( visitDecl( pFuncDecl->getParamDecl(i) ));

				// handle body
				pHtDecl->doesThisDeclarationHaveABody( pFuncDecl->doesThisDeclarationHaveABody() );
				if (pFuncDecl->doesThisDeclarationHaveABody()) {
					bool allowReturnStmt = true;
					pHtDecl->setBody( visitStmt( pFuncDecl->getBody(), allowReturnStmt ));
				}
			}
			break;
		case Decl::ClassTemplate:
			{
				ClassTemplateDecl const * pClassTemplateDecl = cast<ClassTemplateDecl>(pDecl);
				pHtDecl->setName( pClassTemplateDecl->getNameAsString() );

				TemplateParameterList * pParamList = pClassTemplateDecl->getTemplateParameters();
				for (TemplateParameterList::const_iterator paramIter = pParamList->begin();
					paramIter != pParamList->end(); ++paramIter)
				{
					pHtDecl->addParamDecl( visitDecl(*paramIter) );
				}

				pHtDecl->setTemplateDecl( visitDecl( pClassTemplateDecl->getTemplatedDecl() ));

				for (ClassTemplateDecl::spec_iterator specIter = pClassTemplateDecl->spec_begin();
					specIter != pClassTemplateDecl->spec_end(); specIter++)
				{
					pHtDecl->addSpecializationDecl( visitDecl( *specIter ) );
				}
			}
			break;
		case Decl::ClassTemplateSpecialization:
			{
				ClassTemplateSpecializationDecl const * pClassSpecializationDecl = cast<ClassTemplateSpecializationDecl>(pDecl);
				pHtDecl->setName( pClassSpecializationDecl->getNameAsString() );

				pHtDecl->setPreviousDecl( visitDecl( pClassSpecializationDecl->getPreviousDecl() ));
				//pHtDecl->setParentDecl( visitDecl( pClassSpecializationDecl->getSpecializedTemplate() ));

				TemplateArgumentList const * pArgList = &pClassSpecializationDecl->getTemplateArgs();
				for (uint32_t i = 0; i < pArgList->size(); i += 1)
				{
					pHtDecl->addTemplateArg( visitTemplateArg( &(*pArgList)[i] ));
				}

				Type const * pType = pClassSpecializationDecl->getTypeForDecl();

				HtType * pHtType = visitType( pType );
				pHtDecl->setType( pHtType );
				if (pHtType->getHtDecl() == 0)
					pHtType->setHtDecl( pHtDecl );

				for (Decl::attr_iterator AI = pClassSpecializationDecl->attr_begin(); 
					AI != pClassSpecializationDecl->attr_end(); ++AI)
				{
					Attr *pAttr = *AI;
					switch (pAttr->getKind()) {
					case attr::Packed:
						pHtType->setPackedAttr();
						break;
					default:
						unsigned DiagID = m_diags.getCustomDiagID(DiagnosticsEngine::Error,
							"htt/htv unsupported attribute");
						m_diags.Report(pHtDecl->getLineInfo().getLoc(), DiagID);
						break;
					}
				}

				int savedRecordBitPos;
				bool bSavedRecordPackedAttr;
				bool bSavedRecordIsUnion;
				int savedRecordAlignBitSize;

				PreFindRecordPosAndWidth(pHtDecl, savedRecordBitPos, bSavedRecordPackedAttr,
					bSavedRecordIsUnion, savedRecordAlignBitSize);

				DeclContext const * DC = cast<DeclContext>(pClassSpecializationDecl);
				for (DeclContext::decl_iterator DI = DC->noload_decls_begin(); DI != DC->noload_decls_end(); DI++)
					pHtDecl->addSubDecl( visitDecl( *DI) );

				PostFindRecordPosAndWidth(pHtDecl, savedRecordBitPos, bSavedRecordPackedAttr,
					bSavedRecordIsUnion, savedRecordAlignBitSize);
			}
			break;
		case Decl::NonTypeTemplateParm:
			{
				NonTypeTemplateParmDecl const * pParamDecl = cast<NonTypeTemplateParmDecl>(pDecl);
				pHtDecl->setName( pParamDecl->getNameAsString() );

				pHtDecl->setQualType( visitQualType( pParamDecl->getType() ));
				pHtDecl->setDefaultArgument( visitExpr( pParamDecl->getDefaultArgument(), true ));
			}
			break;
		case Decl::TemplateTypeParm:
			{
				TemplateTypeParmDecl const * pParamDecl = cast<TemplateTypeParmDecl>(pDecl);
				pHtDecl->setName( pParamDecl->getNameAsString() );

				pHtDecl->wasDeclaredWithTypename( pParamDecl->wasDeclaredWithTypename() );

				pHtDecl->hasDefaultArgument( pParamDecl->hasDefaultArgument() );
				if (pParamDecl->hasDefaultArgument())
					pHtDecl->setDefaultArgQualType( visitQualType( pParamDecl->getDefaultArgument() ));
			}
			break;
		case Decl::AccessSpec:
			{
				AccessSpecDecl const * pAccessSpecDecl = cast<AccessSpecDecl>(pDecl);
				pHtDecl->setAccess( pAccessSpecDecl->getAccess() );
				break;
			}
			break;
		case Decl::Enum:
			{
				EnumDecl const * pEnumDecl = cast<EnumDecl>(pDecl);
				pHtDecl->setName( pEnumDecl->getNameAsString() );

				pHtDecl->setParentDecl( visitDecl( cast<Decl>(pEnumDecl->getParent()) ));

				Type const * pType = pEnumDecl->getTypeForDecl();

				HtType * pHtType = visitType( pType );
				pHtDecl->setType( pHtType );
				if (pHtType->getHtDecl() == 0)
					pHtType->setHtDecl( pHtDecl );

				DeclContext const * DC = cast<DeclContext>(pEnumDecl);
				for (DeclContext::decl_iterator DI = DC->noload_decls_begin(); DI != DC->noload_decls_end(); DI++)
					pHtDecl->addSubDecl( visitDecl( *DI) );
			}
			break;
		case Decl::EnumConstant:
			{
				EnumConstantDecl const * pEnumConstDecl = cast<EnumConstantDecl>(pDecl);
				pHtDecl->setName( pEnumConstDecl->getNameAsString() );

				pHtDecl->setQualType( visitQualType( pEnumConstDecl->getType() ));

				pHtDecl->setInitExpr( visitExpr( pEnumConstDecl->getInitExpr(), true ));
			}
			break;
		case Decl::FunctionTemplate:
			{
				FunctionTemplateDecl const * pFunctionTemplateDecl = cast<FunctionTemplateDecl>(pDecl);
				pHtDecl->setName( pFunctionTemplateDecl->getNameAsString() );

				TemplateParameterList * pParamList = pFunctionTemplateDecl->getTemplateParameters();
				for (TemplateParameterList::const_iterator paramIter = pParamList->begin();
					paramIter != pParamList->end(); ++paramIter)
				{
					pHtDecl->addParamDecl( visitDecl(*paramIter) );
				}

				pHtDecl->setTemplateDecl( visitDecl( pFunctionTemplateDecl->getTemplatedDecl() ));

				for (FunctionTemplateDecl::spec_iterator specIter = pFunctionTemplateDecl->spec_begin();
					specIter != pFunctionTemplateDecl->spec_end(); specIter++)
				{
					HtDecl * pSpecializationDecl = visitDecl( *specIter );
					pSpecializationDecl->setGeneralTemplateDecl( pHtDecl );
					pHtDecl->addSpecializationDecl( pSpecializationDecl );
				}
			}
			break;
		case Decl::PragmaHtv:
			{
				PragmaHtvDecl const * pPragmaHtvDecl = cast<PragmaHtvDecl>(pDecl);

				pHtDecl->setPragmaTokens(pPragmaHtvDecl->getTokens());
			}
			break;
		default:
			assert(0);
		}

		return pHtDecl;
	}

	HtBaseSpecifier * HtMirrorDesign::visitBaseSpecifier( CXXBaseSpecifier const * pBaseSpec )
	{
		HtBaseSpecifier * pHtBaseSpec = new HtBaseSpecifier(pBaseSpec);

		pHtBaseSpec->isVirtual( pBaseSpec->isVirtual() );
		pHtBaseSpec->setAccessSpec( pBaseSpec->getAccessSpecifier() );
		
		pHtBaseSpec->setQualType( visitQualType( pBaseSpec->getType() ));

		return pHtBaseSpec;
	}

	HtStmt * HtMirrorDesign::visitStmt(Stmt const * pStmt, bool allowReturnStmt)
	{
		if (pStmt == 0)
			return 0;

		HtStmt * pHtStmt = new HtStmt(pStmt);

		Stmt::StmtClass stmtKind = pStmt->getStmtClass();
		switch (stmtKind) {
		case Stmt::NoStmtClass:
		case Stmt::NullStmtClass:
			break;
		case Stmt::CompoundStmtClass: 
			{
				CompoundStmt const * pCS = cast<CompoundStmt>(pStmt);

				for (CompoundStmt::const_body_iterator BI = pCS->body_begin(); BI != pCS->body_end(); BI++ ) {

					// allow a return if this is the last statment of the list and statement is a return and compound stmt
					bool allowReturnStmt2 = allowReturnStmt && BI+1 == pCS->body_end() 
						&& ((*BI)->getStmtClass() == Stmt::CompoundStmtClass || (*BI)->getStmtClass() == Stmt::ReturnStmtClass);

					pHtStmt->addStmt( visitStmt( *BI, allowReturnStmt2 ));
				}
			}
			break;
		case Stmt::DeclStmtClass:
			{
				DeclStmt const * pDS = cast<DeclStmt>(pStmt);
				DeclGroupRef DGR = pDS->getDeclGroup();
				for (DeclGroupRef::const_iterator DI = DGR.begin(); DI != DGR.end(); DI++)
					pHtStmt->addDecl( visitDecl(*DI) );
				break;
			}
		case Stmt::IfStmtClass:
			{
				IfStmt const * pIfStmt = cast<IfStmt>(pStmt);
				pHtStmt->setCond( visitExpr( pIfStmt->getCond(), true ));
				pHtStmt->setThen( visitStmt( pIfStmt->getThen() ));
				pHtStmt->setElse( visitStmt( pIfStmt->getElse() ));
			}
			break;
		case Stmt::SwitchStmtClass:
			{
				SwitchStmt const * pSwitchStmt = cast<SwitchStmt>(pStmt);
				pHtStmt->setCond( visitExpr( pSwitchStmt->getCond(), true ));
				pHtStmt->setBody( visitStmt( pSwitchStmt->getBody() ));
			}
			break;
		case Stmt::CaseStmtClass:
			{
				CaseStmt const * pCaseStmt = cast<CaseStmt>(pStmt);
				pHtStmt->setLhs( visitExpr( pCaseStmt->getLHS(), true ));
				pHtStmt->setSubStmt( visitStmt( pCaseStmt->getSubStmt() ));
			}
			break;
		case Stmt::DefaultStmtClass:
			{
				DefaultStmt const * pDefaultStmt = cast<DefaultStmt>(pStmt);
				pHtStmt->setSubStmt( visitStmt( pDefaultStmt->getSubStmt() ));
			}
			break;
		case Stmt::BreakStmtClass:
			break;
		case Stmt::ReturnStmtClass:
			{
				ReturnStmt const * pReturnStmt = cast<ReturnStmt>(pStmt);
				pHtStmt->setSubExpr( visitExpr(pReturnStmt->getRetValue(), true) );

				if (!allowReturnStmt) {
					unsigned DiagID = m_diags.getCustomDiagID(DiagnosticsEngine::Error,
						"htt/htv only support return as last statement of function");
					m_diags.Report(pHtStmt->getLineInfo().getLoc(), DiagID);
				}
				break;
			}
		case Stmt::CXXOperatorCallExprClass:
		case Stmt::CXXMemberCallExprClass:
		case Stmt::ImplicitCastExprClass:
		case Stmt::MemberExprClass:
		case Stmt::CXXThisExprClass:
		case Stmt::CXXBoolLiteralExprClass:
		case Stmt::ParenExprClass:
		case Stmt::CallExprClass:
		case Stmt::UnaryOperatorClass:
		case Stmt::BinaryOperatorClass:
		case Stmt::CompoundAssignOperatorClass:
		case Stmt::IntegerLiteralClass:
		case Stmt::DeclRefExprClass:
			{
				delete pHtStmt;
				pHtStmt = visitExpr(pStmt, true);
				pHtStmt->isAssignStmt(true);
			}
			break;
		case Stmt::ForStmtClass:
			{
				ForStmt const * pForStmt = cast<ForStmt>(pStmt);
				pHtStmt->setInit( visitStmt( pForStmt->getInit() ));
				pHtStmt->setCond( visitStmt( pForStmt->getCond() ));
				pHtStmt->setInc( visitStmt( pForStmt->getInc() ));
				pHtStmt->setBody( visitStmt( pForStmt->getBody() ));
			}
			break;
		default:
			assert(0);
		}

		return pHtStmt;
	}

	HtExpr * HtMirrorDesign::visitExpr(Stmt const * pStmt, bool isRhs)
	{
		if (pStmt == 0)
			return 0;

		Stmt::StmtClass stmtKind = pStmt->getStmtClass();

		Expr const * pExpr = cast<Expr>(pStmt);
		HtExpr * pHtExpr = new HtExpr(pExpr, visitQualType( pExpr->getType() ));

		if (isRhs 
			&& stmtKind != Stmt::CompoundAssignOperatorClass
			&& (stmtKind != Stmt::BinaryOperatorClass || cast<BinaryOperator>(pStmt)->getOpcode() != BO_Assign)
			&& stmtKind != Stmt::ArraySubscriptExprClass)
		{
			pHtExpr->isConstExpr( pExpr->EvaluateAsRValue( pHtExpr->getEvalResult(), m_context ));
		}

		switch (stmtKind) {
		case Stmt::CompoundAssignOperatorClass:
			{
				CompoundAssignOperator const * pExpr = cast<CompoundAssignOperator>(pStmt);
				pHtExpr->setLhs( visitExpr(pExpr->getLHS(), false) );
				pHtExpr->setRhs( visitExpr(pExpr->getRHS(), true) );
				pHtExpr->setCompoundAssignOperator(pExpr->getOpcode());
			}
			break;
		case Stmt::UnaryOperatorClass:
			{
				UnaryOperator const * pExpr = cast<UnaryOperator>(pStmt);
				pHtExpr->setUnaryOperator(pExpr->getOpcode());
				pHtExpr->setSubExpr( visitExpr(pExpr->getSubExpr(), isRhs) );
			}
			break;
		case Stmt::BinaryOperatorClass:
			{
				BinaryOperator const * pExpr = cast<BinaryOperator>(pStmt);
				pHtExpr->setBinaryOperator(pExpr->getOpcode());
				pHtExpr->setLhs( visitExpr(pExpr->getLHS(), isRhs && pExpr->getOpcode() != BO_Assign) );
				pHtExpr->setRhs( visitExpr(pExpr->getRHS(), isRhs) );
			}
			break;
		case Stmt::ConditionalOperatorClass:
			{
				ConditionalOperator const * pExpr = cast<ConditionalOperator>(pStmt);
				pHtExpr->setCond( visitExpr( pExpr->getCond(), true ));
				pHtExpr->setLhs( visitExpr( pExpr->getLHS(), isRhs ));
				pHtExpr->setRhs( visitExpr( pExpr->getRHS(), isRhs ));
			}
			break;
		case Stmt::MemberExprClass:
			{
				MemberExpr const * pMemberExpr = cast<MemberExpr>(pStmt);

				pHtExpr->isArrow( cast<MemberExpr>(pStmt)->isArrow() );
				pHtExpr->setBase( visitExpr(pMemberExpr->getBase(), isRhs ));
				pHtExpr->setMemberDecl( visitDecl( pMemberExpr->getMemberDecl() ) );

				if (pHtExpr->getMemberDecl()->getKind() == Decl::CXXConversion) {
					SourceRange sr = pMemberExpr->getSourceRange();
					SourceLocation B = sr.getBegin();
					SourceLocation E = sr.getEnd();

					pHtExpr->setIsImplicitNode( pMemberExpr->getSourceRange() == pMemberExpr->getBase()->getSourceRange() );

					//if (!pHtExpr->isImplicitNode()) {
					//	B.dump(m_pASTContext->getSourceManager());
					//	E.dump(m_pASTContext->getSourceManager());
					//}
				}
			}
			break;
		case Stmt::CXXDependentScopeMemberExprClass:
			{
				CXXDependentScopeMemberExpr const * pExpr = cast<CXXDependentScopeMemberExpr>(pStmt);
				pHtExpr->setSubExpr( visitExpr( *( pExpr->child_begin()), isRhs ));
			}
			break;
		case Stmt::DeclRefExprClass:
			{
				pHtExpr->setRefDecl( visitDecl( cast<DeclRefExpr>(pStmt)->getDecl() ) );
			}
			break;
		case Stmt::CXXBoolLiteralExprClass:
			//pHtExpr->setBooleanLiteral( cast<CXXBoolLiteralExpr>(pStmt)->getValue() );
			break;
		case Stmt::IntegerLiteralClass:
			//pHtExpr->setIntegerLiteral(cast<IntegerLiteral>(pStmt)->getValue(),
			//	cast<IntegerLiteral>(pStmt)->getType()->isSignedIntegerType() );
			break;
		case Stmt::StringLiteralClass:
			pHtExpr->setStringLiteral(cast<StringLiteral>(pStmt)->getString());
			break;
		case Stmt::FloatingLiteralClass:
			//pHtExpr->setFloatingLiteral( cast<FloatingLiteral>(pStmt)->getValue() );
			break;
		case Stmt::ImplicitCastExprClass:
			{
				ImplicitCastExpr const * pCastExpr = cast<ImplicitCastExpr>(pStmt);
				pHtExpr->setSubExpr( visitExpr( *( pCastExpr->child_begin()), isRhs ));
			}
			break;
		case Stmt::ArraySubscriptExprClass:
			{
				ArraySubscriptExpr const * pSubscriptExpr = cast<ArraySubscriptExpr>(pStmt);
				pHtExpr->setBase( visitExpr( pSubscriptExpr->getBase(), isRhs ));
				pHtExpr->setIdx( visitExpr( pSubscriptExpr->getIdx(), true ));
			}
			break;
		case Stmt::CXXUnresolvedConstructExprClass:
			{
				CXXUnresolvedConstructExpr const * pCXXConstructExpr = cast<CXXUnresolvedConstructExpr>(pStmt);

				for (Stmt::const_child_range CI = pStmt->children(); CI; ++CI)
					pHtExpr->addArgExpr( visitExpr( *CI, true ));
			}
			break;
		case Stmt::CXXConstructExprClass:
			{
				CXXConstructExpr const * pCXXConstructExpr = cast<CXXConstructExpr>(pStmt);

				pHtExpr->isElidable( pCXXConstructExpr->isElidable() );

				pHtExpr->setConstructorDecl( visitDecl( pCXXConstructExpr->getConstructor() ));

				if (pCXXConstructExpr->getNumArgs() == 1)
					pHtExpr->setIsImplicitNode( pCXXConstructExpr->getSourceRange() == pCXXConstructExpr->getArg(0)->getSourceRange() );

				for (uint32_t i = 0; i < pCXXConstructExpr->getNumArgs(); i += 1)
					pHtExpr->addArgExpr( visitExpr( pCXXConstructExpr->getArg( i ), true ));
			}
			break;
		case Stmt::CXXFunctionalCastExprClass:
			{
				CXXFunctionalCastExpr const * pCastExpr = cast<CXXFunctionalCastExpr>(pStmt);
				pHtExpr->setSubExpr( visitExpr( pCastExpr->getSubExpr(), isRhs ));
			}
			break;
		case Stmt::CallExprClass:
			{
				CallExpr const * pCallExpr = cast<CallExpr>(pStmt);

				pHtExpr->setCalleeExpr( visitExpr( pCallExpr->getCallee(), false ));
				pHtExpr->setCalleeDecl( visitDecl( pCallExpr->getCalleeDecl() ));

				for (uint32_t i = 0; i < pCallExpr->getNumArgs(); i += 1)
					pHtExpr->addArgExpr( visitExpr( pCallExpr->getArg( i ), true ));
			}
			break;
		case Stmt::CXXMemberCallExprClass:
			{
				CXXMemberCallExpr const * pCallExpr = cast<CXXMemberCallExpr>(pStmt);
				
				pHtExpr->setCalleeExpr( visitExpr( pCallExpr->getCallee(), false ));
				pHtExpr->setMemberDecl( visitDecl( pCallExpr->getCalleeDecl() ));

				for (uint32_t i = 0; i < pCallExpr->getNumArgs(); i += 1)
					pHtExpr->addArgExpr( visitExpr( pCallExpr->getArg( i ), true ));
			}
			break;
		case Stmt::MaterializeTemporaryExprClass:
			{
				MaterializeTemporaryExpr const * pTempExpr = cast<MaterializeTemporaryExpr>(pStmt);
				pHtExpr->setSubExpr( visitExpr( pTempExpr->GetTemporaryExpr(), isRhs ));
			}
			break;
		case Stmt::SubstNonTypeTemplateParmExprClass:
			{
				SubstNonTypeTemplateParmExpr const * pSubstExpr = cast<SubstNonTypeTemplateParmExpr>(pStmt);
				pHtExpr->setReplacement( visitExpr( pSubstExpr->getReplacement(), true ));
			}
			break;
		case Stmt::CXXOperatorCallExprClass:
			{
				CXXOperatorCallExpr const * pCallExpr = cast<CXXOperatorCallExpr>(pStmt);

				pHtExpr->setCalleeDecl( visitDecl( pCallExpr->getCalleeDecl() ));
				pHtExpr->setCalleeExpr( visitExpr( pCallExpr->getCallee(), false ));

				for (uint32_t i = 0; i < pCallExpr->getNumArgs(); i += 1)
					pHtExpr->addArgExpr( visitExpr( pCallExpr->getArg( i ), true ));
			}
			break;
		case Stmt::CXXThisExprClass:
			{
				CXXThisExpr const * pThisExpr = cast<CXXThisExpr>(pStmt);
			}
			break;
		case Stmt::CXXStaticCastExprClass:
			{
				CXXStaticCastExpr const * pCastExpr = cast<CXXStaticCastExpr>(pStmt);
				pHtExpr->setSubExpr( visitExpr( *(pCastExpr->child_begin()), isRhs ));
			}
			break;
		case Stmt::CStyleCastExprClass:
			{
				CStyleCastExpr const * pCastExpr = cast<CStyleCastExpr>(pStmt);
				pHtExpr->setSubExpr( visitExpr( *(pCastExpr->child_begin()), isRhs ));
			}
			break;
		case Stmt::CXXTemporaryObjectExprClass:
			{
				CXXTemporaryObjectExpr const * pTempObjExpr = cast<CXXTemporaryObjectExpr>(pStmt);

				//pHtExpr->isElidable( pCXXConstructExpr->isElidable() );

				pHtExpr->setConstructorDecl( visitDecl( pTempObjExpr->getConstructor() ));

				//if (pCXXConstructExpr->getNumArgs() == 1)
				//	pHtExpr->setIsImplicitNode( pCXXConstructExpr->getSourceRange() == pCXXConstructExpr->getArg(0)->getSourceRange() );

				for (uint32_t i = 0; i < pTempObjExpr->getNumArgs(); i += 1)
					pHtExpr->addArgExpr( visitExpr( pTempObjExpr->getArg( i ), true ));
			}
			break;
		case Stmt::ParenExprClass:
			{
				ParenExpr const * pParenExpr = cast<ParenExpr>(pStmt);
				pHtExpr->setSubExpr( visitExpr( pParenExpr->getSubExpr(), isRhs ));
			}
			break;
		case Stmt::InitListExprClass:
			{
				InitListExpr const * pListExpr = cast<InitListExpr>(pStmt);

				for (Stmt::const_child_iterator CI = pListExpr->child_begin(); CI != pListExpr->child_end(); CI++)
					pHtExpr->addInitExpr( visitExpr( *CI, true ));
			}
			break;
		default:
			assert(0);
		}

		return pHtExpr;
	}

	HtQualType HtMirrorDesign::visitQualType( QualType & qualType )
	{
		HtQualType htQualType;
		htQualType.setType( visitType( qualType.getTypePtr() ));
		htQualType.isConst( qualType.isConstQualified() );

		return htQualType;
	}

	HtType * HtMirrorDesign::visitType( Type const * pType )
	{
		// if we already visited this type, just return with HtType
		HtType * pHtType = m_typeMap.findType( pType );
		if (pHtType)
			return pHtType;

		// new type, add to table
		pHtType = new HtType(pType);
		m_typeMap.addType( pType, pHtType );

		Type::TypeClass typeClass = pType->getTypeClass();
		switch (typeClass) {
		case Type::Elaborated:
			{
				ElaboratedType const * pElabType = cast<ElaboratedType>(pType);

				if (pElabType->isStructureType())
					pHtType->setQualType( visitType( pElabType->getAsStructureType() ));
				else
					assert(0);

				pHtType->setHtDecl( pHtType->getType()->getHtDecl() );
			}
			break;
		case Type::DependentSizedArray:
			{
				DependentSizedArrayType const * pDependentArray = cast<DependentSizedArrayType>(pType);

				pHtType->setQualType( visitQualType( pDependentArray->getElementType() ));

				pHtType->setSizeExpr( visitExpr( pDependentArray->getSizeExpr(), true ));
			}
			break;
		case Type::ConstantArray:
			{
				ConstantArrayType const * pConstArray = cast<ConstantArrayType>(pType);

				pHtType->setQualType( visitQualType( pConstArray->getElementType() ));

				pHtType->setSize( pConstArray->getSize() );
			}
			break;
		case Type::Paren:
			{
				ParenType const * pParen = cast<ParenType>(pType);
				pHtType->setQualType( visitQualType( pParen->getInnerType() ));
			}
			break;
		case Type::Decayed:
			{
				DecayedType const * pDecayed = cast<DecayedType>(pType);
				pHtType->setQualType( visitQualType( pDecayed->getDecayedType() ));
			}
			break;
		case Type::Pointer:
			{
				pHtType->setQualType( visitQualType( pType->getPointeeType() ));
			}
			break;
		case Type::LValueReference:
			{
				LValueReferenceType const * pLValRef = cast<LValueReferenceType>(pType);
				pHtType->setQualType( visitQualType( pLValRef->getPointeeType() ));
			}
			break;
		case Type::RValueReference:
			{
				RValueReferenceType const * pRValRef = cast<RValueReferenceType>(pType);
				pHtType->setQualType( visitQualType( pRValRef->getPointeeType() ));
			}
			break;
		case Type::Record:
			{
				RecordType const * pRecordType = cast<RecordType>(pType);
				pHtType->setHtDecl( visitDecl( pRecordType->getDecl() ));
			}
			break;
		case Type::Builtin:
			{
				initializeBuiltinType(pHtType);
			}
			break;
		case Type::FunctionProto:
			{
				FunctionProtoType const * pFuncProto = cast<FunctionProtoType>(pType);
				pHtType->setQualType( visitQualType( pFuncProto->getResultType() ));
			}
			break;
		case Type::InjectedClassName:
			{
				InjectedClassNameType const * pInjectedClassName = cast<InjectedClassNameType>(pType);

				pHtType->setQualType( visitType( pInjectedClassName->getInjectedTST() ));
			}
			break;
		case Type::TemplateSpecialization:
			{
				TemplateSpecializationType const * pTST = cast<TemplateSpecializationType>(pType);
				pHtType->setTemplateDecl( m_declMap.findDecl( pTST->getTemplateName().getAsTemplateDecl() ));

				for (uint32_t i = 0; i < pTST->getNumArgs(); i += 1)
					pHtType->addTemplateArg( visitTemplateArg( &pTST->getArg(i) ));
			}
			break;
		case Type::Typedef:
			{
				TypedefType const * pTypedefType = cast<TypedefType>(pType);
				pHtType->setHtDecl( visitDecl( pTypedefType->getDecl() ));
			}
			break;
		case Type::TemplateTypeParm:
			{
				TemplateTypeParmType const * pTypeParamType = cast<TemplateTypeParmType>(pType);
				pHtType->setTemplateTypeParmName( pTypeParamType->getIdentifier()->getName().str() );
			}
			break;
		case Type::SubstTemplateTypeParm:
			{
				SubstTemplateTypeParmType const * pSubstType = cast<SubstTemplateTypeParmType>(pType);
				pHtType->setQualType( visitQualType( pSubstType->getReplacementType() ));
			}
			break;
		case Type::Enum:
			{
				EnumType const * pEnumType = cast<EnumType>(pType);
				pHtType->setHtDecl( visitDecl( pEnumType->getDecl() ));
			}
			break;
		default:
			pType->dump();
			assert(0);
		}

		return pHtType;
	}

	void HtMirrorDesign::initializeBuiltinType(HtType * pHtType)
	{
		HtDecl * pHtDecl = new HtDecl(HtLineInfo(0));

		pHtType->setHtDecl( pHtDecl );

		switch ( pHtType->getBuiltinTypeKind() ) {
		case BuiltinType::Void:
			pHtDecl->setName("void");
			pHtDecl->setBitSize(0);
			break;
		case BuiltinType::Bool:
			pHtDecl->setName("bool");
			pHtDecl->setBitSize(1);
			break;
		case BuiltinType::UChar:
			pHtDecl->setName("unsigned char");
			pHtDecl->setBitSize(8);
			break;
		case BuiltinType::Short:
			pHtDecl->setName("short");
			pHtDecl->setBitSize(16);
			break;
		case BuiltinType::UShort:
			pHtDecl->setName("unsigned short");
			pHtDecl->setBitSize(16);
			break;
		case BuiltinType::Int:
			pHtDecl->setName("int");
			m_pHtDesign->setBuiltinIntType(pHtType);
			pHtDecl->setBitSize(32);
			break;
		case BuiltinType::UInt:
			pHtDecl->setName("unsigned int");
			pHtDecl->setBitSize(32);
			break;
		case BuiltinType::ULongLong:
			pHtDecl->setName("unsigned long long");
			pHtDecl->setBitSize(64);
			break;
		case BuiltinType::LongLong:
			pHtDecl->setName("long long");
			pHtDecl->setBitSize(64);
			break;
		case BuiltinType::Int128:
			pHtDecl->setName("__int128");
			pHtDecl->setBitSize(128);
			break;
		case BuiltinType::UInt128:
			pHtDecl->setName("unsigned __int128");
			pHtDecl->setBitSize(128);
			break;
		case BuiltinType::Char_S:
			pHtDecl->setName("char");
			pHtDecl->setBitSize(8);
			break;
		case BuiltinType::Float:
			pHtDecl->setName("float");
			pHtDecl->setBitSize(32);
			break;
		case BuiltinType::Double:
			pHtDecl->setName("double");
			pHtDecl->setBitSize(64);
			break;
		case BuiltinType::BoundMember:
			pHtDecl->setName("<bound member builtin type>");
			break;
		case BuiltinType::Dependent:
			pHtDecl->setName("<dependent type>");
			break;
		default:
			assert(0);
		}
	}

	HtTemplateArgument * HtMirrorDesign::visitTemplateArg( TemplateArgument const * pArg )
	{
		HtTemplateArgument * pHtTemplateArgument = new HtTemplateArgument(pArg);

		TemplateArgument::ArgKind argKind = pArg->getKind();
		switch (argKind) {
		case TemplateArgument::Expression:
			{
				pHtTemplateArgument->setExpr( visitExpr( pArg->getAsExpr(), true ));
			}
			break;
		case TemplateArgument::Integral:
			{
				pHtTemplateArgument->setIntegral( pArg->getAsIntegral() );
			}
			break;
		case TemplateArgument::Type:
			{
				pHtTemplateArgument->setQualType( visitQualType( pArg->getAsType() ));
			}
			break;
		default:
			assert(0);
		}

		return pHtTemplateArgument;
	}

	int HtMirrorDesign::FindDeclSize(HtQualType qualType, vector<int> & dimenList, int & typeBitSize, int & alignBitSize)
	{
		int elemCnt = 1;
		while (qualType.getType()->getTypeClass() == Type::ConstantArray) {
			int dimenSize = (int)qualType.getType()->getSize().getZExtValue();
			dimenList.push_back(dimenSize);
			elemCnt *= dimenSize;
			qualType = qualType.getType()->getQualType();
		}

		while (qualType.getType()->getTypeClass() == Type::Typedef)
			qualType = qualType.getType()->getHtDecl()->getQualType();

		switch (qualType.getType()->getTypeClass()) {
		case Type::Builtin:
			typeBitSize = qualType.getType()->getHtDecl()->getBitSize();
			alignBitSize = typeBitSize;
			break;
		case Type::Record:
			typeBitSize = qualType.getType()->getHtDecl()->getBitSize();
			alignBitSize = qualType.getType()->getHtDecl()->getAlignBitSize();
			break;
		case Type::TemplateTypeParm:
			typeBitSize = 0;	// value is not used
			alignBitSize = 1;
			break;
		default:
			assert(0);
		}

		return elemCnt;
	}

	void HtMirrorDesign::PreFindRecordPosAndWidth(HtDecl * pHtDecl, int & savedRecordBitPos,
		bool & bSavedRecordPackedAttr, bool & bSavedRecordIsUnion, int & savedRecordAlignBitSize)
	{
		savedRecordBitPos = m_recordBitPos;
		bSavedRecordPackedAttr = m_bRecordPackedAttr;
		bSavedRecordIsUnion = m_bRecordIsUnion;
		savedRecordAlignBitSize = m_recordAlignBitSize;

		m_recordBitPos = 0;
		m_bRecordPackedAttr = pHtDecl->getType()->isPackedAttr();
		m_bRecordIsUnion = pHtDecl->isUnion();
		m_recordPrevTypeBitSize = -1;
		m_recordAlignBitSize = 1;
	}

	void HtMirrorDesign::PostFindRecordPosAndWidth(HtDecl * pHtDecl, int savedRecordBitPos,
		bool bSavedRecordPackedAttr, bool bSavedRecordIsUnion, int savedRecordAlignBitSize)
	{
		if (!m_bRecordPackedAttr) {
			m_recordBitPos += m_recordAlignBitSize - 1;
			m_recordBitPos &= ~(m_recordAlignBitSize - 1);
		}
		pHtDecl->setBitSize(m_recordBitPos);
		pHtDecl->setAlignBitSize(m_recordAlignBitSize);

		if (g_htvArgs.isDumpPosAndWidth())
			printf("%s recordBitSize=%d\n", pHtDecl->getName().c_str(), m_recordBitPos);

		m_recordBitPos = savedRecordBitPos;
		m_bRecordPackedAttr = bSavedRecordPackedAttr;
		m_bRecordIsUnion = bSavedRecordIsUnion;
		m_recordPrevTypeBitSize = -1;
		m_recordAlignBitSize = savedRecordAlignBitSize;
	}

}
