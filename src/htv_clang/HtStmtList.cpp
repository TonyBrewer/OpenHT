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
		
	void HtStmtIter::ConvertToCompoundList()
	{
		return;

		//// create a compound statement
		//HtStmt * pStmt = new HtStmt( Stmt::CompoundStmtClass );

		//bool atEnd = m_iter == m_pStmtList->end();

		//// move existing statements to compound list
		//HtStmtList_iter_t iter = pStmt->getStmtList().begin();
		//pStmt->getStmtList().insert( iter, m_pStmtList->begin(), m_pStmtList->end() );

		//HtStmtList_iter_t iter2 = m_pStmtList->begin();
		//if (m_pStmtList->empty())
		//	m_pStmtList->insert( iter2, pStmt );
		//else
		//	m_pStmtList->replace( iter2, pStmt );

		//// update iterator to point into compound statment list
		//m_pStmtList = &pStmt->getStmtList();
		//if (atEnd)
		//	m_iter = m_pStmtList->end();
		//else
		//	m_iter = m_pStmtList->begin();
	}

}

