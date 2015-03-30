/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#pragma once

#include "HtLineInfo.h"
#include "HtTpgInfo.h"

using namespace std;
using namespace clang;

namespace ht {

	class HtDecl;
	class HtStmt;
	class HtExpr;
	class HtPath;

	typedef map<Decl const *, HtDecl *>		HtDeclMap_map;
	typedef pair<Decl const *, HtDecl *>	HtDeclMap_valuePair;
	typedef HtDeclMap_map::iterator			HtDeclMap_iter;
	typedef HtDeclMap_map::const_iterator	HtDeclMap_constIter;
	typedef pair<HtDeclMap_iter, bool>		HtDeclMap_insertPair;

	class HtDeclMap {
	public:

		void addDecl(Decl const * pDecl, HtDecl * pHtDecl) {
			HtDeclMap_insertPair insertPair;
			insertPair = m_declMap.insert(HtDeclMap_valuePair( pDecl, (HtDecl *)0 ));
			HtAssert(insertPair.first->second == 0);	// should always be unique
			insertPair.first->second = pHtDecl;
		}

		HtDecl * findDecl( Decl const * pDecl ) {
			HtDeclMap_iter iter = m_declMap.find( pDecl );

			if (iter != m_declMap.end())
				return iter->second;
			else
				return 0;
		}

	private:
		HtDeclMap_map m_declMap;
	};

	typedef list<HtDecl *> HtDeclList_t;
	typedef HtDeclList_t::iterator	HtDeclList_iter_t;

	class HtDeclIter {
	public:
		HtDeclIter(HtDeclList_t & list) {
			m_pList = &list;
			begin();
		}
		void begin() { m_iter = m_pList->begin(); }
		bool isEol() { return m_iter == m_pList->end(); }
		HtDeclIter operator++(int) { HtDeclIter tmp = *this; m_iter++; return tmp; }
		HtDecl * operator* () { return *m_iter; }
		HtDecl * operator-> () { return *m_iter; }
		void erase() { m_pList->erase(m_iter); }
	private:
		HtDeclList_t * m_pList;
		HtDeclList_iter_t m_iter;
	};

	typedef map<string, HtDecl *>			HtCntxMap_map;
	typedef pair<string, HtDecl *>			HtCntxMap_valuePair;
	typedef HtCntxMap_map::iterator			HtCntxMap_iter;
	typedef HtCntxMap_map::const_iterator	HtCntxMap_constIter;
	typedef pair<HtCntxMap_iter, bool>		HtCntxMap_insertPair;

	class HtCntxMap {
	public:
		HtCntxMap() {}

		void addDecl(HtDecl * pHtDecl);

		HtDecl * findDecl( string name ) {
			HtCntxMap_iter iter = m_declMap.find( name );

			if (iter != m_declMap.end())
				return iter->second;
			else
				return 0;
		}

		HtCntxMap_constIter begin() const { return m_declMap.begin(); }
		HtCntxMap_constIter end() const { return m_declMap.end(); }

	private:
		HtCntxMap_map m_declMap;
	};

	struct RangeRef {
		RangeRef() { m_rdRefCnt = 0; m_wrRefCnt = 0; }
		uint16_t m_bitLow;
		uint16_t m_bitHigh;
		uint16_t m_rdRefCnt;
		uint16_t m_wrRefCnt;
	};

	struct DimenRef {
		vector<int> m_dimenList;
		vector<RangeRef> m_rangeRefList;
	};

	class DeclRef {
		vector<DimenRef> m_dimenRefList;

	public:
		void clear() { m_dimenRefList.clear(); }
		void incRdRefCnt(vector<int> * pDimenList=0, uint16_t bitHigh=0xffff, uint16_t bitLow=0);
		void incWrRefCnt(vector<int> * pDimenList=0, uint16_t bitHigh=0xffff, uint16_t bitLow=0);
		int getRdRefCnt(vector<int> * pDimenList=0, uint16_t bitHigh=0xffff, uint16_t bitLow=0);
		int getWrRefCnt(vector<int> * pDimenList=0, uint16_t bitHigh=0xffff, uint16_t bitLow=0);
	};

