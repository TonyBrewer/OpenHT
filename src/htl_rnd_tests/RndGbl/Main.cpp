/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include <stdio.h>
#include <vector>
#include <string>
#include <assert.h>
#include <stdint.h>
#include "time.h"

#include "MtRand.h"

using namespace std;

#ifdef WIN32
#include <windows.h>
#include <WinDef.h>
#include <direct.h>	// _mkdir
#else
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#define _mkdir(a) mkdir(a, 0777)
#endif

const char * pWorkingDir = "C:/Ht/src/htl_rnd_tests/RndGbl/TestDir";

struct CRndFld {
	CRndFld() {}

	string m_name;
	string m_type;
	int m_width;
	int m_rdCnt;
	int m_wrCnt;
};

struct CRndModFld {
	CRndFld * m_pRndFld;
	int m_wrId;
	int m_mifRdId;
};

struct CRndMod {
	vector<CRndModFld> m_srcRdFldList;
	vector<CRndModFld> m_mifRdFldList;
	vector<CRndModFld> m_srcWrFldList;
	vector<CRndModFld> m_mifWrFldList;
	int m_groupW;
	bool m_bRdMemMaxBw;
	bool m_bWrMemMaxBw;
	int m_memQueW;
	int m_callQueW;
	bool m_bClk2x;
};

enum ECtlPhase { CtlRd, CtlWr, CtlRtn };
enum EIdxMode { eMifAddr, eMifIdx, eSrcIdx, eGblShIdx };
enum ECpType { eHc1, eHc1ex, eHc2, eHc2ex, eWx690, eWx2000 };
enum EVarType { eVarPrivate, eVarShared, eVarGlobalPr, eVarGlobalSh };

#define CP_TYPE_COUNT 6
#define VAR_TYPE_COUNT 4

class CDesign {
public:
	CDesign(int seed) : m_seed(seed) {}
	void MakeDirs();
	void GenHtlFile();
	void GenHtdFile();
	void GenCtlSrcFile();
	void GenModSrcFiles();
	void GenMain();

	string FindNextCtlInstr(ECtlPhase ctlPhase, size_t modIdx);
	string GenVarIdxStr(FILE * fp, int wrCmdIdx, int wrReqIdx, EIdxMode idxMode, bool bVarGlobalSh);
	void GenSharedVarIdxStr(FILE * fp, int wrCmdIdx, int wrReqIdx, EIdxMode idxMode, string & varAddrStr, string & varIdxStr2);

public:
	int m_seed;

	int m_var1stIdx;
	int m_var2ndIdx;
	int m_varAddr1W;
	int m_varAddr2W;
	int m_varDimen1;
	int m_varDimen2;
	int m_fldDimen1;
	int m_fldDimen2;

	ECpType m_cpType;
	EVarType m_varType;
	int m_ctlGroupW;
	bool m_bPerfMon;

	vector<CRndFld> m_rndFldList;
	vector<CRndMod> m_rndModList;
};

class CWrFldIter {
public:
	CWrFldIter(CDesign *pDsn, CRndFld & rndFld) : m_pDsn(pDsn), m_rndFld(rndFld) {
		m_modIdx = 0;
		m_wrFldIdx = -1;
		m_bSrcWr = true;
	}

	bool operator++ (int) {
		m_wrFldIdx += 1;
		for (;;) {
			if (m_bSrcWr) {
				if (m_modIdx == (int)m_pDsn->m_rndModList.size()) {
					m_bSrcWr = false;
					m_wrFldIdx = 0;
					m_modIdx = 0;
					continue;
				}
				if (m_wrFldIdx == (int)m_pDsn->m_rndModList[m_modIdx].m_srcWrFldList.size()) {
					m_modIdx += 1;
					m_wrFldIdx = 0;
					continue;
				}
				if (m_pDsn->m_rndModList[m_modIdx].m_srcWrFldList[m_wrFldIdx].m_pRndFld == &m_rndFld) {
					m_pRndModFld = &m_pDsn->m_rndModList[m_modIdx].m_srcWrFldList[m_wrFldIdx];
					return true;
				}
			} else {
				if (m_modIdx ==(int) m_pDsn->m_rndModList.size()) {
					return false; // end
				}
				if (m_wrFldIdx == (int)m_pDsn->m_rndModList[m_modIdx].m_mifWrFldList.size()) {
					m_modIdx += 1;
					m_wrFldIdx = 0;
					continue;
				}
				if (m_pDsn->m_rndModList[m_modIdx].m_mifWrFldList[m_wrFldIdx].m_pRndFld == &m_rndFld) {
					m_pRndModFld = &m_pDsn->m_rndModList[m_modIdx].m_mifWrFldList[m_wrFldIdx];
					return true;
				}
			}
		}
	}

	int m_modIdx;
	bool m_bSrcWr;
	int m_wrFldIdx;
	CRndModFld * m_pRndModFld;

	CDesign * m_pDsn;
	CRndFld & m_rndFld;
};

static inline uint64_t getUsec()
{
#ifdef _WIN32
#define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL

	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);

	uint64_t usec = ft.dwHighDateTime;
	usec <<= 32;
	usec |= ft.dwLowDateTime;

	/*converting file time to unix epoch*/
	usec /= 10; /*convert into microseconds*/
	usec -= DELTA_EPOCH_IN_MICROSECS;
#else
	struct timeval st;
	gettimeofday(&st, NULL);
	uint64_t usec = st.tv_sec * 1000000ULL + (uint64_t)st.tv_usec;
#endif
	return usec;
}

void Usage() {
	printf("\nUsage: RndGbl [options]\n");
	printf("\n");
	printf("Options:\n");
	printf("  -help, -h        Prints this usage message\n");
	printf("  -seed <num>      Seed\n");
}

