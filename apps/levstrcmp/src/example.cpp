/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT levstrcmp application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include <string>
#include <vector>
#include <iostream>

#include "Ht.h"
#include "Main.h"

#ifdef EXAMPLE

FILE *hitFp = 0;

typedef vector<vector <int> > Tmatrix;

#define MAX_STR_CNT 90000
char strList[MAX_STR_CNT][32];
unsigned char coprocList[MAX_STR_CNT][16];

void InitStrings();
void InitStringsFromList(char **pNameList);
void InitStringsFromFile(const char *pFileName);
int CoprocEditD(int origLenL, int origLenR, char *strL, char *strR);
int WithinEditN_CP( unsigned char *l, int origlenL, unsigned char *r, int origlenR, int d, int filter_width );
uint levenshteinDistance(const string, const string);

union CStrPair {
  struct {
    uint64_t m_conIdL:16;
    uint64_t m_conIdxL:16;
    uint64_t m_conIdR:16;
    uint64_t m_conIdxR:16;
  };
  uint64_t m_data64;
};

#define MAX_CON_CNT	2048
#define CON_SIZE	(32*1024)
struct CContainer {
	CContainer *	m_pNextCon;
	int				m_strIdx;
	int				m_strId[CON_SIZE];
	char 			m_str[CON_SIZE][32];
	uint16_t		m_conId;
	uint64_t		m_align;	// force alignment to 64-bits
	char			m_coprocStr[CON_SIZE][16];
};

CContainer * pConHead[32] = { 0 };
CContainer * pConList[MAX_CON_CNT];
int conCnt;
int strCnt;
int maxConSize = 0;

uint8_t bCoprocMatch[MAX_STR_CNT][MAX_STR_CNT/8] = { { false } };	// check matrix

#define NUM_ARGS	9
#define NUM_CALLS	500

