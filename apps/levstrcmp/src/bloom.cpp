/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT levstrcmp application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "Main.h"

#ifndef EXAMPLE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

#include <pthread.h>
#include "editdis.h"

struct CTable {
	int m_nameCnt;
	bool m_bUsed;
	unsigned char m_c0;
	unsigned char m_c1;
	unsigned char m_c2;
};
CTable table[32][32][32];

static unsigned char hashIdxTbl[32][32][32];

void CreateNameHashIdxList(void)
{
	// Create 32x32x32 table indexed by first three characters of name
	//   find number of names for each entry in table
	//   group table entries for uniform hash bin sizes

	memset(table, 0, sizeof(CTable) * 32 * 32 * 32);

	// determine number of names that map to each table entry

	for (uint64_t i = 0; i < tot_num_names; i += 1) {
		unsigned char ch0 = (&lastnames[lastindxs[i]])[0];
		unsigned char ch1 = ch0 == '\0' ? '\0' : (&lastnames[lastindxs[i]])[1];
		unsigned char ch2 = ch1 == '\0' ? '\0' : (&lastnames[lastindxs[i]])[2];
		CTable &entry = table[ch0 & 0x1f][ch1 & 0x1f][ch2 & 0x1f];
		entry.m_nameCnt += 1;
	}

	// create list for sorting
	CTable * pEntryList[32*32*32];
	int idx = 0;
	for (int c0 = 0; c0 < 32; c0 += 1) {
		for (int c1 = 0; c1 < 32; c1 += 1) {
			for (int c2 = 0; c2 < 32; c2 += 1) {
				pEntryList[idx++] = &table[c0][c1][c2];
				table[c0][c1][c2].m_c0 = c0;
				table[c0][c1][c2].m_c1 = c1;
				table[c0][c1][c2].m_c2 = c2;
			}
		}
	}

	// sort list
	for (int c0 = 0; c0 < 32*32*32; c0 += 1) {
		int biggestValue = -1;
		int biggestIdx = -1;
		for (int c1 = c0; c1 < 32*32*32; c1 += 1) {
			if (pEntryList[c1]->m_nameCnt > biggestValue) {
				biggestValue = pEntryList[c1]->m_nameCnt;
				biggestIdx = c1;
			}
		}
		CTable *tmp = pEntryList[c0];
		pEntryList[c0] = pEntryList[biggestIdx];
		pEntryList[biggestIdx] = tmp;
	}

	// assign top 128 values to hash bins
	int hashBinCnt[128];
	memset(hashBinCnt, 0, sizeof(int) * 128);

	for (int i = 0; i < 128; i += 1) {
		hashBinCnt[i] += pEntryList[i]->m_nameCnt;
		pEntryList[i]->m_bUsed = true;
		hashIdxTbl[(int)pEntryList[i]->m_c0][(int)pEntryList[i]->m_c1][(int)pEntryList[i]->m_c2] = i;
	}

	// Assign remainder of entries
	//   assign to hash bin with 2 of 3 char match and bin not full
	//   if no good bloom bit found then assign to hash bin least full
	for (int i = 128; i < 32 * 32 * 32; i += 1) {
		if (pEntryList[i]->m_nameCnt == 0)
			break;

		// search first two char match
		int c0, c1, c2;
		int nameCntMin = 0x7fffffff;
		int hashIdxMin = -1;

		// for each 3-char string, find a hash bin where
		//   two of the three characters match an existing entry
		c2 = pEntryList[i]->m_c2;
		for (c0 = 0; c0 < 32; c0 += 1) {
			for (c1 = 0; c1 < 32; c1 += 1) {
				CTable &entry = table[c0][c1][c2];

				if (!entry.m_bUsed) continue;

				int hashIdx = hashIdxTbl[c0][c1][c2];

				if (nameCntMin > hashBinCnt[hashIdx]) {
					nameCntMin = hashBinCnt[hashIdx];
					hashIdxMin = hashIdx;
				}
			}
		}

		if (hashIdxMin == -1 || hashBinCnt[hashIdxMin] > (int)(tot_num_names / 128)) {
			// either did not find a match, or all match bins were full
			for (int j = 0; j < 128; j += 1) {
				if (nameCntMin > hashBinCnt[j]) {
					nameCntMin = hashBinCnt[j];
					hashIdxMin = j;
				}
			}
		}

		// assign an entry to the selected hash bin
		pEntryList[i]->m_bUsed = true;
		hashIdxTbl[(int)pEntryList[i]->m_c0][(int)pEntryList[i]->m_c1][(int)pEntryList[i]->m_c2] = hashIdxMin;
		hashBinCnt[hashIdxMin] += pEntryList[i]->m_nameCnt;
	}

        pBloomList = (CBloom *)malloc((tot_num_names+1) * sizeof(CBloom));
	if(pBloomList == NULL){
		fprintf(stderr,"\nError, unable to alloc mem for Bloom filter array (%lu)\n\n",(tot_num_names+1));
		exit(1);
        }
        mem_curr += (tot_num_names+1) * sizeof(CBloom);
        mem_hiwater = MXMAX(mem_curr,mem_hiwater);

        memset(pBloomList, 0, tot_num_names * sizeof(CBloom));

        for(uint64_t i=0; i<tot_num_names; i++){
                unsigned char ch0 = (&lastnames[lastindxs[i]])[0];
                unsigned char ch1 = ch0 == '\0' ? '\0' : (&lastnames[lastindxs[i]])[1];
                unsigned char ch2 = ch1 == '\0' ? '\0' : (&lastnames[lastindxs[i]])[2];
                pBloomList[i].hashIdx = hashIdxTbl[ch0 & 0x1f][ch1 & 0x1f][ch2 & 0x1f];
        }

        return;
}

// --------------

void SetBloomList(uint64_t idx1, uint64_t idx2)
{
	unsigned char ch0 = (&lastnames[lastindxs[idx2]])[0];
	unsigned char ch1 = ch0 == '\0' ? '\0' : (&lastnames[lastindxs[idx2]])[1];
	unsigned char ch2 = ch1 == '\0' ? '\0' : (&lastnames[lastindxs[idx2]])[2];
	int hashIdx = hashIdxTbl[ch0 & 0x1f][ch1 & 0x1f][ch2 & 0x1f];

	pBloomList[idx1].m_mask[(hashIdx >> 3) & 0xf] |= 1 << (hashIdx & 0x7);

	return;
}

#endif // EXAMPLE