int main(int argc, char ** argv)
{
#ifdef _WIN32
	int seed = (int)getUsec() & 0x7fffffff;
#else
	int seed = (int)time(NULL);
#endif

	int argPos;
	for (argPos=1; argPos < argc; argPos++) {
		if (argv[argPos][0] == '-') {
			if ((strcmp(argv[argPos], "-h") == 0) ||
				(strcmp(argv[argPos], "-help") == 0)) {
				Usage();
				exit(0);
			} else if ((strcmp(argv[argPos], "-seed") == 0)) {
				argPos += 1;
				seed = atoi(argv[argPos]);
			} else {
				printf("Unknown command line switch: %s\n", argv[argPos]);
				Usage();
				exit(1);
			}
		} else
			break;
	}

	printf("Random seed: %d\n", seed);
	MTRand_int32 rnd32(seed);

	CDesign dsn(seed);

#ifdef WIN32
	if (SetCurrentDirectory(pWorkingDir) == 0) {
		printf("Unable to set working directory (Error=%d)\n", GetLastError());
		exit(1);
	}
#endif

	// choose random test parameters
	dsn.m_cpType = ECpType(rnd32() % CP_TYPE_COUNT);
	dsn.m_bPerfMon = (rnd32() & 1) == 1;

	int rndType = rnd32() % VAR_TYPE_COUNT;
	dsn.m_varType = EVarType(rndType);
	//dsn.m_varType = eVarGlobalSh;

	switch (dsn.m_varType) {
	case eVarShared: printf("Shared\n"); break;
	case eVarPrivate: printf("Private\n"); break;
	case eVarGlobalPr: printf("GlobalPr\n"); break;
	case eVarGlobalSh: printf("GlobalSh\n"); break;
	}

	dsn.m_varAddr1W = 0;
	dsn.m_varAddr2W = 0;
	dsn.m_varDimen1 = 0;
	dsn.m_varDimen2 = 0;
	dsn.m_fldDimen1 = 0;
	dsn.m_fldDimen2 = 0;

	int idxTypeCnt;
	switch (dsn.m_varType) {
	case eVarPrivate: idxTypeCnt = 2;  break;
	case eVarShared: idxTypeCnt = 2; break;
	case eVarGlobalPr: idxTypeCnt = 3; break;
	case eVarGlobalSh: idxTypeCnt = 3; break;
	default: assert(0);
	}

	// choose one or two indexes
	dsn.m_var1stIdx = rnd32() % idxTypeCnt;
	dsn.m_var2ndIdx = rnd32() % idxTypeCnt;

	int varAddr1W = (rnd32() % 1) == 0 ? 4 : 10;	// Dist Ram or BRAM

	switch (dsn.m_var1stIdx) {
	case 0: dsn.m_varAddr1W = varAddr1W; break;
	case 1: dsn.m_varDimen1 = 9; break;
	case 2: dsn.m_fldDimen1 = 10; break;
	default: assert(0);
	}
		
	switch (dsn.m_var2ndIdx) {
	case 0:
		if (dsn.m_var1stIdx == 0) {
			dsn.m_var1stIdx = 3;
			dsn.m_var2ndIdx = 0;
			dsn.m_varAddr2W = 3;
		} else
			dsn.m_varAddr1W = varAddr1W;
		break;
	case 1:
		if (dsn.m_var1stIdx == 1) {
			dsn.m_var1stIdx = 4;
			dsn.m_var2ndIdx = 1;
			dsn.m_varDimen2 = 6;
		} else
			dsn.m_varDimen1 = 9;
		break;
	case 2:
		if (dsn.m_var1stIdx == 2) {
			dsn.m_var1stIdx = 5;
			dsn.m_var2ndIdx = 2;
			dsn.m_fldDimen2 = 5;
		} else
			dsn.m_fldDimen1 = 10;
		break;
	default:
		assert(0);
	}

	// pick field count
	int fldCnt = rnd32() % 1 + 1;

	for (int i = 0; i < fldCnt; i += 1) {

		char nameBuf[32];
		switch (dsn.m_varType) {
		case eVarPrivate: sprintf(nameBuf, "pvar%d", i); break;
		case eVarShared: sprintf(nameBuf, "svar%d", i); break;
		case eVarGlobalPr: sprintf(nameBuf, "data%d", i); break;
		case eVarGlobalSh: sprintf(nameBuf, "data%d", i); break;
		default: assert(0);
		}

		string typeName = "uint64_t";
		int typeWidth = 64;

		//switch (rnd32() % 3) {
		//case 0: 
		//	typeName = "char";
		//	typeWidth = 8;
		//	break;
		//case 1:
		//	typeName = "uint16_t";
		//	typeWidth = 16;
		//	break;
		//case 2: 
		//	{
		//		typeWidth = rnd32() % 32 + 1;
		//		char typeBuf[32];
		//		sprintf(typeBuf, "ht_uint%d", typeWidth);
		//		typeName = typeBuf;
		//		break;
		//	}
		//	break;
		//case 3:
		//	{
		//		// struct/union
		//		assert(0);
		//	}
		//	break;
		//default:
		//	assert(0);
		//}

		dsn.m_rndFldList.push_back(CRndFld());
		CRndFld & rndFld = dsn.m_rndFldList.back();

		rndFld.m_name = nameBuf;
		rndFld.m_type = typeName;
		rndFld.m_width = typeWidth;

		switch (dsn.m_varType) {
		case eVarShared:
			rndFld.m_rdCnt = 1;
			rndFld.m_wrCnt = 1;
			break;
		case eVarGlobalPr:
		case eVarGlobalSh:
			rndFld.m_rdCnt = rnd32() % 4 + 1;
			rndFld.m_wrCnt = rnd32() % 2 + 1;
			break;
		case eVarPrivate:
			rndFld.m_rdCnt = rnd32() % 2 + 1;
			rndFld.m_wrCnt = rnd32() % 2 + 1;
			break;
		default:
			assert(0);
		}

		// select modules for field read
		int mifRdId = 0;
		for (int rd = 0; rd < rndFld.m_rdCnt; rd += 1) {
			size_t rrCnt = 0;
			if (dsn.m_rndModList.size() > 0) {
				float rrWeight = 1.0f / dsn.m_rndModList.size() * 33;
				size_t rrIdx = rnd32() % dsn.m_rndModList.size();
				for (rrCnt = 0; rrCnt < dsn.m_rndModList.size(); rrCnt += 1) {
					CRndMod & rndMod = dsn.m_rndModList[rrIdx];

					if (rrIdx == 0 && dsn.m_varType == eVarGlobalSh)
						continue;

					if (rndMod.m_bClk2x && (rndMod.m_mifRdFldList.size() > 0 || rndMod.m_srcRdFldList.size() > 0))
						continue;

					if (dsn.m_varType == eVarShared || dsn.m_varType == eVarPrivate || rnd32() % 100 < rrWeight) {
						if (rrIdx == 0 && rnd32() % 2 == 0 && (rndMod.m_mifRdFldList.size() == 0 || rndMod.m_mifRdFldList.back().m_pRndFld != &rndFld) ||
							(dsn.m_varType == eVarPrivate && rndMod.m_srcRdFldList.size() > 0))
						{
							rndMod.m_mifRdFldList.push_back(CRndModFld());
							rndMod.m_mifRdFldList.back().m_pRndFld = &rndFld;
							rndMod.m_mifRdFldList.back().m_mifRdId = mifRdId++;
							break;
						} else if (rndMod.m_srcRdFldList.size() == 0 || rndMod.m_srcRdFldList.back().m_pRndFld != &rndFld) {
							rndMod.m_srcRdFldList.push_back(CRndModFld());
							rndMod.m_srcRdFldList.back().m_pRndFld = &rndFld;
							break;
						}
					}

					rrIdx = (rrIdx + 1) % dsn.m_rndModList.size();
				}
			}

			if (rrCnt == dsn.m_rndModList.size()) {
				dsn.m_rndModList.push_back(CRndMod());
				CRndMod & rndMod = dsn.m_rndModList.back();

				if (rndFld.m_rdCnt - rd >= 2) {
					switch (dsn.m_varType) {
					case eVarShared:
						rndMod.m_bClk2x = (rnd32() & 1) == 1;
						break;
					case eVarPrivate:
						rndMod.m_bClk2x = (rnd32() & 3) == 3;
						if (rndMod.m_bClk2x)
							rndFld.m_wrCnt = 2;
						break;
					case eVarGlobalPr:
					case eVarGlobalSh:
						rndMod.m_bClk2x = (rnd32() & 3) == 3 && rndFld.m_wrCnt == 1;
						break;
					default:
						assert(0);
					}
				} else
					rndMod.m_bClk2x = false;

				if (dsn.m_rndModList.size() == 1 && rnd32() % 2 == 0 && (dsn.m_varType != eVarPrivate || !rndMod.m_bClk2x)) {
					rndMod.m_mifRdFldList.push_back(CRndModFld());
					rndMod.m_mifRdFldList.back().m_pRndFld = &rndFld;
					rndMod.m_mifRdFldList.back().m_mifRdId = mifRdId++;
				} else {
					rndMod.m_srcRdFldList.push_back(CRndModFld());
					rndMod.m_srcRdFldList.back().m_pRndFld = &rndFld;
				}

				if (rndMod.m_bClk2x)
					rd += 1;
			}
		}

		// select modules for field write
		for (int wrId = 0; wrId < rndFld.m_wrCnt; wrId += 1) {
			size_t rrCnt = 0;
			if (dsn.m_rndModList.size() > 0) {
				float rrWeight = 1.0f / dsn.m_rndModList.size() * 33;
				size_t rrIdx = dsn.m_rndModList.size() == 0 ? 0 : (rnd32() % dsn.m_rndModList.size());
				for (rrCnt = 0; rrCnt < dsn.m_rndModList.size(); rrCnt += 1) {
					CRndMod & rndMod = dsn.m_rndModList[rrIdx];

					if (rndMod.m_bClk2x && rndFld.m_wrCnt - wrId < 2)
						continue;

					if (dsn.m_varType == eVarPrivate || dsn.m_varType == eVarShared || rnd32() % 100 < rrWeight
						&& (rrIdx == 0 // extern=true can only write src or mif (not both)
						|| (rndMod.m_srcWrFldList.size() == 0 || rndMod.m_srcWrFldList.back().m_pRndFld != &rndFld)
						&& (rndMod.m_mifWrFldList.size() == 0 || rndMod.m_mifWrFldList.back().m_pRndFld != &rndFld)))
					{
						if ((rnd32() % 2 == 0 && (rndMod.m_mifWrFldList.size() == 0 || rndMod.m_mifWrFldList.back().m_pRndFld != &rndFld) ||
							(dsn.m_varType == eVarPrivate && rndMod.m_srcWrFldList.size() > 0)) && (dsn.m_varType != eVarPrivate || !rndMod.m_bClk2x))
						{
							rndMod.m_mifWrFldList.push_back(CRndModFld());
							rndMod.m_mifWrFldList.back().m_pRndFld = &rndFld;
							rndMod.m_mifWrFldList.back().m_wrId = wrId;

							if (rndMod.m_bClk2x)
								wrId += 1;
							break;
						} else if (rndMod.m_srcWrFldList.size() == 0 || rndMod.m_srcWrFldList.back().m_pRndFld != &rndFld) {
							rndMod.m_srcWrFldList.push_back(CRndModFld());
							rndMod.m_srcWrFldList.back().m_pRndFld = &rndFld;
							rndMod.m_srcWrFldList.back().m_wrId = wrId;

							if (rndMod.m_bClk2x)
								wrId += 1;
							break;
						}

					}

					rrIdx = (rrIdx + 1) % dsn.m_rndModList.size();
				}
			}

			if (rrCnt == dsn.m_rndModList.size()) {
				dsn.m_rndModList.push_back(CRndMod());
				CRndMod & rndMod = dsn.m_rndModList.back();

				if (rndFld.m_wrCnt - wrId == 2) {
					switch (dsn.m_varType) {
					case eVarShared:
						rndMod.m_bClk2x = (rnd32() & 1) == 1;
						break;
					case eVarPrivate:
						rndMod.m_bClk2x = (rnd32() & 3) == 3;// && rndFld.m_rdCnt == 1 && rndFld.m_wrCnt == 1;
						break;
					case eVarGlobalPr:
					case eVarGlobalSh:
						rndMod.m_bClk2x = (rnd32() & 3) == 3 && rndFld.m_wrCnt == 1;
						break;
					default:
						assert(0);
					}
				} else
					rndMod.m_bClk2x = false;

				if (rnd32() % 2 == 0) {
					rndMod.m_mifWrFldList.push_back(CRndModFld());
					rndMod.m_mifWrFldList.back().m_pRndFld = &rndFld;
					rndMod.m_mifWrFldList.back().m_wrId = wrId;
				} else {
					rndMod.m_srcWrFldList.push_back(CRndModFld());
					rndMod.m_srcWrFldList.back().m_pRndFld = &rndFld;
					rndMod.m_srcWrFldList.back().m_wrId = wrId;
				}
			}
		}

		printf("rdCnt = %d, wrCnt = %d\n", rndFld.m_rdCnt, rndFld.m_wrCnt);
	}

	dsn.m_ctlGroupW = rnd32() % 5;

	for (size_t modIdx = 0; modIdx < dsn.m_rndModList.size(); modIdx += 1) {
		CRndMod & rndMod = dsn.m_rndModList[modIdx];

		if (modIdx == 0 && dsn.m_varType == eVarGlobalSh)
			rndMod.m_groupW = 1;
		else
			rndMod.m_groupW = rnd32() % 5;

		rndMod.m_bRdMemMaxBw = (rnd32() & 1) == 1;
		rndMod.m_bWrMemMaxBw = (rnd32() & 1) == 1;
		rndMod.m_memQueW = 4 + rnd32() % 2;
		rndMod.m_callQueW = rnd32() % 5;
	}

	// make directory structure
	dsn.MakeDirs();

	// generate htl file
	dsn.GenHtlFile();

	// generate htd file
	dsn.GenHtdFile();

	// generate ctl module src file
	dsn.GenCtlSrcFile();

	// generate mod mdule src files
	dsn.GenModSrcFiles();

	// generate main
	dsn.GenMain();

	return 0;
}

void CDesign::MakeDirs()
{
	_mkdir("src");
	_mkdir("src_pers");
}

void CDesign::GenHtlFile()
{
	FILE *fp;
	if (!(fp = fopen("src_pers/RndTest.htl", "w"))) {
		fprintf(stderr, "Unable to open file 'RndTest.htl'\n");
		exit(1);
	}

	fprintf(fp, "// -cl \"%s/src_pers/RndTest.htl\"\n", pWorkingDir);

	char const * pCpTypeStr;
	switch (m_cpType) {
	case eHc1: pCpTypeStr = "hc-1"; break;
	case eHc1ex: pCpTypeStr = "hc-1ex"; break;
	case eHc2: pCpTypeStr = "hc-2"; break;
	case eHc2ex: pCpTypeStr = "hc-2ex"; break;
	case eWx690: pCpTypeStr = "wx-690"; break;
	case eWx2000: pCpTypeStr = "wx-2000"; break;
	default: assert(0);
	}

	fprintf(fp, "-cp %s\n", pCpTypeStr);
	fprintf(fp, "-uc 1			// unit count\n");
	fprintf(fp, "-un Au			// unit name\n");
	fprintf(fp, "\n");
	fprintf(fp, "-wd \"%s/msvs10/\"		// working directory\n", pWorkingDir);
	fprintf(fp, "\"%s/src_pers/RndTest.htd\"	    // design file location\n", pWorkingDir);
	fprintf(fp, "../ht/sysc							// location for output files\n");

	fclose(fp);
}

