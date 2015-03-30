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

using namespace std;

namespace ht {

	class HtStmt;
	class HtExpr;
	class HtStmtIter;

	union HtStmtPtr {
		HtStmtPtr(HtStmt * pStmt) : m_pStmt(pStmt) {}
		HtStmtPtr(HtExpr * pExpr) : m_pExpr(pExpr) {}

		HtStmt * &asStmt() { return m_pStmt; }
		HtExpr * &asExpr() { return m_pExpr; }

	private:
		HtStmt * m_pStmt;
		HtExpr * m_pExpr;	// provide visibility during debugging
	};

	typedef list<HtStmtPtr> HtStmtList_t;
	typedef HtStmtList_t::iterator	HtStmtList_iter_t;

	class HtStmtList {
	public:
		HtStmtList() {
			m_isCompoundStmtList = false;
		}
		// null copy constructor - statement lists are not copied when HtStmt is duplicated
		HtStmtList(HtStmtList & rhs) {
			m_isCompoundStmtList = rhs.m_isCompoundStmtList;
		}
		//void operator= (HtStmtList const & rhs) {
		//	m_isCompoundStmtList = rhs.m_isCompoundStmtList;
		//}

		void push_back(HtStmt * pStmt) {
			if (pStmt == 0) return;
			m_stmtList.push_back(HtStmtPtr(pStmt));
		}
		void push_front(HtStmt * pStmt) {
			if (pStmt == 0) return;
			m_stmtList.push_front(HtStmtPtr(pStmt));
		}
		void pop_back() {
			m_stmtList.pop_back();
		}

		size_t size() { return m_stmtList.size(); }
		bool empty() { return m_stmtList.empty(); }
		void clear() { m_stmtList.clear(); }

		HtStmtPtr & back() { return m_stmtList.back(); }
		HtStmtPtr & front() { return m_stmtList.front(); }

		void erase(HtStmtList_iter_t & iter) { m_stmtList.erase(iter); }
		void erase(HtStmtList_iter_t & first, HtStmtList_iter_t & last) { m_stmtList.erase(first, last); }
		void append(HtStmtIter & first, HtStmtIter & last);
		void append(HtStmt * pStmt) { m_stmtList.push_back(pStmt); }
		void insert(HtStmtList_iter_t & iter, HtStmt * pStmt)
		{ 
			m_stmtList.insert(iter, pStmt); 
		}
		void insert(HtStmtList_iter_t & iter, HtStmtList_iter_t & beginIter, HtStmtList_iter_t & endIter) { 
			m_stmtList.insert(iter, beginIter, endIter);
		}
		void replace(HtStmtList_iter_t & iter, HtStmt * pStmt)
		{ 
			*iter = pStmt;
		}

		HtStmtList_t & getList() { return m_stmtList; }
		HtStmtList_t const & getList() const { return m_stmtList; }

		HtStmtList_iter_t begin() { return m_stmtList.begin(); }
		HtStmtList_iter_t end() { return m_stmtList.end(); }

		HtStmt * operator-> () { return m_stmtList.front().asStmt(); }
		HtStmt * asStmt() { return m_stmtList.front().asStmt(); }
		HtExpr * asExpr() { return m_stmtList.front().asExpr(); }

		void isCompoundStmtList( bool is ) { m_isCompoundStmtList = is; }
		bool isCompoundStmtList() { return m_isCompoundStmtList; }

	private:
		bool m_isCompoundStmtList;	// true if a compound stmt list
		HtStmtList_t m_stmtList;
	};

	class HtStmtIter {
		friend HtStmtList;
	public:
		HtStmtIter() {
			m_pStmtList = 0;
		}
		HtStmtIter(HtStmtList & list) : m_pStmtList(&list), m_iter(list.begin()) {}
		HtStmtIter(HtStmtList & list, HtStmtList_iter_t & iter) : m_pStmtList(&list), m_iter(iter) {}
		HtStmtIter(HtStmtList * pList, HtStmtList_iter_t & iter) : m_pStmtList(pList), m_iter(iter) {}

		size_t listSize() { return m_pStmtList->size(); }
		bool isEol() { return m_iter == m_pStmtList->end(); }
		HtStmtIter operator++(int) { HtStmtIter tmp = *this; m_iter++; return tmp; }
		HtStmtIter operator--(int) { HtStmtIter tmp = *this; m_iter--; return tmp; }
		HtStmtIter operator+ (int i) { assert(i == 1); HtStmtIter tmp = *this; tmp++; return tmp; }
		HtStmtIter operator- (int i) { assert(i == 1); HtStmtIter tmp = *this; tmp--; return tmp; }
		HtStmt & operator() () { return *(m_iter->asStmt()); }
		HtStmt * operator-> () { return m_iter->asStmt(); }
		void erase() { m_pStmtList->erase(m_iter); }
		void eraseToEol() { m_pStmtList->erase(m_iter, m_pStmtList->end()); }
		void appendToEol(HtStmtIter & first, HtStmtIter & last) { m_pStmtList->append(first, last); }
		void appendToEol(HtStmt * pStmt) { m_pStmtList->append(pStmt); }
		void insert(HtStmt * pStmt) {
			// insert single statement into list
			if (m_pStmtList->size() + 1 > 1 && !m_pStmtList->isCompoundStmtList())
				ConvertToCompoundList();

			m_pStmtList->insert(m_iter, pStmt);
		}
		void insert(HtStmtList &list) {
			// insert entire list of statements
			if (m_pStmtList->size() + list.size() > 1 && !m_pStmtList->isCompoundStmtList())
				ConvertToCompoundList();

			m_pStmtList->insert(m_iter, list.begin(), list.end());
		}
		void replace(HtStmt * pStmt) {
			*m_iter = pStmt;
		}
		operator HtStmt * () { return m_iter->asStmt(); }
		HtExpr * asExpr() { return m_iter->asExpr(); }
		HtStmt * asStmt() { return m_iter->asStmt(); }

		HtStmtList & list() { return *m_pStmtList; }
		HtStmtIter eol() { return HtStmtIter(m_pStmtList, m_pStmtList->end()); }

	private:
		HtStmtList_iter_t & iter() { return m_iter; }
		void ConvertToCompoundList();

	private:
		HtStmtList * m_pStmtList;
		HtStmtList_iter_t m_iter;
	};

	inline void HtStmtList::append(HtStmtIter & first, HtStmtIter & last) {
		m_stmtList.insert(m_stmtList.end(), first.iter(), last.iter());
	}

}
