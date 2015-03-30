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
#include "HtvDesign.h"

namespace htv {

	// Transform switch statement
	// - transform list of statements for each case/default such that a single break
	//   statement exists at the end of the list.
	// - provide error if the statements from one case flows into the next (i.e. no break)

	class HtvMoveVarDeclInScModule : public HtWalkDesign
	{
	public:
		HtvMoveVarDeclInScModule(HtvDesign * pHtvDesign) : m_pHtvDesign(pHtvDesign) {
			m_isInScMethod = false;
		}

		void handleTopDecl(HtDecl ** ppDecl);
		void handleDeclStmt(HtStmt ** ppStmt);

	private:
		HtvDesign * m_pHtvDesign;
		bool m_isInScMethod;
		HtvDecl * m_pScModuleDecl;
	};

	void HtvDesign::moveVarDeclInScModule()
	{
		HtvMoveVarDeclInScModule transform( this );

		transform.walkDesign( this );
	}

	void HtvMoveVarDeclInScModule::handleTopDecl(HtDecl ** ppDecl)
	{
		HtvDecl ** ppHtvDecl = (HtvDecl **)ppDecl;
		HtvDecl * pHtvDecl = *ppHtvDecl;

		// first step is to put variables in ScModule in flat name table
		if (pHtvDecl->isScModule()) {
			for (HtDeclList_iter declIter = pHtvDecl->getSubDeclList().begin();
				declIter != pHtvDecl->getSubDeclList().end(); declIter++)
			{
				HtvDecl * pSubDecl = (HtvDecl*)*declIter;
				if (pSubDecl->getKind() != Decl::Field)
					continue;

				pSubDecl->setFlatName( m_pHtvDesign->addFlatName( pSubDecl->getName() ));
			}
		}

		if (pHtvDecl->isScMethod()) {
			m_isInScMethod = true;
			assert(pHtvDecl->getParentDecl()->isScModule());
			m_pScModuleDecl = pHtvDecl->getParentDecl();
		} else
			m_isInScMethod = false;

	}

	void HtvMoveVarDeclInScModule::handleDeclStmt(HtStmt ** ppStmt)
	{
		if (!m_isInScMethod)
			return;

		// found a declaration statement within the scModule
		// - need to move declarations to the top
		// - need to turn initialization into init statement here

		HtvStmt * pDeclStmt = (HtvStmt *) *ppStmt;

		// walk through individual decls within statement
		for (HtvDecl ** ppDecl = pDeclStmt->getPFirstDecl(); *ppDecl; ppDecl = (*ppDecl)->getPNextDecl()) {
			HtvDecl * pHtvDecl = *ppDecl;

			// step one: insert top level decl for each decl
			{
				// create copy of decl
				HtvDecl * pTopDecl = new HtvDecl( pHtvDecl );
				pTopDecl->setInitExpr(0);

				// add to ScModule top list
				m_pScModuleDecl->addSubDecl( pTopDecl );

				// get unique name for variable
				pTopDecl->setFlatName( m_pHtvDesign->addFlatName( pTopDecl->getName() ));
				pHtvDecl->setFlatName( pTopDecl->getFlatName() );
			}
		}

		// step two: mark local scope declaration statement as an init statement
		pDeclStmt->isInitStmt( true );
	}
}