	class HtTpInfo {
	public:
		HtTpInfo(bool isTps, string &tpGrpName, string &tpSgName, 
			int tpGrpId, int tpSgId, int tpSgUrs, int tpSgTrs, int tpSgTpd)
			: m_isTps(isTps), m_tpGrpName(tpGrpName), m_tpSgName(tpSgName),
			m_tpGrpId(tpGrpId), m_tpSgId(tpSgId), m_tpSgUrs(tpSgUrs),
			m_tpSgTrs(tpSgTrs), m_tpSgTpd(tpSgTpd) {}

		string & getTpGrpName() { return m_tpGrpName; }
		string & getTpSgName() { return m_tpSgName; }
		int getTpSgId() { return m_tpSgId; }
		int getTpGrpId() { return m_tpGrpId; }
		int getTpSgUrs() { return m_tpSgUrs; }
		int getTpSgTrs() { return m_tpSgTrs; }
		int getTpSgTpd() { return m_tpSgTpd; }

	public:
		bool m_isTps;
		string m_tpGrpName;
		string m_tpSgName;
		int m_tpGrpId;
		int m_tpSgId;
		int m_tpSgUrs;
		int m_tpSgTrs;
		int m_tpSgTpd;
	};

	class HtDecl : public HtNode {
	public:
		HtDecl(HtLineInfo & lineInfo) : HtNode(lineInfo) {
			Init();
		}

		HtDecl(Decl const * pDecl) : HtNode(pDecl->getSourceRange()) {
			Init();
			m_kind = pDecl->getKind();
			m_isImplicit = pDecl->isImplicit();
		}

		HtDecl(HtDecl * pHtDecl, bool link=true ) : HtNode( pHtDecl ) {
			Init();
			// copy constructor
			m_kind = pHtDecl->m_kind;
			m_name = pHtDecl->m_name;
			m_qualType = pHtDecl->m_qualType;
			m_desugaredQualType = pHtDecl->m_desugaredQualType;
		
			m_pDecl = 0;
			m_pGeneralTemplateDecl = 0;
			m_pPath = 0;

			m_pPreviousDecl = 0;

			m_access = pHtDecl->m_access;
			m_tagKind = pHtDecl->m_tagKind;

			m_wasDeclaredWithTypename = pHtDecl->m_wasDeclaredWithTypename;
			m_doesThisDeclarationHaveABody = pHtDecl->m_doesThisDeclarationHaveABody;
			m_hasDefaultArgument = pHtDecl->m_hasDefaultArgument;
			m_isImplicit = pHtDecl->m_isImplicit;
			m_isDeclaredInMethod = pHtDecl->m_isDeclaredInMethod;
			m_hasInitExpr = pHtDecl->m_hasInitExpr;
			m_isTempVar = pHtDecl->m_isTempVar;
			m_isBeingInlined = pHtDecl->m_isBeingInlined;
			m_isCompleteDefinition = pHtDecl->m_isCompleteDefinition;
			m_isRegister = pHtDecl->m_isRegister;

			m_isScMethod = pHtDecl->m_isScMethod;
			m_pHtPragmaList = pHtDecl->m_pHtPragmaList;
			m_pTpInfo = pHtDecl->m_pTpInfo;
		}

		HtDecl(Decl::Kind declKind, HtLineInfo & lineInfo) : HtNode(lineInfo) {
			Init();
			m_kind = declKind;
		}

		void Init() {
			m_pDecl = 0;
			m_pPreviousDecl = 0;
			m_pGeneralTemplateDecl = 0;
			m_isImplicit = false;
			m_doesThisDeclarationHaveABody = false;
			m_hasDefaultArgument = false;
			m_isDeclaredInMethod = false;
			m_hasInitExpr = false;
			m_isScMethod = false;
			m_isTempVar = false;
			m_isBeingInlined = false;
			m_pHtPragmaList = 0;
			m_pPath = 0;
			m_pSubstDecl = 0;
			m_isCompleteDefinition = false;
			m_isRegister = false;
			m_pTpInfo = 0;
			m_pTpGrp = 0;
		}

		void setName(string name) { m_name = name; }
		virtual string getName() { return m_name; }

		void setDeclKind( Decl::Kind declKind ) { m_kind = declKind; }

		void setQualType( HtQualType & qualType ) { assert(m_qualType.getType() == 0); m_qualType = qualType; }
		HtQualType & getQualType() { return m_qualType; }

		void setDesugaredQualType(  HtQualType & qualType ) { assert(m_desugaredQualType.getType() == 0); m_desugaredQualType = qualType; }
		
		HtQualType & getDesugaredQualType() { 
			if (m_desugaredQualType.getType())
				return m_desugaredQualType;
			else
				return m_qualType;
		}

