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

using namespace std;
using namespace clang;

namespace ht {

	class HtDecl;
	class HtType;
	class HtStmt;

	typedef map<Type const *, HtType *>		HtTypeMap_map;
	typedef pair<Type const *, HtType *>	HtTypeMap_valuePair;
	typedef HtTypeMap_map::iterator			HtTypeMap_iter;
	typedef HtTypeMap_map::const_iterator	HtTypeMap_constIter;
	typedef pair<HtTypeMap_iter, bool>		HtTypeMap_insertPair;

	class HtTypeMap {
	public:
		HtTypeMap() {}

		void addType(Type const * pType, HtType * pHtType) {
			HtTypeMap_insertPair insertPair;
			insertPair = m_typeMap.insert(HtTypeMap_valuePair( pType, (HtType *)0 ));
			HtAssert(insertPair.first->second == 0);	// should always be unique
			insertPair.first->second = pHtType;
		}

		HtType * findType( Type const * pType ) {
			HtTypeMap_iter iter = m_typeMap.find( pType );

			if (iter != m_typeMap.end())
				return iter->second;
			else
				return 0;
		}

	private:
		HtTypeMap_map m_typeMap;
	};

	class HtType {
	public:
		enum HtPrintType { BaseType, PreType, PostType };
	public:
		HtType(Type const * pType) {
			Init();
			//m_pType = pType;
			m_typeClass = pType->getTypeClass();

			if (m_typeClass == Type::Builtin) {
				BuiltinType const * pBuiltinType = cast<BuiltinType>(pType);
				m_builtinTypeKind = pBuiltinType->getKind();
			}
		}

		void Init() {
			//m_pType = 0;
			m_pDecl = 0;
			m_pHtSubType = 0;
			m_bPackedAttr = false;
		}

		void initializeBuiltinType();

		void setSize( llvm::APInt size ) { m_size = size; }
		llvm::APInt getSize() { return m_size; }

		void setQualType( HtQualType & qualType ) { assert(m_qualType.getType() == 0); m_qualType = qualType; }
		void setQualType( HtType * pType ) { assert(m_qualType.getType() == 0); m_qualType.setType( pType ); }
		HtQualType & getQualType() { return m_qualType; }
		HtType * getType() { return m_qualType.getType(); }

		//HtType * getHtSubType() { return m_pHtSubType; }
		//void setHtSubType( HtType * pHtSubType ) { m_pHtSubType = pHtSubType; }

		HtDecl * getHtDecl() { return m_pDecl; }
		void setHtDecl(HtDecl * pHtDecl) { m_pDecl = pHtDecl; }

		Type::TypeClass getTypeClass() { return m_typeClass; }
		BuiltinType::Kind getBuiltinTypeKind() { return m_builtinTypeKind; }

		void setInjectedTST( HtType * pInjectedType ) { assert(m_pHtSubType == 0); m_pHtSubType = pInjectedType; }
		HtType * getInjectedTST() { return m_pHtSubType; }

		void addTemplateArg( HtTemplateArgument * pArg ) { m_templateArgList.append( pArg ); }
		HtTemplateArgument * getFirstTemplateArg() { return m_templateArgList.getFirst(); }

		void setTemplateDecl( HtDecl * pDecl ) { m_pDecl = pDecl; }
		HtDecl * getTemplateDecl() { return m_pDecl; }

		void setTemplateTypeParmName( string name ) { m_name = name; }
		string & getTemplateTypeParmName() { return m_name; }

		void setSizeExpr( HtExpr * pExpr ) { assert(m_sizeExpr.empty()); m_sizeExpr.push_back((HtStmt*)pExpr); }
		HtStmtList & getSizeExpr() { return m_sizeExpr; }

		void setPackedAttr(bool bAttr=true) { m_bPackedAttr = bAttr; }
		bool isPackedAttr() { return m_bPackedAttr; }

	private:
		Type::TypeClass m_typeClass;
		BuiltinType::Kind m_builtinTypeKind;
		string m_name;
		HtType * m_pHtSubType;
		HtDecl * m_pDecl;	// declaration for this type
		HtStmtList m_sizeExpr;	// for template dependent size
		HtQualType m_qualType;
		llvm::APInt m_size;
		bool m_bPackedAttr;

		HtTemplateArgList m_templateArgList;
	};
}