int example(int argc, char **argv)
{
	hitFp = fopen("hit.txt", "w");

	CAuHif *m_pAuHif = new CAuHif();

	int auCnt = m_pAuHif->unitCnt();

    InitStringsFromFile("./test/lastnames5k.inp");
    //InitStringsFromFile("./test/bdh.inp");

	//#define LN_STR_CNT 2000
	//InitStringsFromFile("../../ln.txt");

	printf("strCnt = %d\n", strCnt);
	printf("conCnt = %d\n", conCnt);
	printf("maxConSize = %d\n", maxConSize);

	int editDistance = 2;

	// create call list
	int			callCnt = 0;
	CAuHif::CCallArgs_htmain	call_argv[NUM_CALLS];
	uint64_t totalCmpCnt = 0;
	uint64_t accumCmpCnt[NUM_CALLS];

	for (int lenL = 0; lenL < 32; lenL += 1) {
        // Levenshtein is symmetric so only do diagonal and above
        for (int lenR = lenL; lenR < 32; lenR += 1) {

			if ( lenL > lenR + editDistance * 1 || lenR > lenL + editDistance * 1 )
				continue;

			if ( !pConHead[lenL] || pConHead[lenL]->m_strIdx <= 0 || !pConHead[lenR] || pConHead[lenR]->m_strIdx <= 0 )
				continue;

			int d = editDistance;
			int d2 = d;

#if 0
			// This is handled internal to the coproc personality ...

			// BEDENBAUGH <-> BREIDENBAUGH issue
			// where truncation changes the result
			// bump d enough to cover this case ...

			if( (lenL > COPROC_FILTER_SIZE) || (lenR > COPROC_FILTER_SIZE) ){
				int cl = MXMAX(0,(lenL - COPROC_FILTER_SIZE));
				int cr = MXMAX(0,(lenR - COPROC_FILTER_SIZE));
				d2 += MXMIN(d, abs(cl - cr));
			}
#endif

			for (CContainer *pConL = pConHead[lenL]; pConL; pConL = pConL->m_pNextCon) {
				for (CContainer *pConR = pConHead[lenR]; pConR; pConR = pConR->m_pNextCon) {

					//if (lenL != 7 || lenR != 11) continue;

					assert(callCnt < NUM_CALLS);

					call_argv[callCnt].m_strLenL = lenL;
					call_argv[callCnt].m_strLenR = lenR;
					call_argv[callCnt].m_mchAdjD = d2;
					call_argv[callCnt].m_conIdL = pConL->m_conId;
					call_argv[callCnt].m_conIdR = pConR->m_conId;
					call_argv[callCnt].m_conSizeL = pConL->m_strIdx;
					call_argv[callCnt].m_conSizeR = pConR->m_strIdx;
					call_argv[callCnt].m_conBaseL = &pConL->m_coprocStr[0][0];
					call_argv[callCnt].m_conBaseR = &pConR->m_coprocStr[0][0];

					totalCmpCnt += pConL->m_strIdx * pConR->m_strIdx;
					accumCmpCnt[callCnt] = totalCmpCnt;
					callCnt += 1;
				}
			}
		}
	}

	// add the flush call
	call_argv[callCnt].m_strLenL = 0;
	call_argv[callCnt].m_strLenR = 0;
	call_argv[callCnt].m_mchAdjD = 0;
	call_argv[callCnt].m_conIdL = 0;
	call_argv[callCnt].m_conIdR = 0;
	call_argv[callCnt].m_conSizeL = 0;
	call_argv[callCnt].m_conSizeR = 0;
	call_argv[callCnt].m_conBaseL = 0;
	call_argv[callCnt].m_conBaseR = 0;
	callCnt += 1;

#define _ONE_CALL
#ifdef ONE_CALL
	#define CNT_L CNT
	#define CNT_R CNT

	#define NUM_CALL 2
	uint64_t call_argv[NUM_CALL][NUM_ARGS] = {
		//	(strLenL, strLenR, mchAdjD, conIdL, conIdR, conSizeL, conSizeR, conBaseL, conBaseR)
		{ 10, 10, coprocEditD, 0,       0,      CNT_L,	CNT_R, (uint64_t)&coprocList[0],       (uint64_t)&coprocList[0] },
		{ 0, 0,   coprocEditD, 0,       0,      CNT_L,	CNT_R, (uint64_t)&coprocList[0],       (uint64_t)&coprocList[0] }
	};
	callCnt = 2;
#endif

	uint64_t popCnt[4*8];
	uint64_t rtnCnt[4*8];
	
	for (int au=0; au<auCnt; au++) {
		popCnt[au] = 0;
		rtnCnt[au] = 0;
	}

	int callNum = 0;
	int rtnNum = 0;
	uint64_t totalPopCnt = 0;
	uint64_t totalRtnCnt = 0;

	int *unitCallCnt = (int *)calloc(sizeof(int), auCnt);

	while (callNum < callCnt || callNum != rtnNum) {
		bool activity = false;
		for (int au=0; au<auCnt; au++) {
			// send call
			if (callNum < callCnt && unitCallCnt[au] < 3) {
				activity = true;
				m_pAuHif->AsyncCall_htmain(au, call_argv[callNum]);
				unitCallCnt[au] += 1;
				callNum += 1;
			}

			// process return
			uint64_t auRtnCnt;
			if (m_pAuHif->Return_htmain(au, auRtnCnt)) {
				totalRtnCnt += auRtnCnt - rtnCnt[au];
				rtnCnt[au] = auRtnCnt;

				activity = true;
				unitCallCnt[au] -= 1;
				rtnNum += 1;
				printf("Return %d of %d  %6.4f%%\n", rtnNum, callCnt, 100.0*accumCmpCnt[rtnNum-1]/totalCmpCnt);
				fflush(stdout);
			}

			// process results
			if (!m_pAuHif->IsEmpty(au)) {
				activity = true;
				while (!m_pAuHif->IsEmpty(au)) {
				    BlkQueDat d = m_pAuHif->Front(au);
				    m_pAuHif->Pop(au);

				    CStrPair p;
				    p.m_data64 = d.dat;

					// set bit in coproc matrix for each result returned.
					//   check for multiple responses for the same L/R strings

					int idxL = pConList[p.m_conIdL]->m_strId[p.m_conIdxL];
					int idxR = pConList[p.m_conIdR]->m_strId[p.m_conIdxR];

					if (hitFp && (bCoprocMatch[idxL][idxR/8] & (1 << (idxR&7))) != 0)
						fprintf(hitFp, "L=%d, R=%d  %s\n", idxL, idxR, (bCoprocMatch[idxL][idxR/8] & (1 << (idxR&7))) != 0 ? "###" : "");
					if (hitFp && (bCoprocMatch[idxL][idxR/8] & (1 << (idxR&7))) != 0)
						fflush(hitFp);

					//assert(!bCoprocMatch[idxL][idxR]);
					bCoprocMatch[idxL][idxR/8] |= 1 << (idxR&7);
                    // if left == right then right == left
                    bCoprocMatch[idxR][idxL/8] |= 1 << (idxL&7);

				    popCnt[au] += 1;
					totalPopCnt += 1;
				}
			}
		}

		// let HW do it's thing
		if (!activity)
		    HtSleep(100);
	}

	printf("Begin validation\n");
	fflush(stdout);

	//FILE *mchFp = fopen("coproc.txt", "w");

	// now that the coproc finished, check each L/R string pair
	int errorCnt = 0;
	int warningCnt = 0;
	for (int idxL = 0; idxL < strCnt; idxL += 1) {

		size_t lenL = strlen(strList[idxL]);
		//if (lenL != 7) continue;

 		for (int idxR = 0; idxR < strCnt; idxR += 1) {

			size_t lenR = strlen(strList[idxR]);
			//if (lenR != 11) continue;
         
			size_t d = editDistance;

			if ( lenL > lenR + editDistance * 1 || lenR > lenL + editDistance * 1 )
				continue;

			//bool bMatch = idxL != idxR && WithinEditN_CP( coprocList[idxL], (int)lenL, coprocList[idxR], (int)lenR, (int)d, 10 );

			bool bFullMatch = WithinEditN_CP( (unsigned char *)strList[idxL], (int)lenL, (unsigned char *)strList[idxR], (int)lenR, (int)d, 30 );

			if (bFullMatch && (bCoprocMatch[idxL][idxR/8] & (1 << (idxR&7))) == 0 && idxL != idxR) {
				errorCnt += 1;
				if (hitFp) fprintf(hitFp, "# Error -> hostFull=%s, coproc=%s : %dL %s , %dR %s\n",
					bFullMatch ? "t" : "f",
					(bCoprocMatch[idxL][idxR/8] & (1 << (idxR&7))) != 0 ? "t" : "f",
					idxL, strList[idxL], idxR, strList[idxR] );
			} else if (!bFullMatch && (bCoprocMatch[idxL][idxR/8] & (1 << (idxR&7))) != 0) {
				warningCnt += 1;
				if (hitFp && warningCnt < 100) fprintf(hitFp, "Warning -> hostFull=%s, coproc=%s : %dL %s , %dR %s\n",
					bFullMatch ? "t" : "f",
					(bCoprocMatch[idxL][idxR/8] & (1 << (idxR&7))) != 0 ? "t" : "f",
					idxL, strList[idxL], idxR, strList[idxR] );

				//for(;;) {
				//	WithinEditN_CP( (unsigned char *)strList[idxL], (int)lenL, (unsigned char *)strList[idxR], (int)lenR, (int)d, 30 );
				//} 
			}

			//if (bMatch && idxL != idxR)
			//	fprintf(mchFp, "%d, %d\n", idxL, idxR);
		}
	}

	for (int au=0; au<auCnt; au++) {
		if (hitFp) fprintf(hitFp, "AU%d: %ld/%ld\n", au, popCnt[au], rtnCnt[au]);
	}

	printf("Warning/Errors/PopCnt/RtnCnt = %d/%d/%lld/%lld\n", warningCnt, errorCnt, (long long)totalPopCnt, (long long)totalRtnCnt);

	if (hitFp) fprintf(hitFp, "Warning/Errors = %d/%d\n", warningCnt, errorCnt);
	if (hitFp) fclose(hitFp);

	delete m_pAuHif;
}