		void setType( HtType * pType ) { assert(m_qualType.getType() == 0); m_qualType.setType( pType ); }
		HtType * getType() { return m_qualType.getType(); }

		bool isImplicit() { return m_isImplicit; }

		void isDeclaredInMethod(bool is) { m_isDeclaredInMethod = is; }
		bool isDeclaredInMethod() { return m_isDeclaredInMethod; }

		void setDefaultArgument( HtExpr * pExpr ) { 
			if (pExpr == 0) return;
			assert(m_defArgExpr.empty());
			m_defArgExpr.push_back((HtStmt*)pExpr); }
		HtStmtList & getDefaultArgument() { return m_defArgExpr; }

		void setTemplateDecl( HtDecl * pDecl ) { assert(m_pDecl == 0); m_pDecl = pDecl; }
		HtDecl * getTemplateDecl() { return m_pDecl; }

		void setFieldBitWidth( int fieldBitWidth ) {
			m_fieldBitWidth = fieldBitWidth;
		}
		bool hasFieldBitWidth() { return m_fieldBitWidth > 0; }
		int getFieldBitWidth() { return m_fieldBitWidth; }

		HtDecl * findFirstDecl() {
			HtDecl * pDecl = this;
			while ( pDecl->getPreviousDecl() )
				pDecl = pDecl->getPreviousDecl();
			return pDecl;
		}

		void setBody( HtStmt * pBody ) {
			if (pBody == 0) return;

			HtDecl * pDecl = findFirstDecl();

			assert(pDecl->m_body.size() == 0);
			pDecl->m_body.push_back( pBody );
		}
		bool hasBody() { return findFirstDecl()->m_body.size() > 0; }

		HtStmtList & getBody() {
			HtDecl * pDecl = findFirstDecl();
			return pDecl->m_body;
		}

		void setInitExpr( HtExpr * pInitExpr ) {
			if (pInitExpr == 0) {
				m_initExpr.clear();
				return;
			}
			assert(m_initExpr.size() == 0);
			m_initExpr.push_back((HtStmt*)pInitExpr);
		}
		bool hasInitExpr() { return m_initExpr.size() > 0; }
		HtStmtList & getInitExpr() { return m_initExpr; }

		void setKind( Decl::Kind kind ) { m_kind = kind; }
		Decl::Kind getKind() { return m_kind; }

		void setTagKind( TagDecl::TagKind tagKind ) { m_tagKind = tagKind; }
		TagDecl::TagKind getTagKind() { return m_tagKind; }
		string getTagKindAsStr() { return TypeWithKeyword::getTagTypeKindName( m_tagKind ); }
		bool isUnion() { return m_tagKind == TTK_Union; }

		void addSubDecl( HtDecl * pHtDecl ) {
			m_subDeclVec.push_back(pHtDecl);
			if (pHtDecl->getKind() != Decl::AccessSpec && pHtDecl->getKind() != Decl::PragmaHtv) {
				assert(pHtDecl->m_name.size() > 0);
				m_cntx.addDecl( pHtDecl );
			}
		}
		HtDeclList_t & getSubDeclList() { return m_subDeclVec; }

		HtDecl * findSubDecl( string name ) { return m_cntx.findDecl( name ); }

		void addSpecializationDecl( HtDecl * pHtDecl ) { m_specializationDeclList.push_back(pHtDecl); }
		HtDeclList_t & getSpecializationDeclList() { return m_specializationDeclList; }

		void addParamDecl( HtDecl * pHtDecl ) { m_paramDeclList.push_back(pHtDecl); }
		HtDeclList_t & getParamDeclList() { return m_paramDeclList; }
		HtDecl * getParamDecl(int i) {
			HtDeclList_iter_t paramIter = m_paramDeclList.begin();
			for (int j = 0; j < i; j += 1)
				paramIter++;
			return *paramIter; 
		}

		HtBaseSpecifier * getFirstBaseSpecifier() { return m_baseSpecifierList.getFirst(); }
		void addBaseSpecifier( HtBaseSpecifier * pHtBaseSpecifier ) { m_baseSpecifierList.append(pHtBaseSpecifier); }

		void setAccess( AccessSpecifier access) { m_access = access; }
		AccessSpecifier getAccess() { return m_access; }
		string getAccessName() {
			switch (m_access) {
			case AS_none: return "";
			case AS_public: return "public";
			case AS_private: return "private";
			case AS_protected: return "protected";
			default: assert(0);
			}
			return "";
		}