void CDesign::GenHtdFile()
{
	FILE *fp;
	if (!(fp = fopen("src_pers/RndTest.htd", "w"))) {
		fprintf(stderr, "Unable to open file 'RndTest.htd'\n");
		exit(1);
	}

	fprintf(fp, "// seed: %d\n", m_seed);
	fprintf(fp, "\n");

	fprintf(fp, "\t// Module ctl\n");
	fprintf(fp, "\n");
	fprintf(fp, "\tdsnInfo.AddModule(name=ctl, htIdW=%d);\n", m_ctlGroupW);
	fprintf(fp, "\n");

	int firstModIdx = -1;
	for (size_t modIdx = 0; modIdx < m_rndModList.size(); modIdx += 1) {
		CRndMod & rndMod = m_rndModList[modIdx];

		if (rndMod.m_srcWrFldList.size() > 0 || rndMod.m_mifWrFldList.size() > 0) {
			fprintf(fp, "\tctl.AddInst(name=CTL_WR_MOD%d);\n", (int)modIdx+1);
			if (firstModIdx < 0)
				firstModIdx = (int)modIdx;
		}
	}

	if (m_varType != eVarPrivate) {
		for (size_t modIdx = 0; modIdx < m_rndModList.size(); modIdx += 1) {
			CRndMod & rndMod = m_rndModList[modIdx];

			if (rndMod.m_srcRdFldList.size() > 0 || rndMod.m_mifRdFldList.size() > 0)
				fprintf(fp, "\tctl.AddInst(name=CTL_RD_MOD%d);\n", (int)modIdx+1);
		}
	}
	fprintf(fp, "\tctl.AddInst(name=CTL_RTN);\n");

	fprintf(fp, "\n");
	fprintf(fp, "\tctl.AddEntry(func=htmain, inst=CTL_WR_MOD%d, host=true)\n", firstModIdx+1);
	fprintf(fp, "\t\t.AddParam(hostType=uint64_t, type=ht_uint48, name=memAddrIn)\n");
	fprintf(fp, "\t\t.AddParam(hostType=uint64_t, type=ht_uint48, name=memAddrOut)\n");
	fprintf(fp, "\t\t;\n");
	fprintf(fp, "\n");
	fprintf(fp, "\tctl.AddReturn(func=htmain)\n");
	fprintf(fp, "\t\t.AddParam(type=short, name=err)\n");
	fprintf(fp, "\t\t;\n");
	fprintf(fp, "\n");
	fprintf(fp, "\tctl.AddPrivate()\n");
	fprintf(fp, "\t\t.AddVar(type=ht_uint48, name=memAddrIn)\n");
	fprintf(fp, "\t\t.AddVar(type=ht_uint48, name=memAddrOut)\n");
	fprintf(fp, "\t\t.AddVar(type=short, name=err)\n");
	fprintf(fp, "\t\t.AddVar(type=short, name=errRtn)\n");
	fprintf(fp, "\t\t;\n");
	fprintf(fp, "\n");

	for (size_t modIdx = 0; modIdx < m_rndModList.size(); modIdx += 1) {
		CRndMod & rndMod = m_rndModList[modIdx];

		if (rndMod.m_srcWrFldList.size() > 0 || rndMod.m_mifWrFldList.size() > 0)
			fprintf(fp, "\tctl.AddCall( func=Mod%dWr, queueW=%d );\n", (int)modIdx+1, rndMod.m_callQueW);

		if (m_varType != eVarPrivate) {
			if (rndMod.m_srcRdFldList.size() > 0 || rndMod.m_mifRdFldList.size() > 0)
				fprintf(fp, "\tctl.AddCall( func=Mod%dRd, queueW=%d );\n", (int)modIdx+1, rndMod.m_callQueW);
		}
	}
	fprintf(fp, "\n");

	for (size_t modIdx = 0; modIdx < m_rndModList.size(); modIdx += 1) {
		CRndMod & rndMod = m_rndModList[modIdx];

		fprintf(fp, "\t// Module mod%d\n", (int)modIdx+1);
		fprintf(fp, "\n");
		fprintf(fp, "\tdsnInfo.AddModule(name=mod%d, htIdW=%d, clock=%s);\n",
			(int)modIdx+1, rndMod.m_groupW, rndMod.m_bClk2x ? "2x" : "1x");
		fprintf(fp, "\n");

		int wrIdx = 1;
		if (rndMod.m_srcWrFldList.size() > 0) {
			for (int i = 0; i < 3; i += 1)
				fprintf(fp, "\tmod%d.AddInst( name=MOD%d_WR%d );\n", (int)modIdx+1, (int)modIdx+1, wrIdx++);
		}
		if (rndMod.m_mifWrFldList.size() > 0) {
			fprintf(fp, "\tmod%d.AddInst( name=MOD%d_WR%d );\n", (int)modIdx+1, (int)modIdx+1, wrIdx++);
		}
		if (m_varType != eVarPrivate) {
			if (rndMod.m_srcWrFldList.size() > 0 || rndMod.m_mifWrFldList.size() > 0) {
				fprintf(fp, "\tmod%d.AddInst( name=MOD%d_WR_RTN );\n", (int)modIdx+1, (int)modIdx+1);
			}
		}

		int rdIdx = 1;
		for (size_t fldIdx = 0; fldIdx < rndMod.m_srcRdFldList.size(); fldIdx += 1) {
			CRndFld & rndFld = *rndMod.m_srcRdFldList[fldIdx].m_pRndFld;

			for (int i = 0; i < 3 * rndFld.m_wrCnt; i += 1)
				fprintf(fp, "\tmod%d.AddInst( name=MOD%d_RD%d );\n", (int)modIdx+1, (int)modIdx+1, rdIdx++);
		}
		for (size_t fldIdx = 0; fldIdx < rndMod.m_mifRdFldList.size(); fldIdx += 1) {
			CRndFld & rndFld = *rndMod.m_mifRdFldList[fldIdx].m_pRndFld;

			for (int i = 0; i < 1 * rndFld.m_wrCnt; i += 1)
				fprintf(fp, "\tmod%d.AddInst( name=MOD%d_RD%d );\n", (int)modIdx+1, (int)modIdx+1, rdIdx++);
		}
		if (rndMod.m_srcRdFldList.size() > 0 || rndMod.m_mifRdFldList.size() > 0) {
			fprintf(fp, "\tmod%d.AddInst( name=MOD%d_RD_RTN );\n", (int)modIdx+1, (int)modIdx+1);
		}
		fprintf(fp, "\n");

		if (rndMod.m_srcWrFldList.size() > 0 || rndMod.m_mifWrFldList.size() > 0) {
			fprintf(fp, "\tmod%d.AddEntry(func=Mod%dWr, inst=MOD%d_WR1)\n", (int)modIdx+1, (int)modIdx+1, (int)modIdx+1);
			fprintf(fp, "\t\t.AddParam(hostType=uint64_t, type=ht_uint48, name=memAddrIn)\n");
			fprintf(fp, "\t\t.AddParam(hostType=uint64_t, type=ht_uint48, name=memAddrOut)\n");
			fprintf(fp, "\t\t;\n");
			fprintf(fp, "\n");
			fprintf(fp, "\tmod%d.AddReturn(func=Mod%dWr)\n", (int)modIdx+1, (int)modIdx+1);
			fprintf(fp, "\t\t.AddParam(type=short, name=errRtn)\n");
			fprintf(fp, "\t\t;\n");
			fprintf(fp, "\n");
		}

		if (m_varType != eVarPrivate) {
			if (rndMod.m_srcRdFldList.size() > 0 || rndMod.m_mifRdFldList.size() > 0) {
				fprintf(fp, "\tmod%d.AddEntry(func=Mod%dRd, inst=MOD%d_RD1)\n", (int)modIdx+1, (int)modIdx+1, (int)modIdx+1);
				fprintf(fp, "\t\t.AddParam(hostType=uint64_t, type=ht_uint48, name=memAddrIn)\n");
				fprintf(fp, "\t\t.AddParam(hostType=uint64_t, type=ht_uint48, name=memAddrOut)\n");
				fprintf(fp, "\t\t;\n");
				fprintf(fp, "\n");
				fprintf(fp, "\tmod%d.AddReturn(func=Mod%dRd)\n", (int)modIdx+1, (int)modIdx+1);
				fprintf(fp, "\t\t.AddParam(type=short, name=errRtn)\n");
				fprintf(fp, "\t\t;\n");
				fprintf(fp, "\n");
			}
		}

		fprintf(fp, "\tmod%d.AddPrivate()\n", (int)modIdx+1);
		fprintf(fp, "\t\t.AddVar(type=ht_uint48, name=memAddrIn)\n");
		fprintf(fp, "\t\t.AddVar(type=ht_uint48, name=memAddrOut)\n");

		string addrW;
		if (m_var1stIdx == 1 || m_var2ndIdx == 1) {
			char buf[32];
			sprintf(buf, "dimen1=%d, ", m_varDimen1);
			addrW += buf;
		}
		if (m_var1stIdx == 4 || m_var2ndIdx == 4) {
			char buf[32];
			sprintf(buf, "dimen2=%d, ", m_varDimen2);
			addrW += buf;
		}

		if (m_var1stIdx == 0 || m_var2ndIdx == 0) {
			char buf[32];
			switch (m_varType) {
			case eVarShared: sprintf(buf, "addr1W=%d, ", m_varAddr1W); break;
			case eVarGlobalSh:
			case eVarPrivate:
			case eVarGlobalPr: sprintf(buf, "addr1W=%d, addr1=varAddr1, ", m_varAddr1W); break;
			default: assert(0);
			}
			addrW += buf;
			if (modIdx > 0 || m_varType != eVarGlobalSh) {
				fprintf(fp, "\t\t.AddVar(type=ht_uint%d, name=varAddr1)\n", m_varAddr1W);
			}
		}
		if (m_var1stIdx == 3 || m_var2ndIdx == 3) {
			char buf[32];
			switch (m_varType) {
			case eVarShared: sprintf(buf, "addr2W=%d, ", m_varAddr2W); break;
			case eVarPrivate:
			case eVarGlobalPr:
			case eVarGlobalSh: sprintf(buf, "addr2W=%d, addr2=varAddr2, ", m_varAddr2W); break;
			default: assert(0);
			}
			addrW += buf;
			if (modIdx > 0 || m_varType != eVarGlobalSh) {
				fprintf(fp, "\t\t.AddVar(type=ht_uint%d, name=varAddr2)\n", m_varAddr2W);
			}
		}

		if (m_varType == eVarPrivate) {
			for (size_t fldIdx = 0; fldIdx < m_rndFldList.size(); fldIdx += 1) {
				CRndFld &rndFld = m_rndFldList[fldIdx];

				fprintf(fp, "\t\t.AddVar(type=%s, %sname=%s)\n",
					rndFld.m_type.c_str(), addrW.c_str(), rndFld.m_name.c_str());
			}
		}

		fprintf(fp, "\t\t.AddVar(type=short, name=errRtn)\n");
		fprintf(fp, "\t\t;\n");
		fprintf(fp, "\n");

		if (modIdx == 0 && m_varType == eVarGlobalSh) {
			fprintf(fp, "\tmod%d.AddStage( execStg=2 )\n", (int)modIdx+1);
			if (m_var1stIdx == 0 || m_var2ndIdx == 0) {
				fprintf(fp, "\t\t.AddVar(type=ht_uint%d, name=varAddr1, range=1)\n", m_varAddr1W);
			}
			if (m_var1stIdx == 3 || m_var2ndIdx == 3) {
				fprintf(fp, "\t\t.AddVar(type=ht_uint%d, name=varAddr2, range=1)\n", m_varAddr2W);
			}
			fprintf(fp, "\t\t;\n");
			fprintf(fp, "\n");
		}

		switch (m_varType) {
		case eVarPrivate:
			break;
		case eVarShared:
			fprintf(fp, "\tmod%d.AddShared()\n", (int)modIdx+1);

			for (size_t fldIdx = 0; fldIdx < m_rndFldList.size(); fldIdx += 1) {
				CRndFld &rndFld = m_rndFldList[fldIdx];

				fprintf(fp, "\t\t.AddVar( type=%s, %sname=%s )\n",
					rndFld.m_type.c_str(), addrW.c_str(), rndFld.m_name.c_str());
			}

			fprintf(fp, "\t\t;\n");
			break;
		case eVarGlobalSh:
			if (modIdx == 0) {
				fprintf(fp, "\tmod%d.AddGlobal( var=gvar, %sextern=false, rdStg=2, wrStg=2 )\n", (int)modIdx+1, addrW.c_str());
			}
		case eVarGlobalPr:
			if (modIdx == 0) {
				if (m_varType == eVarGlobalPr)
					fprintf(fp, "\tmod%d.AddGlobal( var=gvar, %sextern=false )\n", (int)modIdx+1, addrW.c_str());
			} else {
				fprintf(fp, "\tmod%d.AddGlobal( var=gvar, %sextern=true )\n", (int)modIdx+1, addrW.c_str());
			}
			for (size_t fldIdx = 0; fldIdx < m_rndFldList.size(); fldIdx += 1) {
				CRndFld &rndFld = m_rndFldList[fldIdx];

				bool bSrcRd = false;
				bool bSrcWr = false;
				bool bMifRd = false;
				bool bMifWr = false;

				for (size_t modFldIdx = 0; modFldIdx < rndMod.m_srcRdFldList.size(); modFldIdx += 1) {
					CRndModFld &modFld = rndMod.m_srcRdFldList[modFldIdx];

					if (modFld.m_pRndFld == &rndFld)
						bSrcRd = true;
				}

				for (size_t modFldIdx = 0; modFldIdx < rndMod.m_mifRdFldList.size(); modFldIdx += 1) {
					CRndModFld &modFld = rndMod.m_mifRdFldList[modFldIdx];

					if (modFld.m_pRndFld == &rndFld)
						bMifRd = true;
				}

				for (size_t modFldIdx = 0; modFldIdx < rndMod.m_srcWrFldList.size(); modFldIdx += 1) {
					CRndModFld &modFld = rndMod.m_srcWrFldList[modFldIdx];

					if (modFld.m_pRndFld == &rndFld)
						bSrcWr = true;
				}

				for (size_t modFldIdx = 0; modFldIdx < rndMod.m_mifWrFldList.size(); modFldIdx += 1) {
					CRndModFld &modFld = rndMod.m_mifWrFldList[modFldIdx];

					if (modFld.m_pRndFld == &rndFld)
						bMifWr = true;
				}

				if (bSrcRd || bSrcWr || bMifRd || bMifWr) {
					string addrW;
					if (m_var1stIdx == 2 || m_var2ndIdx == 2) {
						char buf[32];
						sprintf(buf, "dimen1=%d, ", m_fldDimen1);
						addrW = buf;
					}
					if (m_var1stIdx == 5 || m_var2ndIdx == 5) {
						char buf[32];
						sprintf(buf, "dimen2=%d, ", m_fldDimen2);
						addrW += buf;
					}

					fprintf(fp, "\t\t.AddField( type=%s, name=%s, %sread=%s, write=%s )\n",
						rndFld.m_type.c_str(), rndFld.m_name.c_str(), addrW.c_str(),
						bSrcRd ? "true" : "false", bSrcWr ? "true" : "false");
				}
			}
			fprintf(fp, "\t\t;\n");
			fprintf(fp, "\n");
		}

		if (rndMod.m_mifWrFldList.size() > 0) {
			fprintf(fp, "\tmod%d.AddReadMem( maxBw=%s, queueW=%d )\n", (int)modIdx+1,
				rndMod.m_bRdMemMaxBw ? "true" : "false", rndMod.m_memQueW);
			for (size_t modFldIdx = 0; modFldIdx < rndMod.m_mifWrFldList.size(); modFldIdx += 1) {
				CRndModFld &modFld = rndMod.m_mifWrFldList[modFldIdx];

				string dstIdxStr;
				switch (m_var1stIdx) {
				case 0: dstIdxStr = "varAddr1"; break;
				case 1: dstIdxStr = "varIdx1"; break;
				case 2: dstIdxStr = "fldIdx1"; break;
				case 3: dstIdxStr = "varAddr2"; break;
				case 4: dstIdxStr = "varIdx2"; break;
				case 5: dstIdxStr = "fldIdx2"; break;
				}

				switch (m_varType) {
				case eVarShared:
				case eVarPrivate:
					fprintf(fp, "\t\t.AddDst( var=%s, multiRd=true, dstIdx=%s )\n",
						modFld.m_pRndFld->m_name.c_str(), dstIdxStr.c_str());
					break;
				case eVarGlobalPr:
				case eVarGlobalSh:
					fprintf(fp, "\t\t.AddDst( var=gvar, field=%s, multiRd=true, dstIdx=%s )\n",
						modFld.m_pRndFld->m_name.c_str(), dstIdxStr.c_str());
					break;
				default:
					assert(0);
				}
			}
			fprintf(fp, "\t\t;\n");
			fprintf(fp, "\n");
		}

		if (rndMod.m_mifRdFldList.size() > 0) {

			bool bFirstMemWr = true;
			bool bWriteReqPause = false;
			for (size_t fldIdx = 0; fldIdx < rndMod.m_mifRdFldList.size(); fldIdx += 1) {
				CRndFld & rndFld = *rndMod.m_mifRdFldList[fldIdx].m_pRndFld;

				CWrFldIter wrFldIter(this, rndFld);
				while (wrFldIter++) {

					if (bFirstMemWr) {
						bFirstMemWr = false;
					} else {
						bWriteReqPause = true;
					}
				}
			}

			fprintf(fp, "\tmod%d.AddWriteMem( maxBw=%s, queueW=%d, reqPause=%s )\n", (int)modIdx+1,
				rndMod.m_bRdMemMaxBw ? "true" : "false", rndMod.m_memQueW, bWriteReqPause ? "true" : "false");
			for (size_t modFldIdx = 0; modFldIdx < rndMod.m_mifRdFldList.size(); modFldIdx += 1) {
				CRndModFld &modFld = rndMod.m_mifRdFldList[modFldIdx];

				string srcIdxStr;
				switch (m_var1stIdx) {
				case 0: srcIdxStr = "varAddr1"; break;
				case 1: srcIdxStr = "varIdx1"; break;
				case 2: srcIdxStr = "fldIdx1"; break;
				case 3: srcIdxStr = "varAddr2"; break;
				case 4: srcIdxStr = "varIdx2"; break;
				case 5: srcIdxStr = "fldIdx2"; break;
				}

				switch (m_varType) {
				case eVarShared:
				case eVarPrivate:
					fprintf(fp, "\t\t.AddSrc( var=%s, multiWr=true, srcIdx=%s )\n",
						modFld.m_pRndFld->m_name.c_str(), srcIdxStr.c_str());
					break;
				case eVarGlobalPr:
				case eVarGlobalSh:
					fprintf(fp, "\t\t.AddSrc( var=gvar, field=%s, multiWr=true, srcIdx=%s )\n",
						modFld.m_pRndFld->m_name.c_str(), srcIdxStr.c_str());
					break;
				default:
					assert(0);
				}
			}
			fprintf(fp, "\t\t;\n");
			fprintf(fp, "\n");
		}
	}

	fclose(fp);
}

