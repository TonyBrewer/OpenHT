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

typedef map<string, int>				KeywordMap;
typedef pair<string, int>				KeywordMap_ValuePair;
typedef KeywordMap::iterator			KeywordMap_Iter;
typedef KeywordMap::const_iterator		KeywordMap_ConstIter;
typedef pair<KeywordMap_Iter, bool>		KeywordMap_InsertPair;

class CKeywordMap  
{
public:
    void Insert(const string &str) {
        KeywordMap_InsertPair insertPair;
        m_keywordMap.insert(KeywordMap_ValuePair(str, 0));
    }
    bool Find(const string &str, int & nextUniqueId) {
        KeywordMap_Iter iter = m_keywordMap.find(str);
        if (iter == m_keywordMap.end())
            return false;
		nextUniqueId = iter->second++;
        return true;
    }
private:
    KeywordMap		m_keywordMap;
};