		void setGeneralTemplateDecl( HtDecl * pDecl ) { m_pGeneralTemplateDecl = pDecl; }
		HtDecl * getGeneralTemplateDecl() { return m_pGeneralTemplateDecl; }

		void addTemplateArg( HtTemplateArgument * pArg) { m_templateArgList.append( pArg ); }
		HtTemplateArgument * getFirstTemplateArg() { return m_templateArgList.getFirst(); }

		void wasDeclaredWithTypename(bool wasDeclaredWithTypename) { m_wasDeclaredWithTypename = wasDeclaredWithTypename; }
		bool wasDeclaredWithTypename() { return m_wasDeclaredWithTypename; }
		
		void doesThisDeclarationHaveABody(bool doesThisDeclarationHaveABody) { m_doesThisDeclarationHaveABody = doesThisDeclarationHaveABody; }
		bool doesThisDeclarationHaveABody() { return m_doesThisDeclarationHaveABody; }

		void hasDefaultArgument(bool hasDefaultArgument) { m_hasDefaultArgument = hasDefaultArgument; }
		bool hasDefaultArgument() { return m_hasDefaultArgument; }

		void setDefaultArgQualType(HtQualType & qualType) { assert(m_qualType.getType() == 0); m_qualType = qualType; }
		HtQualType & getDefaultArgQualType() { return m_qualType; }

		// Parent decl points to CXXRecord for a CXXMethod
		void setParentDecl( HtDecl * pDecl ) { assert(m_pParentDecl == 0); m_pParentDecl = pDecl; }
		HtDecl * getParentDecl() { return m_pParentDecl; }

		// Previous decl points to previous place CXXMethod was declared
		void setPreviousDecl( HtDecl * pDecl ) { assert(m_pPreviousDecl == 0); m_pPreviousDecl = pDecl; }
		HtDecl * getPreviousDecl() const { return m_pPreviousDecl; }

		HtCntxMap * getCntx() { return & m_cntx; }

		// must follow prevDecl link to find head decl
		void isScMethod(bool is) { findFirstDecl()->m_isScMethod = is; }
		bool isScMethod() { return findFirstDecl()->m_isScMethod; }

		void pushInlineDecl( HtDecl * pInlineDecl ) { findFirstDecl()->m_inlineDecl.push_back(pInlineDecl); }
		void popInlineDecl() { findFirstDecl()->m_inlineDecl.pop_back(); }
		HtDecl * getInlineDecl() {
			HtDecl * pFirstDecl = findFirstDecl();
			return pFirstDecl->m_inlineDecl.size() > 0 ? pFirstDecl->m_inlineDecl.back() : 0;
		}

		void isCompleteDefinition(bool is) { m_isCompleteDefinition = is; }
		bool isCompleteDefinition() { return m_isCompleteDefinition; }

		void isTempVar(bool is) { m_isTempVar = is; m_isKeepTempVar = false; }
		bool isTempVar() { return m_isTempVar; }

		void isKeepTempVar(bool is) { m_isKeepTempVar = is; }
		bool isKeepTempVar() { return m_isKeepTempVar; }

		void isBeingInlined(bool is) { m_isBeingInlined = is; }
		bool isBeingInlined() { return m_isBeingInlined; }

		void isRegister(bool is) { m_isRegister = is; }
		bool isRegister() { return m_isRegister; }

		void setTpInfo(HtTpInfo * pTpInfo) { findFirstDecl()->m_pTpInfo = pTpInfo; }
		HtTpInfo * getTpInfo() { return findFirstDecl()->m_pTpInfo; }

		void setTpGrp(HtTpg * pTpGrp) { m_pTpGrp = pTpGrp; }
		HtTpg * getTpGrp() { return m_pTpGrp; }

		void setTpGrpId(int tpGrpId) { m_tpGrpId = tpGrpId; }
		int getTpGrpId() { return m_tpGrpId; }

		void setTpSgId(int tpSgId) { m_tpSgId = tpSgId; }
		int getTpSgId() { return m_tpSgId; }

		void setTpSgUrs(int tpSgUrs) { m_tpSgUrs = tpSgUrs; }
		int getTpSgUrs() { return m_tpSgUrs; }