void CDesign::GenCtlSrcFile()
{
	FILE *fp;
	if (!(fp = fopen("src_pers/PersCtl_src.cpp", "w"))) {
		fprintf(stderr, "Unable to open file 'PersCtl_src.cpp'\n");
		exit(1);
	}

	fprintf(fp, "#include \"Ht.h\"\n");
	fprintf(fp, "#include \"PersCtl.h\"\n");
	fprintf(fp, "\n");
	fprintf(fp, "void CPersCtl::PersCtl()\n");
	fprintf(fp, "{\n");
	fprintf(fp, "\tif (PR_htValid) {\n");
	fprintf(fp, "\t\tswitch (PR_htInst) {\n");

	bool bFirst = true;
	for (size_t modIdx = 0; modIdx < m_rndModList.size(); modIdx += 1) {
		CRndMod & rndMod = m_rndModList[modIdx];

		if (rndMod.m_srcWrFldList.size() > 0 || rndMod.m_mifWrFldList.size() > 0) {
			fprintf(fp, "\t\tcase CTL_WR_MOD%d:\n", (int)modIdx+1);
			fprintf(fp, "\t\t{\n");
			fprintf(fp, "\t\t\tif (SendCallBusy_Mod%dWr()) {\n", (int)modIdx+1);
			fprintf(fp, "\t\t\t\tHtRetry();\n");
			fprintf(fp, "\t\t\t\tbreak;\n");
			fprintf(fp, "\t\t\t}\n");
			fprintf(fp, "\n");
			if (bFirst) {
				bFirst = false;
				fprintf(fp, "\t\t\tP_err = 0;\n");
				fprintf(fp, "\n");
			}
			
			fprintf(fp, "\t\t\tSendCall_Mod%dWr(%s, P_memAddrIn, P_memAddrOut);\n", 
				(int)modIdx+1, FindNextCtlInstr(CtlWr, modIdx).c_str());
			fprintf(fp, "\t\t}\n");
			fprintf(fp, "\t\tbreak;\n");
		}
	}

	if (m_varType != eVarPrivate) {
		bFirst = true;
		for (size_t modIdx = 0; modIdx < m_rndModList.size(); modIdx += 1) {
			CRndMod & rndMod = m_rndModList[modIdx];

			if (rndMod.m_srcRdFldList.size() > 0 || rndMod.m_mifRdFldList.size() > 0) {
				fprintf(fp, "\t\tcase CTL_RD_MOD%d:\n", (int)modIdx+1);
				fprintf(fp, "\t\t{\n");
				fprintf(fp, "\t\t\tif (SendCallBusy_Mod%dRd()) {\n", (int)modIdx+1);
				fprintf(fp, "\t\t\t\tHtRetry();\n");
				fprintf(fp, "\t\t\t\tbreak;\n");
				fprintf(fp, "\t\t\t}\n");
				fprintf(fp, "\n");
				if (bFirst) {
					bFirst = false;
				} else {
					fprintf(fp, "\t\t\tP_err += P_errRtn;\n");
					fprintf(fp, "\n");
				}
				fprintf(fp, "\t\t\tSendCall_Mod%dRd(%s, P_memAddrIn, P_memAddrOut);\n", (int)modIdx+1, FindNextCtlInstr(CtlRd, modIdx).c_str());
				fprintf(fp, "\t\t}\n");
				fprintf(fp, "\t\tbreak;\n");
			}
		}
	}

	fprintf(fp, "\t\tcase CTL_RTN:\n");
	fprintf(fp, "\t\t{\n");
	fprintf(fp, "\t\t\tif (SendReturnBusy_htmain()) {\n");
	fprintf(fp, "\t\t\t\tHtRetry();\n");
	fprintf(fp, "\t\t\t\tbreak;\n");
	fprintf(fp, "\t\t\t}\n");
	fprintf(fp, "\n");
	fprintf(fp, "\t\t\tP_err += P_errRtn;\n");
	fprintf(fp, "\n");
	fprintf(fp, "\t\t\tSendReturn_htmain(P_err);\n");
	fprintf(fp, "\t\t}\n");
	fprintf(fp, "\t\tbreak;\n");
	fprintf(fp, "\t\tdefault:\n");
	fprintf(fp, "\t\t\tassert(0);\n");
	fprintf(fp, "\t\t}\n");
	fprintf(fp, "\t}\n");
	fprintf(fp, "}\n");

	fclose(fp);
}