MTRand_int32 g_rndNum;

#define MAP_INPUT

void InitStringsFromFile(const char *pFileName)
{
	// load containers from file

	FILE *fp;
	if (!(fp = fopen(pFileName, "r"))) {
		printf("Could not open file\n");
		exit(1);
	}

	uint8_t	chMap[256] = { 0 };
	uint8_t chMapIdx = 1;

	int strId = 0;
	int conId = 0;
	char line[128];
	while (fgets(line, 128, fp)) {
		// convert to upper case
		char *pCh = line;
		while (*pCh != '\n' && *pCh != '\r' && *pCh != '\0') {
			if (islower(*pCh))
				*pCh = *pCh - 'a' + 'A';

			pCh += 1;
		}
		*pCh = '\0';

		size_t len = pCh - line;
		assert(len < 31);

#ifdef LN_STR_CNT
		//if (len != 9)
		//	continue;
		int rndNum = g_rndNum() % 45000000;

		if (rndNum > LN_STR_CNT)
			continue;
#endif

		// get container
		if (pConHead[len] == 0) {
			pConHead[len] = new CContainer;
			pConHead[len]->m_conId = conId;
			pConHead[len]->m_pNextCon = 0;
			pConHead[len]->m_strIdx = 0;

			pConList[conId++] = pConHead[len];
		}
		CContainer *pCon = pConHead[len];

		if (pCon->m_strIdx == CON_SIZE) {
			// need new container
			pConHead[len] = new CContainer;
			pConHead[len]->m_conId = conId;
			pConHead[len]->m_pNextCon = pCon;
			pConHead[len]->m_strIdx = 0;

			pConList[conId++] = pConHead[len];

			pCon = pConHead[len];
		}

		if (strId >= MAX_STR_CNT) {
			printf("Exceeded MAX_STR_CNT (%d)\n", MAX_STR_CNT);
			exit(1);
		}

		strcpy(strList[strId], line);
		pCon->m_strId[pCon->m_strIdx] = strId;
		strcpy(pCon->m_str[pCon->m_strIdx], line);

		strncpy(pCon->m_coprocStr[pCon->m_strIdx], pCon->m_str[pCon->m_strIdx], 12);
		size_t minLen = len < 10 ? len : 10;
		for (size_t j = minLen; j < 16; j += 1)
			pCon->m_coprocStr[pCon->m_strIdx][j] = ' ';

#ifdef MAP_INPUT
		for (size_t j = 0; j < 16; j += 1) {
			int ch = pCon->m_coprocStr[pCon->m_strIdx][j];
			if (chMap[ch] == 0) {
				assert(chMapIdx < 64);
				chMap[ch] = chMapIdx++;
			}

			pCon->m_coprocStr[pCon->m_strIdx][j] = chMap[ch];
		}
#endif

		memcpy(coprocList[strId], pCon->m_coprocStr[pCon->m_strIdx], 16);


		strId++;
		pCon->m_strIdx++;

		if (maxConSize < pCon->m_strIdx)
			maxConSize = pCon->m_strIdx;

		if (pCon->m_strIdx >= MAX_STR_CNT)
			break;
	}

	assert(conId <= MAX_CON_CNT);

	conCnt = conId;
	strCnt = strId;
}