		void setTpSgTpd(int sgTpd) { m_tpgInfo.m_sgTpd = sgTpd; }
		//void setSgTsd(int sgTsd) { m_tpgInfo.m_sgTsd=  sgTsd; }
		int getTpSgTpd() { return m_tpgInfo.m_sgTpd; }

		void initTpSgTrs() {
			m_maxRdTrs = TRS_UNKNOWN;

			if (m_pTpInfo && m_pTpInfo->m_isTps) {
				m_minWrTrs = 0;
				m_maxWrTrs = 0;
				setTpSgTpd(m_pTpInfo->getTpSgTpd());
			} else if (isRegister()) {
				m_minWrTrs = 0;
				m_maxWrTrs = 0;
				setTpSgTpd(TPD_REG_OUT);
			} else {
				m_minWrTrs = TRS_UNKNOWN;
				m_maxWrTrs = TRS_UNKNOWN;
				setTpSgTpd(TPD_UNKNOWN);
			}
		}

		void setTpSgRdTrs(int rdTrs) { m_maxRdTrs = max(m_maxRdTrs, rdTrs); }
		int getTpSgMaxRdTrs() { return m_maxRdTrs; }

		void setTpSgWrTrs(int wrTrs) {
			if (wrTrs == TRS_UNKNOWN)
				assert(0);
			//if (wrTrs == URS_ISCONST)
			//	return;	// constant

			if (m_pTpInfo && m_pTpInfo->m_isTps)
				assert(0);	// tps should not be written

			if (isRegister())
				m_pTpGrp->setSgMaxWrTrs( getTpSgId(), getTpSgUrs(), wrTrs );

			if (m_minWrTrs < 0 || m_minWrTrs > wrTrs)
				m_minWrTrs = wrTrs;
			if (m_maxWrTrs < wrTrs)
				m_maxWrTrs = wrTrs;
		}
		int getTpSgMinWrTrs() { return m_minWrTrs; }
		int getTpSgMaxWrTrs(bool bMinRd=false) {
			if (m_pTpInfo && m_pTpInfo->m_isTps)
				return m_maxWrTrs;
			else if (isRegister()) {
				if (bMinRd)
					return 0;
				else
					return m_pTpGrp->getSgMaxWrTrs( getTpSgId(), getTpSgUrs() );
			} else
				return m_maxWrTrs; 
		}

		void setSubstExpr(HtExpr * pSubstExpr) { m_pSubstExpr = pSubstExpr; }
		HtExpr * getSubstExpr() { return m_pSubstExpr; }

		void setPragmaTokens(ArrayRef<Token const> tokens) { m_pragmaTokens = tokens; }
		ArrayRef<Token const> getPragmaTokens() { return m_pragmaTokens; }

		void setPragmaList(vector<HtPragma *> * pHtPragmaList) { findFirstDecl()->m_pHtPragmaList = pHtPragmaList; }
		bool isHtPrim() {
			HtDecl * pFirstDecl = findFirstDecl();
			if (pFirstDecl->m_pHtPragmaList == 0) return false;
			for (size_t i = 0; i < pFirstDecl->m_pHtPragmaList->size(); i += 1)
				if ((*(pFirstDecl->m_pHtPragmaList))[i]->isHtPrim())
					return true;
			return false;
		}
		bool isScModule() {
			HtDecl * pFirstDecl = findFirstDecl();
			if (pFirstDecl->m_pHtPragmaList == 0) return false;
			for (size_t i = 0; i < pFirstDecl->m_pHtPragmaList->size(); i += 1)
				if ((*(pFirstDecl->m_pHtPragmaList))[i]->isScModule())
					return true;
			return false;
		}
		bool isScCtor() {
			HtDecl * pFirstDecl = findFirstDecl();
			if (pFirstDecl->m_pHtPragmaList == 0) return false;
			for (size_t i = 0; i < pFirstDecl->m_pHtPragmaList->size(); i += 1)
				if ((*(pFirstDecl->m_pHtPragmaList))[i]->isScCtor())
					return true;
			return false;
		}

		void setSubstDecl(HtDecl * pSubstDecl) { m_pSubstDecl = pSubstDecl; }
		HtDecl * getSubstDecl() { return m_pSubstDecl; }

		void clearRefCnt() { m_wrRefCnt = 0; m_rdRefCnt = 0; }
		void incRdRefCnt() { m_rdRefCnt += 1; }
		void incWrRefCnt() { m_wrRefCnt += 1; }
		int getRdRefCnt() { return m_rdRefCnt; }
		int getWrRefCnt() { return m_wrRefCnt; }