string CDesign::FindNextCtlInstr(ECtlPhase ctlPhase, size_t modIdx)
{
	char instrName[32];
	modIdx += 1;

	// find the name of the next instruction
	if (ctlPhase == CtlWr) {
		for ( ; modIdx < m_rndModList.size(); modIdx += 1) {
			CRndMod & rndMod = m_rndModList[modIdx];

			if (rndMod.m_srcWrFldList.size() > 0 || rndMod.m_mifWrFldList.size() > 0) {
				sprintf(instrName, "CTL_WR_MOD%d", (int)modIdx+1);
				return string(instrName);
			}
		}

		modIdx = 0;
	}

	if (m_varType != eVarPrivate) {
		for ( ; modIdx < m_rndModList.size(); modIdx += 1) {
			CRndMod & rndMod = m_rndModList[modIdx];

			if (rndMod.m_srcRdFldList.size() > 0 || rndMod.m_mifRdFldList.size() > 0) {
				sprintf(instrName, "CTL_RD_MOD%d", (int)modIdx+1);
				return string(instrName);
			}
		}
	}

	return string("CTL_RTN");
}

void CDesign::GenModSrcFiles()
{
	char const * pStgStr= 0;
	for (size_t modIdx = 0; modIdx < m_rndModList.size(); modIdx += 1) {
		CRndMod & rndMod = m_rndModList[modIdx];

		char fileName[64];
		sprintf(fileName, "src_pers/PersMod%d_src.cpp", (int)modIdx+1);

		FILE *fp;
		if (!(fp = fopen(fileName, "w"))) {
			fprintf(stderr, "Unable to open file '%s'\n", fileName);
			exit(1);
		}

		fprintf(fp, "#include \"Ht.h\"\n");
		fprintf(fp, "#include \"PersMod%d.h\"\n", (int)modIdx+1);
		fprintf(fp, "\n");
		fprintf(fp, "void CPersMod%d::PersMod%d()\n", (int)modIdx+1, (int)modIdx+1);
		fprintf(fp, "{\n");

		if (modIdx == 0 && m_varType == eVarGlobalSh) {
			pStgStr = "2";

			int rdIdx = 1;
			for (size_t fldIdx = 0; fldIdx < rndMod.m_srcRdFldList.size(); fldIdx += 1) {
				CRndFld & rndFld = *rndMod.m_srcRdFldList[fldIdx].m_pRndFld;

				fprintf(fp, "\tif (PR1_htValid) {\n");
				fprintf(fp, "\t\tswitch (PR1_htInst) {\n");

				CWrFldIter wrFldIter(this, rndFld);
				while (wrFldIter++) {
					int wrCmdIdx = wrFldIter.m_pRndModFld->m_wrId;

					rdIdx++;

					fprintf(fp, "\t\tcase MOD%d_RD%d:\n", (int)modIdx+1, rdIdx++);
					fprintf(fp, "\t\t{\n");

					GenVarIdxStr(fp, wrCmdIdx, 0, eGblShIdx, true);

					fprintf(fp, "\t\t}\n");
					fprintf(fp, "\t\tbreak;\n");

					fprintf(fp, "\t\tcase MOD%d_RD%d:\n", (int)modIdx+1, rdIdx++);
					fprintf(fp, "\t\t{\n");

					GenVarIdxStr(fp, wrCmdIdx, 1, eGblShIdx, true);

					fprintf(fp, "\t\t}\n");
					fprintf(fp, "\t\tbreak;\n");
				}

				fprintf(fp, "\t\tdefault:\n");
				fprintf(fp, "\t\t{\n");

				GenVarIdxStr(fp, -1, 0, eGblShIdx, true);

				fprintf(fp, "\t\t}\n");
				fprintf(fp, "\t\tbreak;\n");

				fprintf(fp, "\t\t}\n");
				fprintf(fp, "\t}\n");
				fprintf(fp, "\n");
			}

		} else
			pStgStr = "";

		fprintf(fp, "\tif (PR%s_htValid) {\n", pStgStr);
		fprintf(fp, "\t\tswitch (PR%s_htInst) {\n", pStgStr);

		int wrIdx = 1;
		for (size_t fldIdx = 0; fldIdx < rndMod.m_srcWrFldList.size(); fldIdx += 1) {
			CRndFld & rndFld = *rndMod.m_srcWrFldList[fldIdx].m_pRndFld;

			fprintf(fp, "\t\tcase MOD%d_WR%d:\n", (int)modIdx+1, wrIdx++);
			fprintf(fp, "\t\t{\n");

			int wrCmdIdx = rndMod.m_srcWrFldList[fldIdx].m_wrId;


			switch (m_varType) {
			case eVarShared:
				{
					string varAddrStr, varIdxStr1;
					GenSharedVarIdxStr(fp, wrCmdIdx, 0, eSrcIdx, varAddrStr, varIdxStr1);

					if (varAddrStr.size() > 0) {
						fprintf(fp, "\t\t\tS_%s%s.write_addr(%s);\n",
							rndFld.m_name.c_str(), varIdxStr1.c_str(), varAddrStr.c_str());
						fprintf(fp, "\t\t\tS_%s%s.write_mem(%s);\n",
							rndFld.m_name.c_str(), varIdxStr1.c_str(), wrCmdIdx == 0 ? "0xaaaabbbbcccc1234LL" : "0xaaaabbbbcccc3456LL");
					} else {
						fprintf(fp, "\t\t\tS_%s%s = %s;\n",
							rndFld.m_name.c_str(), varIdxStr1.c_str(), wrCmdIdx == 0 ? "0xaaaabbbbcccc1234LL" : "0xaaaabbbbcccc3456LL");
					}
				}
				break;
			case eVarPrivate:
				{
					string varIdxStr1 = GenVarIdxStr(fp, wrCmdIdx, 0, eSrcIdx, false);

					if (m_varDimen1 > 0 && m_varDimen2 > 0 && rndMod.m_mifWrFldList.size() == 0 && rndMod.m_mifRdFldList.size() == 0)
						fprintf(fp, "\t\t\tP_%s[varIdx1][varIdx2] = %s;\n", 
							rndFld.m_name.c_str(), wrCmdIdx == 0 ? "0xaaaabbbbcccc1234LL" : "0xaaaabbbbcccc3456LL");
					else
						fprintf(fp, "\t\t\tPW_%s(%s%s);\n", 
							rndFld.m_name.c_str(), varIdxStr1.c_str(), wrCmdIdx == 0 ? "0xaaaabbbbcccc1234LL" : "0xaaaabbbbcccc3456LL");
				}
				break;
			case eVarGlobalPr:
			case eVarGlobalSh:
				{
					string varIdxStr1 = GenVarIdxStr(fp, wrCmdIdx, 0, eSrcIdx, false);

					fprintf(fp, "\t\t\tGW%s_gvar_%s(%s%s);\n", pStgStr, 
						rndFld.m_name.c_str(), varIdxStr1.c_str(), wrCmdIdx == 0 ? "0xaaaabbbbcccc1234LL" : "0xaaaabbbbcccc3456LL");
				}
				break;
			default: assert(0);
			}

			fprintf(fp, "\n");
			fprintf(fp, "\t\t\tHtContinue(MOD%d_WR%d);\n", (int)modIdx+1, wrIdx);
			fprintf(fp, "\t\t}\n");
			fprintf(fp, "\t\tbreak;\n");
			fprintf(fp, "\t\tcase MOD%d_WR%d:\n", (int)modIdx+1, wrIdx++);
			fprintf(fp, "\t\t{\n");

			switch (m_varType) {
			case eVarShared:
				{
					string varAddrStr, varIdxStr2;
					GenSharedVarIdxStr(fp, wrCmdIdx, 1, eSrcIdx, varAddrStr, varIdxStr2);

					if (varAddrStr.size() > 0) {
						fprintf(fp, "\t\t\tS_%s%s.write_addr(%s);\n",
							rndFld.m_name.c_str(), varIdxStr2.c_str(), varAddrStr.c_str());
						fprintf(fp, "\t\t\tS_%s%s.write_mem(%s);\n",
							rndFld.m_name.c_str(), varIdxStr2.c_str(), wrCmdIdx == 0 ? "0xaaaabbbbcccc2345LL" : "0xaaaabbbbcccc4567LL");
					} else {
						fprintf(fp, "\t\t\tS_%s%s = %s;\n",
							rndFld.m_name.c_str(), varIdxStr2.c_str(), wrCmdIdx == 0 ? "0xaaaabbbbcccc2345LL" : "0xaaaabbbbcccc4567LL");
					}
				}
				break;
			case eVarPrivate:
				{
					string varIdxStr2 = GenVarIdxStr(fp, wrCmdIdx, 1, eSrcIdx, false);

					if (m_varDimen1 > 0 && m_varDimen2 > 0 && rndMod.m_mifWrFldList.size() == 0 && rndMod.m_mifRdFldList.size() == 0)
						fprintf(fp, "\t\t\tP_%s[varIdx1][varIdx2] = %s;\n", 
							rndFld.m_name.c_str(), wrCmdIdx == 0 ? "0xaaaabbbbcccc2345LL" : "0xaaaabbbbcccc4567LL");
					else
						fprintf(fp, "\t\t\tPW_%s(%s%s);\n",
							rndFld.m_name.c_str(), varIdxStr2.c_str(), wrCmdIdx == 0 ? "0xaaaabbbbcccc2345LL" : "0xaaaabbbbcccc4567LL");
				}
				break;
			case eVarGlobalPr:
			case eVarGlobalSh:
				{
					string varIdxStr2 = GenVarIdxStr(fp, wrCmdIdx, 1, eSrcIdx, false);

					fprintf(fp, "\t\t\tGW%s_gvar_%s(%s%s);\n", pStgStr,
						rndFld.m_name.c_str(), varIdxStr2.c_str(), wrCmdIdx == 0 ? "0xaaaabbbbcccc2345LL" : "0xaaaabbbbcccc4567LL");
				}
				break;
			default: assert(0);
			}

			fprintf(fp, "\n");
			if (fldIdx+1 == rndMod.m_srcWrFldList.size() && rndMod.m_mifWrFldList.size() == 0) {
				if (m_varType != eVarPrivate)
					fprintf(fp, "\t\t\tHtContinue(MOD%d_WR_RTN);\n", (int)modIdx+1);
				else
					fprintf(fp, "\t\t\tHtContinue(MOD%d_RD1);\n", (int)modIdx+1);
			} else
				fprintf(fp, "\t\t\tHtContinue(MOD%d_WR%d);\n", (int)modIdx+1, wrIdx);
			fprintf(fp, "\t\t}\n");
			fprintf(fp, "\t\tbreak;\n");
		}

		for (size_t fldIdx = 0; fldIdx < rndMod.m_mifWrFldList.size(); fldIdx += 1) {
			CRndFld & rndFld = *rndMod.m_mifWrFldList[fldIdx].m_pRndFld;

			fprintf(fp, "\t\tcase MOD%d_WR%d:\n", (int)modIdx+1, wrIdx++);
			fprintf(fp, "\t\t{\n");
			fprintf(fp, "\t\t\tif (ReadMemBusy()) {\n");
			fprintf(fp, "\t\t\t\tHtRetry();\n");
			fprintf(fp, "\t\t\t\tbreak;\n");
			fprintf(fp, "\t\t\t}\n");
			fprintf(fp, "\n");

			int wrCmdIdx = rndMod.m_mifWrFldList[fldIdx].m_wrId;
			string varIdxStr1 = GenVarIdxStr(fp, wrCmdIdx, 0, eSrcIdx, false);

			fprintf(fp, "\t\t\tht_uint4 qwCnt = 2;\n");
			fprintf(fp, "\n");
			switch (m_varType) {
			case eVarPrivate:
			case eVarShared:
				fprintf(fp, "\t\t\tReadMem_%s(P%s_memAddrIn+%d, %sqwCnt);\n", 
					rndFld.m_name.c_str(), pStgStr, wrCmdIdx*16, varIdxStr1.c_str());
				break;
			case eVarGlobalPr:
			case eVarGlobalSh:
				fprintf(fp, "\t\t\tReadMem_gvar(P%s_memAddrIn+%d, %sqwCnt);\n", 
					pStgStr, wrCmdIdx*16, varIdxStr1.c_str());
				break;
			default:
				assert(0);
			}
			fprintf(fp, "\n");
			if (m_varType != eVarPrivate)
				fprintf(fp, "\t\t\tReadMemPause(MOD%d_WR_RTN);\n", (int)modIdx+1);
			else
				fprintf(fp, "\t\t\tReadMemPause(MOD%d_RD1);\n", (int)modIdx+1);
			fprintf(fp, "\t\t}\n");
			fprintf(fp, "\t\tbreak;\n");
		}

		if (m_varType != eVarPrivate) {
			if (rndMod.m_srcWrFldList.size() > 0 || rndMod.m_mifWrFldList.size() > 0) {
				fprintf(fp, "\t\tcase MOD%d_WR_RTN:\n", (int)modIdx+1);
				fprintf(fp, "\t\t{\n");
				fprintf(fp, "\t\t\tif (SendReturnBusy_Mod%dWr()) {\n", (int)modIdx+1);
				fprintf(fp, "\t\t\t\tHtRetry();\n");
				fprintf(fp, "\t\t\t\tbreak;\n");
				fprintf(fp, "\t\t\t}\n");
				fprintf(fp, "\n");
				fprintf(fp, "\t\t\tSendReturn_Mod%dWr(0);\n", (int)modIdx+1);
				fprintf(fp, "\t\t}\n");
				fprintf(fp, "\t\tbreak;\n");
			}
		}

		int rdIdx = 1;
		bool bFirst = true;
		for (size_t fldIdx = 0; fldIdx < rndMod.m_srcRdFldList.size(); fldIdx += 1) {
			CRndFld & rndFld = *rndMod.m_srcRdFldList[fldIdx].m_pRndFld;

			CWrFldIter wrFldIter(this, rndFld);
			while (wrFldIter++) {

				if (bFirst)
					bFirst = false;
				else {
					fprintf(fp, "\t\t\tHtContinue(MOD%d_RD%d);\n", (int)modIdx+1, rdIdx);
					fprintf(fp, "\t\t}\n");
					fprintf(fp, "\t\tbreak;\n");
				}

				fprintf(fp, "\t\tcase MOD%d_RD%d:\n", (int)modIdx+1, rdIdx++);
				fprintf(fp, "\t\t{\n");

				if (rdIdx == 2)
					fprintf(fp, "\t\t\tP%s_errRtn = 0;\n", pStgStr);

				int wrCmdIdx = wrFldIter.m_pRndModFld->m_wrId;
				GenVarIdxStr(fp, wrCmdIdx, 0, eMifAddr, modIdx == 0 && m_varType == eVarGlobalSh);

				fprintf(fp, "\n");
				fprintf(fp, "\t\t\tHtContinue(MOD%d_RD%d);\n", (int)modIdx+1, rdIdx);
				fprintf(fp, "\t\t}\n");
				fprintf(fp, "\t\tbreak;\n");
				fprintf(fp, "\t\tcase MOD%d_RD%d:\n", (int)modIdx+1, rdIdx++);
				fprintf(fp, "\t\t{\n");

				switch (m_varType) {
				case eVarShared:
					{
						string varAddrStr, varIdxStr1;
						GenSharedVarIdxStr(fp, wrCmdIdx, 0, eSrcIdx, varAddrStr, varIdxStr1);

						if (varAddrStr.size() > 0) {
							fprintf(fp, "\t\t\tS_%s%s.read_addr(%s);\n",
								rndFld.m_name.c_str(), varIdxStr1.c_str(), varAddrStr.c_str());
							fprintf(fp, "\t\t\tuint64_t data = S_%s%s.read_mem();\n",
								rndFld.m_name.c_str(), varIdxStr1.c_str());
						} else {
							fprintf(fp, "\t\t\tuint64_t data = S_%s%s;\n",
								rndFld.m_name.c_str(), varIdxStr1.c_str());
						}
					}
					break;
				case eVarPrivate:
					{
						string varAddrStr, varIdxStr1;
						GenSharedVarIdxStr(fp, wrCmdIdx, 0, eMifIdx, varAddrStr, varIdxStr1);

						fprintf(fp, "\t\t\tuint64_t data = PR_%s%s;\n",
							rndFld.m_name.c_str(), varIdxStr1.c_str());
					}
					break;
				case eVarGlobalPr:
				case eVarGlobalSh:
					{				
						string fldIdxStr1 = GenVarIdxStr(fp, wrCmdIdx, 0, eMifIdx, false);

						if (fldIdxStr1.size() == 0)
							fprintf(fp, "\t\t\t%s data = GR%s_gvar_%s();\n",
								rndFld.m_type.c_str(), pStgStr, rndFld.m_name.c_str());
						else
							fprintf(fp, "\t\t\t%s data = GR%s_gvar_%s(%s);\n",
								rndFld.m_type.c_str(), pStgStr, rndFld.m_name.c_str(), fldIdxStr1.substr(0, fldIdxStr1.size()-2).c_str());
					}
					break;
				default:
					assert(0);
				}

				fprintf(fp, "\n");
				fprintf(fp, "\t\t\tif (data != %s)\n",
					wrFldIter.m_pRndModFld->m_wrId == 0 ? (wrFldIter.m_bSrcWr ? "0xaaaabbbbcccc1234LL" : "0xddddeeeeffff1234LL")
					: (wrFldIter.m_bSrcWr ? "0xaaaabbbbcccc3456LL" : "0xddddeeeeffff3456LL"));
				fprintf(fp, "\t\t\t\tP%s_errRtn += 1;\n", pStgStr);

				GenVarIdxStr(fp, wrCmdIdx, 1, eMifAddr, modIdx == 0 && m_varType == eVarGlobalSh);

				fprintf(fp, "\n");
				fprintf(fp, "\t\t\tHtContinue(MOD%d_RD%d);\n", (int)modIdx+1, rdIdx);
				fprintf(fp, "\t\t}\n");
				fprintf(fp, "\t\tbreak;\n");
				fprintf(fp, "\t\tcase MOD%d_RD%d:\n", (int)modIdx+1, rdIdx++);
				fprintf(fp, "\t\t{\n");

				switch (m_varType) {
				case eVarPrivate:
					{
						string varAddrStr, varIdxStr2;
						GenSharedVarIdxStr(fp, wrCmdIdx, 1, eMifIdx, varAddrStr, varIdxStr2);

						fprintf(fp, "\t\t\tuint64_t data = PR_%s%s;\n",
							rndFld.m_name.c_str(), varIdxStr2.c_str());
					}
					break;
				case eVarShared:
					{
						string varAddrStr, varIdxStr2;
						GenSharedVarIdxStr(fp, wrCmdIdx, 1, eSrcIdx, varAddrStr, varIdxStr2);

						if (varAddrStr.size() > 0) {
							fprintf(fp, "\t\t\tS_%s%s.read_addr(%s);\n",
								rndFld.m_name.c_str(), varIdxStr2.c_str(), varAddrStr.c_str());
							fprintf(fp, "\t\t\tuint64_t data = S_%s%s.read_mem();\n",
								rndFld.m_name.c_str(), varIdxStr2.c_str());
						} else {
							fprintf(fp, "\t\t\tuint64_t data = S_%s%s;\n",
								rndFld.m_name.c_str(), varIdxStr2.c_str());
						}
					}
					break;
				case eVarGlobalPr:
				case eVarGlobalSh:
					{				
						string fldIdxStr2 = GenVarIdxStr(fp, wrCmdIdx, 1, eMifIdx, false);

						if (fldIdxStr2.size() == 0)
							fprintf(fp, "\t\t\t%s data = GR%s_gvar_%s();\n",
								rndFld.m_type.c_str(), pStgStr, rndFld.m_name.c_str());
						else
							fprintf(fp, "\t\t\t%s data = GR%s_gvar_%s(%s);\n",
								rndFld.m_type.c_str(), pStgStr, rndFld.m_name.c_str(), fldIdxStr2.substr(0, fldIdxStr2.size()-2).c_str());
					}
					break;
				default:
					assert(0);
				}

				fprintf(fp, "\n");
				fprintf(fp, "\t\t\tif (data != %s)\n",
					wrFldIter.m_pRndModFld->m_wrId == 0 ? (wrFldIter.m_bSrcWr ? "0xaaaabbbbcccc2345LL" : "0xddddeeeeffff2345LL")
					: (wrFldIter.m_bSrcWr ? "0xaaaabbbbcccc4567LL" : "0xddddeeeeffff4567LL"));
				fprintf(fp, "\t\t\t\tP%s_errRtn += 1;\n", pStgStr);
				fprintf(fp, "\n");
			}
		}

		bool bFirstMemWr = true;
		for (size_t fldIdx = 0; fldIdx < rndMod.m_mifRdFldList.size(); fldIdx += 1) {
			CRndFld & rndFld = *rndMod.m_mifRdFldList[fldIdx].m_pRndFld;

			int mifRdId = rndMod.m_mifRdFldList[fldIdx].m_mifRdId;
			int wrCnt = rndFld.m_wrCnt;

			CWrFldIter wrFldIter(this, rndFld);
			while (wrFldIter++) {

				if (bFirst) {
					bFirst = false;
					bFirstMemWr = false;
				} else if (bFirstMemWr) {
					bFirstMemWr = false;
					fprintf(fp, "\t\t\tHtContinue(MOD%d_RD%d);\n", (int)modIdx+1, rdIdx);
					fprintf(fp, "\t\t}\n");
					fprintf(fp, "\t\tbreak;\n");
				} else {
					fprintf(fp, "\t\t\tWriteReqPause(MOD%d_RD%d);\n", (int)modIdx+1, rdIdx);
					fprintf(fp, "\t\t}\n");
					fprintf(fp, "\t\tbreak;\n");
				}

				fprintf(fp, "\t\tcase MOD%d_RD%d:\n", (int)modIdx+1, rdIdx++);
				fprintf(fp, "\t\t{\n");
				fprintf(fp, "\t\t\tif (WriteMemBusy()) {\n");
				fprintf(fp, "\t\t\t\tHtRetry();\n");
				fprintf(fp, "\t\t\t\tbreak;\n");
				fprintf(fp, "\t\t\t}\n");
				fprintf(fp, "\n");

				int wrId = wrFldIter.m_pRndModFld->m_wrId;
				string varIdxStr1 = GenVarIdxStr(fp, wrId, 0, eSrcIdx, false);

				fprintf(fp, "\t\t\tht_uint4 qwCnt = 2;\n");
				fprintf(fp, "\n");
				switch (m_varType) {
				case eVarPrivate:
				case eVarShared:
					fprintf(fp, "\t\t\tWriteMem_%s(P%s_memAddrOut+%d, %sqwCnt);\n",
						rndFld.m_name.c_str(), pStgStr, mifRdId * wrCnt + wrId * 16, varIdxStr1.c_str());
					break;
				case eVarGlobalPr:
				case eVarGlobalSh:
					fprintf(fp, "\t\t\tWriteMem_gvar(P%s_memAddrOut+%d, %sqwCnt);\n",
						pStgStr, mifRdId * wrCnt + wrId * 16, varIdxStr1.c_str());
					break;
				default:
					assert(0);
				}
				fprintf(fp, "\n");
			}
		}
		
		if (!bFirst) {
			if (bFirstMemWr) {
				fprintf(fp, "\t\t\tHtContinue(MOD%d_RD_RTN);\n", (int)modIdx+1);
				fprintf(fp, "\t\t}\n");
				fprintf(fp, "\t\tbreak;\n");
			} else {
				fprintf(fp, "\t\t\tWriteMemPause(MOD%d_RD_RTN);\n", (int)modIdx+1);
				fprintf(fp, "\t\t}\n");
				fprintf(fp, "\t\tbreak;\n");
			}
		}

		if (m_varType != eVarPrivate) {
			if (rndMod.m_srcRdFldList.size() > 0 || rndMod.m_mifRdFldList.size() > 0) {
				fprintf(fp, "\t\tcase MOD%d_RD_RTN:\n", (int)modIdx+1);
				fprintf(fp, "\t\t{\n");
				fprintf(fp, "\t\t\tif (SendReturnBusy_Mod%dRd()) {\n", (int)modIdx+1);
				fprintf(fp, "\t\t\t\tHtRetry();\n");
				fprintf(fp, "\t\t\t\tbreak;\n");
				fprintf(fp, "\t\t\t}\n");
				fprintf(fp, "\n");
				fprintf(fp, "\t\t\tSendReturn_Mod%dRd(P%s_errRtn);\n", (int)modIdx+1, pStgStr);
				fprintf(fp, "\t\t}\n");
				fprintf(fp, "\t\tbreak;\n");
			}
		} else {
			if (rndMod.m_srcRdFldList.size() > 0 || rndMod.m_mifRdFldList.size() > 0) {
				fprintf(fp, "\t\tcase MOD%d_RD_RTN:\n", (int)modIdx+1);
				fprintf(fp, "\t\t{\n");
				fprintf(fp, "\t\t\tif (SendReturnBusy_Mod%dWr()) {\n", (int)modIdx+1);
				fprintf(fp, "\t\t\t\tHtRetry();\n");
				fprintf(fp, "\t\t\t\tbreak;\n");
				fprintf(fp, "\t\t\t}\n");
				fprintf(fp, "\n");
				fprintf(fp, "\t\t\tSendReturn_Mod%dWr(P%s_errRtn);\n", (int)modIdx+1, pStgStr);
				fprintf(fp, "\t\t}\n");
				fprintf(fp, "\t\tbreak;\n");
			}
		}
		fprintf(fp, "\t\tdefault:\n");
		fprintf(fp, "\t\t\tassert(0);\n");
		fprintf(fp, "\t\t}\n");
		fprintf(fp, "\t}\n");
		fprintf(fp, "}\n");

		fclose(fp);
	}
}

