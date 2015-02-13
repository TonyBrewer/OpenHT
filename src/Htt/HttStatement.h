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

class CHttStatement : public CHtfeStatement
{
public:
	CHttStatement() : CHtfeStatement() {
		m_bIsSynthesized = false;
        m_bIsInSynSet = false;
		m_synSetSize = 0;
		m_bIsClockedAlwaysAt = false;
		m_synSetId = 0;
		m_bIsXilinxFdPrim = false;
	}

	CHttStatement * GetNext() { return (CHttStatement *)CHtfeStatement::GetNext(); }
    CHttStatement **GetPNext() { return (CHttStatement **)CHtfeStatement::GetPNext(); }

    CHttStatement *GetCompound1() { return (CHttStatement *)CHtfeStatement::GetCompound1(); }
    const CHttStatement *GetCompound1() const { return (CHttStatement const *)CHtfeStatement::GetCompound1(); }
    CHttStatement *GetCompound2() { return (CHttStatement *)CHtfeStatement::GetCompound2(); }

    CHttStatement **GetPCompound1() { return (CHttStatement **)CHtfeStatement::GetPCompound1(); }
    CHttStatement **GetPCompound2() { return (CHttStatement **)CHtfeStatement::GetPCompound2(); }

	CHttOperand * GetExpr() { return (CHttOperand *)CHtfeStatement::GetExpr(); }

    bool IsInWriteRefSet(CHtIdentElem identElem) {
		for (unsigned i = 0; i < m_writeRefSet.size(); i += 1)
            if (m_writeRefSet[i].Contains(identElem))
                return true;
        return false;
    }
    void SetIsInSynSet() { Assert(m_bIsInSynSet == false); m_bIsInSynSet = true; }
    bool IsInSynSet() { return m_bIsInSynSet; }

    void AddWriteRef(CHtIdentElem &identElem) {
        for (unsigned int i = 0; i < m_writeRefSet.size(); i += 1)
            if (m_writeRefSet[i].Contains(identElem)) {
				if (identElem.m_elemIdx == -1)
					m_writeRefSet[i].m_elemIdx = -1;
                return;
			}

        m_writeRefSet.push_back(identElem);
    }
    vector<CHtIdentElem> &GetWriteRefSet() { return m_writeRefSet; }

    void SetIsSynthesized() { m_bIsSynthesized = true; }
    bool IsSynthesized() { return m_bIsSynthesized; }

    void SetSynSetSize(int synSetSize) { m_synSetSize = synSetSize; }
    int GetSynSetSize() { return m_synSetSize; }

    void SetIsClockedAlwaysAt() { m_bIsClockedAlwaysAt = true; }
    bool IsClockedAlwaysAt() { return m_bIsClockedAlwaysAt; }

    void SetSynSetId(int synSetId) { m_synSetId = synSetId; }
    int GetSynSetId() { return m_synSetId; }

    void SetIsXilinxFdPrim() { m_bIsXilinxFdPrim = true; }
    bool IsXilinxFdPrim() { return m_bIsXilinxFdPrim; }

private:
    bool		    m_bIsSynthesized;
    bool		    m_bIsInSynSet;
    int			    m_synSetSize;
    int			    m_synSetId;
    bool		    m_bIsClockedAlwaysAt;
    vector<CHtIdentElem>  m_writeRefSet;
    bool		    m_bIsXilinxFdPrim;
};