		//void incRdRefCnt(vector<int> * pDimenList=0, uint16_t bitHigh=0xffff, uint16_t bitLow=0)
		//	{ m_declRef.incRdRefCnt(pDimenList, bitHigh, bitLow); }
		//void incWrRefCnt(vector<int> * pDimenList=0, uint16_t bitHigh=0xffff, uint16_t bitLow=0)
		//	{ m_declRef.incWrRefCnt(pDimenList, bitHigh, bitLow); }
		//int getRdRefCnt(vector<int> * pDimenList=0, uint16_t bitHigh=0xffff, uint16_t bitLow=0)
		//	{ return m_declRef.getRdRefCnt(pDimenList, bitHigh, bitLow); }
		//int getWrRefCnt(vector<int> * pDimenList=0, uint16_t bitHigh=0xffff, uint16_t bitLow=0)
		//	{ return m_declRef.getWrRefCnt(pDimenList, bitHigh, bitLow); }

		void setDimenList(vector<int> &dimenList) { m_dimenList = dimenList; }
		void setBitSize(int bitSize) { m_bitSize = bitSize; }
		void setBitPos(int bitPos) { m_bitPos = bitPos; }
		void setAlignBitSize(int alignBitSize) { m_alignBitSize = alignBitSize; }

		vector<int> const & getDimenList() const { return m_dimenList; }
		int getBitSize() { return m_bitSize; }
		int getBitPos() { return m_bitPos; }
		int getAlignBitSize() { return m_alignBitSize; }

		void setPath(HtPath * pPath) { m_pPath = pPath; }
		HtPath * getPath() { return m_pPath; }

	private:
		Decl::Kind m_kind;
		string m_name;
		HtQualType m_qualType;		// type for declaration
		HtQualType m_desugaredQualType;
		HtStmtList m_body;
		
		HtCntxMap m_cntx;

		HtStmtList m_initExpr;		// initializer expression
		HtStmtList m_defArgExpr;	// default argument expression
		int m_fieldBitWidth;		// field bit width

		union {
			HtDecl * m_pDecl;
			HtDecl * m_pParentDecl;
		};
		HtDecl * m_pGeneralTemplateDecl;	// for specialized templates

		HtDecl * m_pSubstDecl;
		HtExpr * m_pSubstExpr;

		vector<HtDecl *> m_inlineDecl;
		HtDecl * m_pPreviousDecl;

		AccessSpecifier m_access;
		TagDecl::TagKind m_tagKind;

		ArrayRef<Token const> m_pragmaTokens;

		bool m_wasDeclaredWithTypename;
		bool m_doesThisDeclarationHaveABody;
		bool m_hasDefaultArgument;
		bool m_isImplicit;
		bool m_isDeclaredInMethod;
		bool m_hasInitExpr;
		bool m_isTempVar;
		bool m_isBeingInlined;
		bool m_isCompleteDefinition;
		bool m_isKeepTempVar;
		bool m_isRegister;

		HtTpInfo * m_pTpInfo;

		struct DeclTpgInfo {	// TPG info
			int m_sgTpd;
			int m_sgTsd;

			DeclTpgInfo() {
				m_sgTpd = 0;
				m_sgTsd = 0;
			}
		} m_tpgInfo;

		// Declaration timing path info
		struct {
			HtTpg * m_pTpGrp;
			int m_tpGrpId;
			int m_tpSgId;
			int m_tpSgUrs;
			int m_maxRdTrs;
			int m_minWrTrs;	// min wrTrs for an if or case statement
			int m_maxWrTrs;
		};

		vector<int> m_dimenList;	// list of dimension sizes for array
		int m_bitPos;	// starting position of field within record
		int m_bitSize;	// base type size
		int m_alignBitSize;

		//DeclRef m_declRef;

		// summary reference counts for variable (not broken down for fields)
		int m_wrRefCnt;
		int m_rdRefCnt;

		HtDeclList_t m_subDeclVec;
		HtDeclList_t m_paramDeclList;
		HtDeclList_t m_specializationDeclList;

		HtBaseSpecifierList m_baseSpecifierList;

		HtTemplateArgList m_templateArgList;

		bool m_isScMethod;
		vector<HtPragma *> * m_pHtPragmaList;

		HtPath * m_pPath;
	};
}