string CDesign::GenVarIdxStr(FILE * fp, int wrCmdIdx, int wrReqIdx, EIdxMode idxMode, bool bVarGlobalSh)
{
	int idx1stValue = 0;
	int idx2ndValue = 0;

	switch (wrCmdIdx) {
	case 0:
		idx1stValue = 1 + wrReqIdx;
		idx2ndValue = 3;
		break;
	case 1:
		idx1stValue = 3 + wrReqIdx;
		idx2ndValue = 2;
		break;
	default:
		break;
	}

	if (idxMode == eGblShIdx) {
		if (m_var1stIdx == 0)
			fprintf(fp, "\t\t\tT1_varAddr1 = %d;\n", idx1stValue);
		if (m_var2ndIdx == 0)
			fprintf(fp, "\t\t\tT1_varAddr1 = %d;\n", idx2ndValue);
		if (m_var1stIdx == 3)
			fprintf(fp, "\t\t\tT1_varAddr2 = %d;\n", idx1stValue);
		if (m_var2ndIdx == 3)
			fprintf(fp, "\t\t\tT1_varAddr2 = %d;\n", idx2ndValue);
		return string();
	}

	if (idxMode == eMifAddr) {
		if (!bVarGlobalSh) {
			if (m_var1stIdx == 0)
				fprintf(fp, "\t\t\tP_varAddr1 = %d;\n", idx1stValue);
			if (m_var2ndIdx == 0)
				fprintf(fp, "\t\t\tP_varAddr1 = %d;\n", idx2ndValue);
			if (m_var1stIdx == 3)
				fprintf(fp, "\t\t\tP_varAddr2 = %d;\n", idx1stValue);
			if (m_var2ndIdx == 3)
				fprintf(fp, "\t\t\tP_varAddr2 = %d;\n", idx2ndValue);
		}
		return string();
	}

	string varIdxStr1;
	if (idxMode == eSrcIdx) {
		if (m_var1stIdx == 0) {
			fprintf(fp, "\t\t\tht_uint%d varAddr1 = %d;\n", m_varAddr1W, idx1stValue);
			varIdxStr1 += "varAddr1, ";
		}
		if (m_var2ndIdx == 0) {
			fprintf(fp, "\t\t\tht_uint%d varAddr1 = %d;\n", m_varAddr1W, idx2ndValue);
			varIdxStr1 += "varAddr1, ";
		}
		if (m_var1stIdx == 3) {
			fprintf(fp, "\t\t\tht_uint3 varAddr2 = %d;\n", idx1stValue);
			varIdxStr1 += "varAddr2, ";
		}
		if (m_var2ndIdx == 3) {
			fprintf(fp, "\t\t\tht_uint3 varAddr2 = %d;\n", idx2ndValue);
			varIdxStr1 += "varAddr2, ";
		}
	}
	if (m_var1stIdx == 1) {
		fprintf(fp, "\t\t\tht_uint4 varIdx1 = %d;\n", idx1stValue);
		varIdxStr1 += "varIdx1, ";
	}
	if (m_var2ndIdx == 1) {
		fprintf(fp, "\t\t\tht_uint4 varIdx1 = %d;\n", idx2ndValue);
		varIdxStr1 += "varIdx1, ";
	}
	if (m_var1stIdx == 4) {
		fprintf(fp, "\t\t\tht_uint3 varIdx2 = %d;\n", idx1stValue);
		varIdxStr1 += "varIdx2, ";
	}
	if (m_var2ndIdx == 4) {
		fprintf(fp, "\t\t\tht_uint3 varIdx2 = %d;\n", idx2ndValue);
		varIdxStr1 += "varIdx2, ";
	}
	if (m_var1stIdx == 2) {
		fprintf(fp, "\t\t\tht_uint4 fldIdx1 = %d;\n", idx1stValue);
		varIdxStr1 += "fldIdx1, ";
	}
	if (m_var2ndIdx == 2) {
		fprintf(fp, "\t\t\tht_uint4 fldIdx1 = %d;\n", idx2ndValue);
		varIdxStr1 += "fldIdx1, ";
	}
	if (m_var1stIdx == 5) {
		fprintf(fp, "\t\t\tht_uint3 fldIdx2 = %d;\n", idx1stValue);
		varIdxStr1 += "fldIdx2, ";
	}
	if (m_var2ndIdx == 5) {
		fprintf(fp, "\t\t\tht_uint3 fldIdx2 = %d;\n", idx2ndValue);
		varIdxStr1 += "fldIdx2, ";
	}

	return varIdxStr1;
}