int CoprocEditD(int origLenL, int origLenR, char *chL, char *chR) {
   string strL, strR;
   // convert to a string with a max length of COPROC_STR_LEN
   strL.assign(chL,STR_CMP_LEN);
   strR.assign(chR,STR_CMP_LEN);
   return (int)levenshteinDistance(strL, strR);
}

int WithinEditN_CP( unsigned char *l, int origlenL, unsigned char *r, int origlenR, int d, int filter_width )
{
   int lenL = min(origlenL, filter_width);
   int lenR = min(origlenR, filter_width);
 
   string strL, strR;
   strL.assign((char *)l, lenL);
   strR.assign((char *)r, lenR);
 
   return (int)levenshteinDistance(strL, strR) <= d;
}


uint levenshteinDistance(const string source, const string target) {
  /*
    1	Set n to be the source string with a length equal to s
    Set m to be the target string with a length equal to t
    If n = 0, return m.
    If m = 0, return n.
    Initialize a matrix with 0..m rows and 0..n columns.
    2	Let the first row elements be equal to 0,1,,n (in order)
    Similarly assign the first column to 0,1,,m
    3	Loop through each character of s (i from 1 to n).
    4	In a sub-loop, iterate each character of t (j from 1 to m).
    5	If s[i] and t[j] are equal, the cost is 0.
    If s[i] doesn't equate to t[j], the cost is 1.
    6	Assign to cell d[i,j] of the matrix to be equal to the minimum of:
    a. The value of field immediately above, plus 1: d[i-1,j] + 1.
    b. The value of field immediately to the left plus 1: d[i,j-1] + 1.
    c. The value of field diagonally above and to the left plus the cost: d[i-1,j-1] + cost.
    7	After the processing steps (3, 4, 5, 6) are complete, the distance is that stored in the cell d[n,m] in the end.
  */
  const int m = target.length();
  const int n = source.length();
  if (n == 0) {
    return m;
  }
  if (m == 0) {
    return n;
  }
  Tmatrix matrix(n+1); //the matrix

  // resize the rows in the 2nd dimension. Unfortunately C++ doesn't
  // allow the allocation on declaration of 2nd dimension of the matrix directly

  for (int i = 0; i <= n; i++) {
    matrix[i].resize(m+1);
  }

  // Step 2

  for (int i = 0; i <= n; i++) {
    matrix[i][0]=i;
  }

  for (int j = 0; j <= m; j++) {
    matrix[0][j]=j;
  }

  // Step 3

  for (int i = 1; i <= n; i++) {

    const char s_i = source[i-1];

    // Step 4

    for (int j = 1; j <= m; j++) {

      const char t_j = target[j-1];

      // Step 5

      int cost;
      if (s_i == t_j) {
	cost = 0;
      }
      else {
	cost = 1;
      }

      // Step 6

      const int diag = matrix[i-1][j-1];
      const int left = matrix[i][j-1];
      const int above = matrix[i-1][j];
      matrix[i][j] = min( above + 1, min(left + 1, diag + cost));

    }
  }

  // The final step

  return matrix[n][m];
}

#endif // EXAMPLE
