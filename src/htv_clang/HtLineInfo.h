/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
// HtLineInfo.h: interface for the CLineInfo class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

using namespace std;

namespace ht {
	struct HtLineInfo {
	public:
		typedef map<string, int>				HtNameMap_map;
		typedef pair<string, int>				HtNameMap_valuePair;
		typedef HtNameMap_map::iterator			HtNameMap_iter;
		typedef HtNameMap_map::const_iterator	HtNameMap_constIter;
		typedef pair<HtNameMap_iter, bool>		HtNameMap_insertPair;

	public:

		HtLineInfo(int) {}
		HtLineInfo( SourceRange &range ) {
			m_range = range;
			SourceLocation & loc = range.getEnd();
			if (loc.isValid()) {
				SourceLocation SpellingLoc = m_pSourceManager->getSpellingLoc(loc);
				PresumedLoc PLoc = m_pSourceManager->getPresumedLoc(SpellingLoc);
				m_pathName = PLoc.getFilename();
				m_lineNum = PLoc.getLine();

				HtNameMap_insertPair insertPair;
				insertPair = m_pathNameMap.insert(HtNameMap_valuePair( m_pathName, 0 ));

				if (insertPair.first->second == 0)
					insertPair.first->second = m_nextFileNum++;

				m_fileNum = insertPair.first->second;
			}
		}

		SourceLocation getLoc() { return m_range.getEnd(); }
		SourceRange getRange() { return m_range; }
		int getBeginColumn() {
			SourceLocation loc = m_range.getBegin();
			SourceLocation SpellingLoc = m_pSourceManager->getSpellingLoc(loc);
			PresumedLoc PLoc = m_pSourceManager->getPresumedLoc(SpellingLoc);
			return PLoc.getColumn();
		}
		int getBeginLine() {
			SourceLocation loc = m_range.getBegin();
			SourceLocation SpellingLoc = m_pSourceManager->getSpellingLoc(loc);
			PresumedLoc PLoc = m_pSourceManager->getPresumedLoc(SpellingLoc);
			return PLoc.getLine();
		}

		bool isInMainFile() {
			SourceLocation loc = m_range.getBegin();
			return m_pSourceManager->isInMainFile(loc);
		}

	public:
		static void setSourceManager(SourceManager * pSourceManager) {
			m_pSourceManager = pSourceManager;
		}

		static HtNameMap_map & getPathNameMap() { return m_pathNameMap; }

	public:
		SourceRange m_range;
		int m_lineNum;
		int m_fileNum;
		string m_pathName;
		string m_fileName;

	private:
		static SourceManager * m_pSourceManager;
		static int m_nextFileNum;
		static HtNameMap_map m_pathNameMap;
	};
}