void CDesign::GenSharedVarIdxStr(FILE * fp, int wrCmdIdx, int wrReqIdx, EIdxMode idxMode, string & varAddrStr, string & varIdxStr)
{
	int idx1stValue = wrCmdIdx == 0 ? (1 + wrReqIdx) : (3 + wrReqIdx);
	int idx2ndValue = wrCmdIdx == 0 ? 3 : 2;

	//if (idxMode == eMifAddr) {
	//	if (m_var1stIdx == 0)
	//		fprintf(fp, "\t\t\tP_varAddr1 = %d;\n", idx1stValue);
	//	if (m_var2ndIdx == 0)
	//		fprintf(fp, "\t\t\tP_varAddr1 = %d;\n", idx2ndValue);
	//	if (m_var1stIdx == 3)
	//		fprintf(fp, "\t\t\tP_varAddr2 = %d;\n", idx1stValue);
	//	if (m_var2ndIdx == 3)
	//		fprintf(fp, "\t\t\tP_varAddr2 = %d;\n", idx2ndValue);
	//	return string();
	//}

	if (idxMode == eSrcIdx) {
		if (m_var1stIdx == 0) {
			fprintf(fp, "\t\t\tht_uint%d varAddr1 = %d;\n", m_varAddr1W, idx1stValue);
			varAddrStr += "varAddr1, ";
		}
		if (m_var2ndIdx == 0) {
			fprintf(fp, "\t\t\tht_uint%d varAddr1 = %d;\n", m_varAddr1W, idx2ndValue);
			varAddrStr += "varAddr1, ";
		}
		if (m_var1stIdx == 3) {
			fprintf(fp, "\t\t\tht_uint3 varAddr2 = %d;\n", idx1stValue);
			varAddrStr += "varAddr2, ";
		}
		if (m_var2ndIdx == 3) {
			fprintf(fp, "\t\t\tht_uint3 varAddr2 = %d;\n", idx2ndValue);
			varAddrStr += "varAddr2, ";
		}
	}
	if (m_var1stIdx == 1) {
		fprintf(fp, "\t\t\tht_uint4 varIdx1 = %d;\n", idx1stValue);
		varIdxStr += "[varIdx1]";
	}
	if (m_var2ndIdx == 1) {
		fprintf(fp, "\t\t\tht_uint4 varIdx1 = %d;\n", idx2ndValue);
		varIdxStr += "[varIdx1]";
	}
	if (m_var1stIdx == 4) {
		fprintf(fp, "\t\t\tht_uint3 varIdx2 = %d;\n", idx1stValue);
		varIdxStr += "[varIdx2]";
	}
	if (m_var2ndIdx == 4) {
		fprintf(fp, "\t\t\tht_uint3 varIdx2 = %d;\n", idx2ndValue);
		varIdxStr += "[varIdx2]";
	}

	if (varAddrStr.size() > 0)
		varAddrStr.erase(varAddrStr.size()-2, 2);
}

void CDesign::GenMain()
{
	FILE *fp;
	if (!(fp = fopen("src/Main.cpp", "w"))) {
		fprintf(stderr, "Unable to open file 'Main.cpp'\n");
		exit(1);
	}

	fprintf(fp, "#include <string.h>\n");
	fprintf(fp, "\n");
	fprintf(fp, "#include \"Ht.h\"\n");
	fprintf(fp, "\n");
	fprintf(fp, "int main(int argc, char **argv)\n");
	fprintf(fp, "{\n");

	fprintf(fp, "\tuint64_t * pDataIn;\n");
	fprintf(fp, "\tht_cp_posix_memalign((void **)&pDataIn, 64, 64);\n");
	fprintf(fp, "\n");
	fprintf(fp, "\tpDataIn[0] = 0xddddeeeeffff1234ULL;\n");
	fprintf(fp, "\tpDataIn[1] = 0xddddeeeeffff2345ULL;\n");
	fprintf(fp, "\tpDataIn[2] = 0xddddeeeeffff3456ULL;\n");
	fprintf(fp, "\tpDataIn[3] = 0xddddeeeeffff4567ULL;\n");
	fprintf(fp, "\n");
	fprintf(fp, "\tuint64_t * pDataOut;\n");
	fprintf(fp, "\tht_cp_posix_memalign((void **)&pDataOut, 64, 32 * 8);\n");
	fprintf(fp, "\n");
	fprintf(fp, "\tCHtHif *pHtHif = new CHtHif();\n");
	fprintf(fp, "\tCHtAuUnit *pAuUnit = pHtHif->AllocAuUnit();\n");
	fprintf(fp, "\n");
	fprintf(fp, "\tshort err = 0;\n");
	fprintf(fp, "\tfor (int loopCnt = 0; loopCnt < 4; loopCnt += 1) {\n");
	fprintf(fp, "\n");
	fprintf(fp, "\t\tmemset(pDataOut, 0, 32*8);\n");
	fprintf(fp, "\n");
	fprintf(fp, "\t\twhile(!pAuUnit->SendCall_htmain((uint64_t)pDataIn, (uint64_t)pDataOut));\n");
	fprintf(fp, "\n");
	fprintf(fp, "\t\tshort errRtn;\n");
	fprintf(fp, "\t\twhile (!pAuUnit->RecvReturn_htmain(errRtn))\n");
	fprintf(fp, "\t\t\tusleep(1000);\n");
	fprintf(fp, "\n");
	fprintf(fp, "\t\terr += errRtn;\n");
	fprintf(fp, "\n");

	for (size_t modIdx = 0; modIdx < m_rndModList.size(); modIdx += 1) {
		CRndMod & rndMod = m_rndModList[modIdx];

		for (size_t fldIdx = 0; fldIdx < rndMod.m_mifRdFldList.size(); fldIdx += 1) {
			CRndFld & rndFld = *rndMod.m_mifRdFldList[fldIdx].m_pRndFld;
			int mifRdId = rndMod.m_mifRdFldList[fldIdx].m_mifRdId;
			int wrCnt = rndFld.m_wrCnt;

			CWrFldIter wrFldIter(this, rndFld);
			while (wrFldIter++) {

				int wrId = wrFldIter.m_pRndModFld->m_wrId;

				fprintf(fp, "\t\tif (pDataOut[%d] != %s)\n",
					(mifRdId * wrCnt + wrId) * 2,
					wrId == 0 ? (wrFldIter.m_bSrcWr ? "0xaaaabbbbcccc1234LL" : "0xddddeeeeffff1234LL")
					: (wrFldIter.m_bSrcWr ? "0xaaaabbbbcccc3456LL" : "0xddddeeeeffff3456LL"));
				fprintf(fp, "\t\t\terr += 1;\n");
				fprintf(fp, "\t\tif (pDataOut[%d] != %s)\n",
					(mifRdId * wrCnt + wrId) * 2 + 1,
					wrId == 0 ? (wrFldIter.m_bSrcWr ? "0xaaaabbbbcccc2345LL" : "0xddddeeeeffff2345LL")
					: (wrFldIter.m_bSrcWr ? "0xaaaabbbbcccc4567LL" : "0xddddeeeeffff4567LL"));
				fprintf(fp, "\t\t\terr += 1;\n");
				fprintf(fp, "\n");
			}
		}
	}

	fprintf(fp, "\t}\n");
	fprintf(fp, "\n");
	fprintf(fp, "\tdelete pHtHif;\n");
	fprintf(fp, "\n");
	fprintf(fp, "\tif (err > 0)\n");
	fprintf(fp, "\t\tprintf(\"FAILED\\n\");\n");
	fprintf(fp, "\telse\n");
	fprintf(fp, "\t\tprintf(\"PASSED\\n\");\n");
	fprintf(fp, "\n");
	fprintf(fp, "\treturn err;\n");
	fprintf(fp, "}\n");

	fclose(fp);
}
