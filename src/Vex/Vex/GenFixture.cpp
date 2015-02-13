/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
// Generate fixtures for each module in input vex file
//

#include "stdafx.h"
#include "Design.h"

struct CPort {
	CPort(string name, bool bIndexed, int highBit, int lowBit, int portIdx) 
		: m_name(name), m_bIndexed(bIndexed), m_highBit(highBit), m_lowBit(lowBit), m_portIdx(portIdx) {}
	string	m_name;
	bool	m_bIndexed;
	int		m_highBit;
	int		m_lowBit;
	int		m_portIdx;
};

struct CPortList {
	string			m_clock;
	vector<CPort>	m_inputs;
	vector<CPort>	m_outputs;
};

void
CDesign::GenFixture(const string &verilogFileName)
{
	static const basic_string <char>::size_type npos = -1;

	string testDir = "VexTest";

	MkDir(testDir);

	GenVexTestRunAll(testDir + "/RunAll");

	GenFixtureCommon(testDir + "/Common");

	MkDir(testDir + "/Modules");

	string primFileName = verilogFileName;
	size_t periodPos = primFileName.find_last_of(".");
	if (periodPos != npos) primFileName.erase(periodPos);
	size_t slashPos = primFileName.find_last_of("/\\");
	if (slashPos != npos) primFileName.erase(0, slashPos+1);

	ModuleMap_Iter moduleIter;
	for (moduleIter = m_moduleMap.begin(); moduleIter != m_moduleMap.end(); moduleIter++) {
		CModule *pModule = &moduleIter->second;
		if (pModule->m_bPrim)
			continue;

		GenFixtureModule(testDir + "/Modules/" + pModule->m_name.c_str(), primFileName, pModule);
	}
}

void
CDesign::GenVexTestRunAll(const string &runAllFile)
{
	FILE *fp = fopen(runAllFile.c_str(), "wb");
	if (!fp) {
		printf("Could not open '%s' for writing\n", runAllFile.c_str());
		exit(1);
	}

	fprintf(fp, "#!/bin/tcsh\n");
	fprintf(fp, "cd Common\n");
	fprintf(fp, "g++ VcdDiff.cpp -o VcdDiff\n");
	fprintf(fp, "cd ..\n");
	fprintf(fp, "cd Modules\n");
	fprintf(fp, "foreach i (*)\n");
	fprintf(fp, "cd $i\n");
	fprintf(fp, "./RunTest $i\n");
	fprintf(fp, "cd ..\n");
	fprintf(fp, "end\n");

	fclose(fp);
}

void
CDesign::GenFixtureCommon(const string &commonDir)
{

	MkDir(commonDir);

	GenFixtureCommonRandomH(commonDir + "/Random.h");
	GenFixtureCommonRandomCpp(commonDir + "/Random.cpp");
	GenFixtureCommonVcdDiff(commonDir + "/VcdDiff.cpp");
}

void
CDesign::GenFixtureCommonRandomH(const string &randomHFile)
{
	FILE *fp = fopen(randomHFile.c_str(), "w");
	if (!fp) {
		printf("Could not open '%s' for writing\n", randomHFile.c_str());
		exit(1);
	}

	fprintf(fp, "// Random.h: Header for random number generator\n");
	fprintf(fp, "//\n");
	fprintf(fp, "//////////////////////////////////////////////////////////////////////\n");
	fprintf(fp, "\n");
	fprintf(fp, "#pragma once\n");
	fprintf(fp, "\n");
	fprintf(fp, "class CRandom;\n");
	fprintf(fp, "\n");
	fprintf(fp, "class CRndGen\n");
	fprintf(fp, "{\n");
	fprintf(fp, "public:\n");
	fprintf(fp, "	CRndGen(uint32_t init=1);\n");
	fprintf(fp, "	virtual ~CRndGen();\n");
	fprintf(fp, "\n");
	fprintf(fp, "	uint32_t AsUint32();\n");
	fprintf(fp, "	uint64_t AsUint64() { return ((uint64_t)AsUint32() << 32) | AsUint32(); }\n");
	fprintf(fp, "	int64_t AsSint64() { return (int64_t)(((uint64_t)AsUint32() << 32) | AsUint32()); }\n");
	fprintf(fp, "	double AsDouble();\n");
	fprintf(fp, "	float AsFloat();\n");
	fprintf(fp, "	uint32_t GetSeed() { return initialSeed; }\n");
	fprintf(fp, "	void Seed(uint32_t init);\n");
	fprintf(fp, "\n");
	fprintf(fp, "private:\n");
	fprintf(fp, "	void ResetRndGen();\n");
	fprintf(fp, "\n");
	fprintf(fp, "private:\n");
	fprintf(fp, "	union PrivateDoubleType {	   	// used to access doubles as unsigneds\n");
	fprintf(fp, "		double d;\n");
	fprintf(fp, "		uint32_t u[2];\n");
	fprintf(fp, "	};\n");
	fprintf(fp, "	PrivateDoubleType doubleMantissa;\n");
	fprintf(fp, "\n");
	fprintf(fp, "	union PrivateFloatType {		// used to access floats as unsigneds\n");
	fprintf(fp, "		float f;\n");
	fprintf(fp, "		uint32_t u;\n");
	fprintf(fp, "	};\n");
	fprintf(fp, "	PrivateFloatType floatMantissa;\n");
	fprintf(fp, "\n");
	fprintf(fp, "	// struct defines a knot of the circular queue\n");
	fprintf(fp, "	// used by the lagged Fibonacci algorithm\n");
	fprintf(fp, "	struct Vknot {\n");
	fprintf(fp, "		Vknot() { v = 0; vnext = 0; }\n");
	fprintf(fp, "		uint32_t v;\n");
	fprintf(fp, "		Vknot *vnext;\n");
	fprintf(fp, "	} m_Vknot[97];\n");
	fprintf(fp, "	uint32_t initialSeed;  // start value of initialization routine\n");
	fprintf(fp, "	uint32_t Xn;		// generated value\n");
	fprintf(fp, "	uint32_t cn, Vn;	// temporary values\n");
	fprintf(fp, "	Vknot *V33, *V97, *Vanker;	// some pointers to the queue\n");
	fprintf(fp, "};\n");
	fprintf(fp, "\n");
	fprintf(fp, "class CRandom\n");
	fprintf(fp, "{\n");
	fprintf(fp, "	friend class CRndGen;\n");
	fprintf(fp, "public:\n");
	fprintf(fp, "	CRandom() {}\n");
	fprintf(fp, "	//	~CRandom() {};\n");
	fprintf(fp, "	void Seed(uint32_t init) { m_rndGen.Seed(init); }\n");
	fprintf(fp, "\n");
	fprintf(fp, "	bool RndBool() { return (m_rndGen.AsUint32() & 1) == 1; }\n");
	fprintf(fp, "\n");
	fprintf(fp, "	int32_t RndSint32() { return m_rndGen.AsUint32(); }\n");
	fprintf(fp, "\n");
	fprintf(fp, "	uint32_t RndUint32() { return m_rndGen.AsUint32(); }\n");
	fprintf(fp, "\n");
	fprintf(fp, "	uint32_t RndUint32(uint32_t low, uint32_t high)\n");
	fprintf(fp, "	{ return (m_rndGen.AsUint32() %% (high-low+1)) + low; }\n");
	fprintf(fp, "\n");
	fprintf(fp, "	uint64_t RndSint64(int64_t low, int64_t high)\n");
	fprintf(fp, "	{ return (m_rndGen.AsUint64() %% (high-low+1)) + low; }\n");
	fprintf(fp, "\n");
	fprintf(fp, "	uint32_t RndSint32(int32_t low,int32_t high)\n");
	fprintf(fp, "	{ return (m_rndGen.AsUint32() %% (high-low+1)) + low; }\n");
	fprintf(fp, "\n");
	fprintf(fp, "	uint64_t RndUint64() { return m_rndGen.AsUint64(); }\n");
	fprintf(fp, "\n");
	fprintf(fp, "	uint64_t RndUint64(uint64_t low, uint64_t high)\n");
	fprintf(fp, "	{ return (m_rndGen.AsUint64() %% (high-low+1)) + low; }\n");
	fprintf(fp, "\n");
	fprintf(fp, "	double RndUniform(double pLow=0.0, double pHigh=1.0)\n");
	fprintf(fp, "	{ return pLow + (pHigh - pLow) * m_rndGen.AsDouble(); }\n");
	fprintf(fp, "\n");
	fprintf(fp, "	float RndUniform(float pLow, float pHigh=1.0)\n");
	fprintf(fp, "	{ return pLow + (pHigh - pLow) * m_rndGen.AsFloat(); }\n");
	fprintf(fp, "\n");
	fprintf(fp, "	double RndPercent() { return RndUniform(0.0, 99.999999999); }\n");
	fprintf(fp, "\n");
	fprintf(fp, "	//	double RndNegExp(double mean)\n");
	fprintf(fp, "	//	{ return(-mean * log(m_rndGen.AsDouble())); }\n");
	fprintf(fp, "\n");
	fprintf(fp, "private:\n");
	fprintf(fp, "	CRndGen m_rndGen;\n");
	fprintf(fp, "};\n");

	fclose(fp);
}

void
CDesign::GenFixtureCommonRandomCpp(const string &randomCppFile)
{
	FILE *fp = fopen(randomCppFile.c_str(), "w");
	if (!fp) {
		printf("Could not open '%s' for writing\n", randomCppFile.c_str());
		exit(1);
	}

	fprintf(fp, "// Random.cpp: Random number generator for fixture\n");
	fprintf(fp, "//\n");
	fprintf(fp, "//////////////////////////////////////////////////////////////////////\n");
	fprintf(fp, "\n");
	fprintf(fp, "#include <stdint.h>\n");
	fprintf(fp, "#include \"Random.h\"\n");
	fprintf(fp, "\n");
	fprintf(fp, "//////////////////////////////////////////////////////////////////////\n");
	fprintf(fp, "// Construction/Destruction\n");
	fprintf(fp, "//////////////////////////////////////////////////////////////////////\n");
	fprintf(fp, "\n");
	fprintf(fp, "CRndGen::CRndGen(uint32_t init)\n");
	fprintf(fp, "{\n");
	fprintf(fp, "	// create a 97 element circular queue\n");
	fprintf(fp, "	Vknot *Vhilf;\n");
	fprintf(fp, "	Vanker = V97 = Vhilf = &m_Vknot[0];\n");
	fprintf(fp, "\n");
	fprintf(fp, "	for(int16_t i=96; i>0; i-- ) {\n");
	fprintf(fp, "		Vhilf->vnext = &m_Vknot[i];\n");
	fprintf(fp, "		Vhilf = Vhilf->vnext;\n");
	fprintf(fp, "		if (i == 33)\n");
	fprintf(fp, "			V33 = Vhilf;\n");
	fprintf(fp, "	}\n");
	fprintf(fp, "\n");
	fprintf(fp, "	Vhilf->vnext = Vanker;\n");
	fprintf(fp, "\n");
	fprintf(fp, "	initialSeed = init;\n");
	fprintf(fp, "\n");
	fprintf(fp, "	ResetRndGen();\n");
	fprintf(fp, "\n");
	fprintf(fp, "	// initialize mantissa to all ones\n");
	fprintf(fp, "	doubleMantissa.d = 1.0;\n");
	fprintf(fp, "	doubleMantissa.u[0] ^= 0xffffffff;\n");
	fprintf(fp, "	doubleMantissa.u[1] ^= 0x3fffffff;\n");
	fprintf(fp, "\n");
	fprintf(fp, "	// initialize mantissa to all ones\n");
	fprintf(fp, "	floatMantissa.f = 1.0;\n");
	fprintf(fp, "	floatMantissa.u ^= 0x3fffffff;\n");
	fprintf(fp, "}\n");
	fprintf(fp, "\n");
	fprintf(fp, "CRndGen::~CRndGen()\n");
	fprintf(fp, "{\n");
	fprintf(fp, "}\n");
	fprintf(fp, "\n");
	fprintf(fp, "void\n");
	fprintf(fp, "CRndGen::Seed(uint32_t init)\n");
	fprintf(fp, "{\n");
	fprintf(fp, "	initialSeed = init;\n");
	fprintf(fp, "\n");
	fprintf(fp, "	ResetRndGen();\n");
	fprintf(fp, "}\n");
	fprintf(fp, "\n");
	fprintf(fp, "void\n");
	fprintf(fp, "CRndGen::ResetRndGen()\n");
	fprintf(fp, "{\n");
	fprintf(fp, "	// initialise FIBOG like RANMAR (or IMAV)\n");
	fprintf(fp, "	Vknot *Vhilf = V97;\n");
	fprintf(fp, "\n");
	fprintf(fp, "	int32_t ijkl = 54217137;\n");
	fprintf(fp, "	if (initialSeed >0 && initialSeed <= 0x7fffffff) ijkl = initialSeed;\n");
	fprintf(fp, "	int32_t ij = ijkl / 30082;\n");
	fprintf(fp, "	int32_t kl = ijkl - 30082 * ij;\n");
	fprintf(fp, "	int32_t i = (ij/177) %% 177 + 2;\n");
	fprintf(fp, "	int32_t j = ij %% 177 + 2;\n");
	fprintf(fp, "	int32_t k = (kl/169) %% 178 + 1;\n");
	fprintf(fp, "	int32_t l = kl %% 169;\n");
	fprintf(fp, "	int32_t s, t;\n");
	fprintf(fp, "	for(int32_t ii=0; ii<97; ii++) {\n");
	fprintf(fp, "		s = 0; t = 0x80000000;\n");
	fprintf(fp, "		for(int32_t jj=0; jj<32; jj++) {\n");
	fprintf(fp, "			int32_t m = (((i * j) %% 179) * k) %% 179;\n");
	fprintf(fp, "			i = j; j = k; k = m;\n");
	fprintf(fp, "			l = (53 * l + 1) %% 169;\n");
	fprintf(fp, "			if ((l * m) %% 64 >= 32) s = s + t;\n");
	fprintf(fp, "			t /= 2;\n");
	fprintf(fp, "		}\n");
	fprintf(fp, "\n");
	fprintf(fp, "		Vhilf->v = s;\n");
	fprintf(fp, "		Vhilf = Vhilf->vnext;\n");
	fprintf(fp, "	}\n");
	fprintf(fp, "\n");
	fprintf(fp, "	cn = 15362436;\n");
	fprintf(fp, "}\n");
	fprintf(fp, "\n");
	fprintf(fp, "uint32_t\n");
	fprintf(fp, "CRndGen::AsUint32()\n");
	fprintf(fp, "{\n");
	fprintf(fp, "	Vn = (V97->v - V33->v) & 0xffffffff;	// (x & 2^32) might be faster  \n");
	fprintf(fp, "	cn = (cn + 362436069)  & 0xffffffff;	// than (x ^32)\n");
	fprintf(fp, "	Xn = (Vn - cn)         & 0xffffffff;\n");
	fprintf(fp, "\n");
	fprintf(fp, "	V97->v = Vn;\n");
	fprintf(fp, "	V97 = V97->vnext;\n");
	fprintf(fp, "	V33 = V33->vnext;\n");
	fprintf(fp, "\n");
	fprintf(fp, "	return Xn;\n");
	fprintf(fp, "}\n");
	fprintf(fp, "\n");
	fprintf(fp, "double\n");
	fprintf(fp, "CRndGen::AsDouble()\n");
	fprintf(fp, "{\n");
	fprintf(fp, "	PrivateDoubleType result;\n");
	fprintf(fp, "	result.d = 1.0;\n");
	fprintf(fp, "	result.u[1] |= (AsUint32() & doubleMantissa.u[1]);\n");
	fprintf(fp, "	result.u[0] |= (AsUint32() & doubleMantissa.u[0]);\n");
	fprintf(fp, "	result.d -= 1.0;\n");
	fprintf(fp, "	return( result.d );\n");
	fprintf(fp, "}\n");
	fprintf(fp, "\n");
	fprintf(fp, "float\n");
	fprintf(fp, "CRndGen::AsFloat()\n");
	fprintf(fp, "{\n");
	fprintf(fp, "	PrivateFloatType result;\n");
	fprintf(fp, "	result.f = 1.0;\n");
	fprintf(fp, "	result.u |= (AsUint32() & floatMantissa.u);\n");
	fprintf(fp, "	result.f -= 1.0;\n");
	fprintf(fp, "	return( result.f );\n");
	fprintf(fp, "}\n");

	fclose(fp);
}

void
CDesign::GenFixtureCommonVcdDiff(const string &vcdDiffFile)
{
	FILE *fp = fopen(vcdDiffFile.c_str(), "w");
	if (!fp) {
		printf("Could not open '%s' for writing\n", vcdDiffFile.c_str());
		exit(1);
	}

	fprintf(fp, "#include <stdio.h>\n");
	fprintf(fp, "#include <stdlib.h>\n");
	fprintf(fp, "#include <stdint.h>\n");
	fprintf(fp, "\n");
	fprintf(fp, "#include <string>\n");
	fprintf(fp, "#include <vector>\n");
	fprintf(fp, "#include <map>\n");
	fprintf(fp, "\n");
	fprintf(fp, "using namespace std;\n");
	fprintf(fp, "\n");
	fprintf(fp, "struct CSignal {\n");
	fprintf(fp, "	CSignal(string name, int width) : m_name(name), m_width(width) {}\n");
	fprintf(fp, "\n");
	fprintf(fp, "	string	m_name;\n");
	fprintf(fp, "	int		m_width;\n");
	fprintf(fp, "	string	m_value;\n");
	fprintf(fp, "};\n");
	fprintf(fp, "\n");
	fprintf(fp, "typedef map<string, int>		SigIdMap;\n");
	fprintf(fp, "typedef pair <string, int>		SigIdPair;\n");
	fprintf(fp, "\n");
	fprintf(fp, "void GetSignals(FILE *fp, vector<CSignal> &sv, SigIdMap &hv);\n");
	fprintf(fp, "bool AdvTime(int cmpTime, int &curTime, char *line, FILE *fp, vector<CSignal> &sv, SigIdMap &hv);\n");
	fprintf(fp, "bool UpdateSignal(char *line, vector<CSignal> &sv, SigIdMap &hv);\n");
	fprintf(fp, "void GetCmpList(vector<int> &cmpList, vector<CSignal> &sv1, vector<CSignal> &sv2);\n");
	fprintf(fp, "void CmpSignals(int cmpTime, vector<int> &cmpList, vector<CSignal> &sv1, vector<CSignal> &sv2);\n");
	fprintf(fp, "\n");
	fprintf(fp, "string moduleName;\n");
	fprintf(fp, "\n");
	fprintf(fp, "int main(int argc, char **argv)\n");
	fprintf(fp, "{\n");
	fprintf(fp, "	if (argc != 4) {\n");
	fprintf(fp, "		fprintf(stderr, \"VcdDiff ModuleName VcdFile1 VcdFile2\\n\");\n");
	fprintf(fp, "		exit(1);\n");
	fprintf(fp, "	}\n");
	fprintf(fp, "\n");
	fprintf(fp, "	moduleName = argv[1];\n");
	fprintf(fp, "\n");
	fprintf(fp, "	int argIdx = 2;\n");
	fprintf(fp, "\n");
	fprintf(fp, "	FILE *fp1 = fopen(argv[argIdx], \"r\");\n");
	fprintf(fp, "	if (!fp1) {\n");
	fprintf(fp, "		printf(\"%%s: Could not open '%%s' for reading\\n\", moduleName.c_str(), argv[argIdx]);\n");
	fprintf(fp, "		exit(1);\n");
	fprintf(fp, "	}\n");
	fprintf(fp, "	argIdx += 1;\n");
	fprintf(fp, "\n");
	fprintf(fp, "	FILE *fp2 = fopen(argv[argIdx], \"r\");\n");
	fprintf(fp, "	if (!fp2) {\n");
	fprintf(fp, "		printf(\"%%s: Could not open '%%s' for reading\\n\", moduleName.c_str(), argv[argIdx]);\n");
	fprintf(fp, "		exit(1);\n");
	fprintf(fp, "	}\n");
	fprintf(fp, "	argIdx += 1;\n");
	fprintf(fp, "\n");
	fprintf(fp, "	vector<CSignal> sv1;\n");
	fprintf(fp, "	SigIdMap hm1;\n");
	fprintf(fp, "	GetSignals(fp1, sv1, hm1);\n");
	fprintf(fp, "\n");
	fprintf(fp, "	vector<CSignal> sv2;\n");
	fprintf(fp, "	SigIdMap hm2;\n");
	fprintf(fp, "	GetSignals(fp2, sv2, hm2);\n");
	fprintf(fp, "\n");
	fprintf(fp, "	vector<int>	cmpList;\n");
	fprintf(fp, "	GetCmpList(cmpList, sv1, sv2);\n");
	fprintf(fp, "\n");
	fprintf(fp, "	// now start comparing\n");
	fprintf(fp, "	int curTime1 = 0;\n");
	fprintf(fp, "	int curTime2 = 0;\n");
	fprintf(fp, "	char line1[256] = { '\\0' };\n");
	fprintf(fp, "	char line2[256] = { '\\0' };\n");
	fprintf(fp, "\n");
	fprintf(fp, "	int cmpTime = 168000;\n");
	fprintf(fp, "	for (;;) {\n");
	fprintf(fp, "		if (!AdvTime(cmpTime, curTime1, line1, fp1, sv1, hm1) || !AdvTime(cmpTime, curTime2, line2, fp2, sv2, hm2))\n");
	fprintf(fp, "			break;\n");
	fprintf(fp, "\n");
	fprintf(fp, "		// compare signal values\n");
	fprintf(fp, "		CmpSignals(cmpTime, cmpList, sv1, sv2);\n");
	fprintf(fp, "\n");
	fprintf(fp, "		cmpTime += 8000;\n");
	fprintf(fp, "\n");
	fprintf(fp, "		if (!AdvTime(cmpTime, curTime1, line1, fp1, sv1, hm1) || !AdvTime(cmpTime, curTime2, line2, fp2, sv2, hm2))\n");
	fprintf(fp, "			break;\n");
	fprintf(fp, "\n");
	fprintf(fp, "		CmpSignals(cmpTime, cmpList, sv1, sv2);\n");
	fprintf(fp, "\n");
	fprintf(fp, "		cmpTime += 2000;\n");
	fprintf(fp, "	}\n");
	fprintf(fp, "\n");
	fprintf(fp, "	printf(\"%%s: Test Passed - final time %%d\\n\", argv[1], curTime1);\n");
	fprintf(fp, "}\n");
	fprintf(fp, "\n");
	fprintf(fp, "void GetSignals(FILE *fp, vector<CSignal> &sv, SigIdMap &hm)\n");
	fprintf(fp, "{\n");
	fprintf(fp, "	char line[256];\n");
	fprintf(fp, "	char *lp;\n");
	fprintf(fp, "	while (lp = fgets(line, 256, fp)) {\n");
	fprintf(fp, "		if (strncmp(lp, \"$scope\", 6) == 0)\n");
	fprintf(fp, "			break;\n");
	fprintf(fp, "	}\n");
	fprintf(fp, "\n");
	fprintf(fp, "	for (int sigIdx = 0; lp = fgets(line, 256, fp); sigIdx += 1) {\n");
	fprintf(fp, "		if (strncmp(lp, \"$var\", 4) != 0)\n");
	fprintf(fp, "			break;\n");
	fprintf(fp, "\n");
	fprintf(fp, "		char typeStr[32];\n");
	fprintf(fp, "		int width;\n");
	fprintf(fp, "		char idStr[32];\n");
	fprintf(fp, "		char nameStr[128];\n");
	fprintf(fp, "		char endStr[32];\n");
	fprintf(fp, "\n");
	fprintf(fp, "		if (sscanf(line, \"$var %%s %%d %%s %%s %%s\", typeStr, &width, idStr, nameStr, endStr) != 5) {\n");
	fprintf(fp, "			printf(\"%%s: Unknown $var format: %%s\", moduleName.c_str(), line);\n");
	fprintf(fp, "			exit(1);\n");
	fprintf(fp, "		}\n");
	fprintf(fp, "\n");
	fprintf(fp, "		sv.push_back(CSignal(nameStr, width));\n");
	fprintf(fp, "		hm.insert( SigIdPair(idStr, sigIdx) );\n");
	fprintf(fp, "	}\n");
	fprintf(fp, "}\n");
	fprintf(fp, "\n");
	fprintf(fp, "bool AdvTime(int advTime, int &curTime, char *line, FILE *fp, vector<CSignal> &sv, SigIdMap &hv)\n");
	fprintf(fp, "{\n");
	fprintf(fp, "	char *lp;\n");
	fprintf(fp, "	for (;;) {\n");
	fprintf(fp, "		if (strncmp(line, \"$dumpvars\", 9) == 0) {\n");
	fprintf(fp, "			while ((lp = fgets(line, 256, fp)) && UpdateSignal(line, sv, hv));\n");
	fprintf(fp, "			if (lp == 0) return false;\n");
	fprintf(fp, "			if (strncmp(line, \"$end\", 4) != 0) {\n");
	fprintf(fp, "				printf(\"%%s: Unknown line format\\n\", moduleName.c_str());\n");
	fprintf(fp, "				exit(1);\n");
	fprintf(fp, "			}\n");
	fprintf(fp, "		} else if (line[0] == '#') {\n");
	fprintf(fp, "			sscanf(line, \"#%%d\", &curTime);\n");
	fprintf(fp, "			if (curTime == 195100)\n");
	fprintf(fp, "				bool stop = true;\n");
	fprintf(fp, "			if (curTime >= advTime)\n");
	fprintf(fp, "				return true;\n");
	fprintf(fp, "			while ((lp = fgets(line, 256, fp)) && UpdateSignal(line, sv, hv));\n");
	fprintf(fp, "			if (!lp) return false;\n");
	fprintf(fp, "			continue;\n");
	fprintf(fp, "		} else if (strncmp(line, \"$comment\", 8) == 0) {\n");
	fprintf(fp, "			while (fgets(line, 256, fp) && strncmp(line, \"$end\", 4) != 0);\n");
	fprintf(fp, "		} else if (strncmp(line, \"$end\", 4) != 0 && line[0] != '\\n' && line[0] != '\\r' && line[0] != '\\0') {\n");
	fprintf(fp, "			printf(\"%%s: Unknown line format\\n\", moduleName.c_str());\n");
	fprintf(fp, "			exit(1);\n");
	fprintf(fp, "		}\n");
	fprintf(fp, "\n");
	fprintf(fp, "		fgets(line, 256, fp);\n");
	fprintf(fp, "	};\n");
	fprintf(fp, "\n");
	fprintf(fp, "	return true;\n");
	fprintf(fp, "}\n");
	fprintf(fp, "\n");
	fprintf(fp, "bool UpdateSignal(char *line, vector<CSignal> &sv, SigIdMap &hm)\n");
	fprintf(fp, "{\n");
	fprintf(fp, "	char *lp = line;\n");
	fprintf(fp, "	char num[256];\n");
	fprintf(fp, "	char *np = num;\n");
	fprintf(fp, "	char id[256];\n");
	fprintf(fp, "	char *ip = id;\n");
	fprintf(fp, "	SigIdMap :: const_iterator hmIter;\n");
	fprintf(fp, "\n");
	fprintf(fp, "	if (*lp == 'b' || *lp == '1' || *lp == '0' || *lp == 'x' || *lp == 'z') {\n");
	fprintf(fp, "		// binary\n");
	fprintf(fp, "		if (*lp == 'b') lp += 1;\n");
	fprintf(fp, "\n");
	fprintf(fp, "		// skip leading zeros\n");
	fprintf(fp, "		if (*lp == '0') {\n");
	fprintf(fp, "			while (*lp == '0') lp += 1;\n");
	fprintf(fp, "			if (*lp != '1' && *lp != 'z' && *lp != 'x')\n");
	fprintf(fp, "				lp -= 1;\n");
	fprintf(fp, "		}\n");
	fprintf(fp, "		while (*lp == '0' || *lp == '1' || *lp == 'z' || *lp == 'x')\n");
	fprintf(fp, "			*np++ = *lp++;\n");
	fprintf(fp, "		*np = '\\0';\n");
	fprintf(fp, "	} else \n");
	fprintf(fp, "		return false;\n");
	fprintf(fp, "\n");
	fprintf(fp, "	while (*lp == ' ') lp += 1;\n");
	fprintf(fp, "\n");
	fprintf(fp, "	while (*lp != '\\n' && *lp != '\\r' && *lp != ' ' && *lp != '\\0')\n");
	fprintf(fp, "		*ip++ = *lp++;\n");
	fprintf(fp, "	*ip = '\\0';\n");
	fprintf(fp, "\n");
	fprintf(fp, "	hmIter = hm.find( id );\n");
	fprintf(fp, "\n");
	fprintf(fp, "	if (hmIter == hm.end()) {\n");
	fprintf(fp, "		printf(\"%%s: Did not find Id String\\n\", moduleName.c_str());\n");
	fprintf(fp, "		exit(1);\n");
	fprintf(fp, "	}\n");
	fprintf(fp, "\n");
	fprintf(fp, "	int sigIdx = hmIter->second;\n");
	fprintf(fp, "\n");
	fprintf(fp, "	sv[sigIdx].m_value = num;\n");
	fprintf(fp, "\n");
	fprintf(fp, "	return true;\n");
	fprintf(fp, "}\n");
	fprintf(fp, "\n");
	fprintf(fp, "void GetCmpList(vector<int> &cmpList, vector<CSignal> &sv1, vector<CSignal> &sv2)\n");
	fprintf(fp, "{\n");
	fprintf(fp, "	for (size_t i = 0; i < sv1.size(); i += 1)\n");
	fprintf(fp, "		for (size_t j = 0; j < sv2.size(); j += 1)\n");
	fprintf(fp, "			if (sv1[i].m_name == sv2[j].m_name) {\n");
	fprintf(fp, "				cmpList.push_back(j);\n");
	fprintf(fp, "				break;\n");
	fprintf(fp, "			}\n");
	fprintf(fp, "}\n");
	fprintf(fp, "\n");
	fprintf(fp, "void CmpSignals(int cmpTime, vector<int> &cmpList, vector<CSignal> &sv1, vector<CSignal> &sv2)\n");
	fprintf(fp, "{\n");
	fprintf(fp, "	for (size_t i = 0; i < cmpList.size(); i += 1)\n");
	fprintf(fp, "		if (sv1[i].m_value != sv2[cmpList[i]].m_value) {\n");
	fprintf(fp, "			printf(\"%%s: Trace mismatch found at time %%d, signal %%s\\n\", moduleName.c_str(), cmpTime, sv1[i].m_name.c_str());\n");
	fprintf(fp, "			exit(1);\n");
	fprintf(fp, "		}\n");
	fprintf(fp, "}\n");

	fclose(fp);
}

void
CDesign::GenFixtureModule(const string &modDir, const string &primFileName, CModule *pModule)
{

	MkDir(modDir);

	CPortList	portList;
	GetModulePorts(pModule, portList);

	GenFixtureModuleRunTest(modDir + "/RunTest", pModule);

	GenFixtureModuleSystemC(modDir + "/SystemC", primFileName, pModule, portList);

	GenFixtureModuleVerilog(modDir + "/Verilog", primFileName, pModule, portList);
}

void
CDesign::GetModulePorts(CModule *pModule, CPortList &portList)
{
	SignalMap_Iter sigIter;
	for (sigIter = pModule->m_signalMap.begin(); sigIter != pModule->m_signalMap.end(); sigIter++) {
		CSignal *pSignal = sigIter->second;

		if (pSignal->m_sigType == sig_input) {
			if (pSignal->m_name == "ck" || pSignal->m_name == "clk")
				portList.m_clock = pSignal->m_name;
			else
				portList.m_inputs.push_back(CPort(pSignal->m_name, pSignal->m_bIndexed, pSignal->m_upperIdx, pSignal->m_lowerIdx, pSignal->m_portIdx));
		}
	}

	for (sigIter = pModule->m_signalMap.begin(); sigIter != pModule->m_signalMap.end(); sigIter++) {
		CSignal *pSignal = sigIter->second;

		if (pSignal->m_sigType == sig_output)
			portList.m_outputs.push_back(CPort(pSignal->m_name, pSignal->m_bIndexed, pSignal->m_upperIdx, pSignal->m_lowerIdx, pSignal->m_portIdx));
	}
}

void 
CDesign::GenFixtureModuleRunTest(const string &runTestFile, CModule *pModule)
{
	FILE *fp = fopen(runTestFile.c_str(), "wb");
	if (!fp) {
		printf("Could not open '%s' for writing\n", runTestFile.c_str());
		exit(1);
	}

	fprintf(fp, "#!/bin/sh\n");
	fprintf(fp, "cd Verilog; ./Verilog.script >& Verilog.log; cd ..\n");
	fprintf(fp, "cd SystemC; ./SystemC.script >& SystemC.log; cd ..\n");
	fprintf(fp, "../../Common/VcdDiff %s SystemC/SystemC.vcd Verilog/Verilog.vcd | tee RunTest.out\n", pModule->m_name.c_str());

	fclose(fp);
}

void
CDesign::GenFixtureModuleSystemC(const string &systemcDir, const string &primFileName, CModule *pModule, CPortList &portList)
{
	MkDir(systemcDir);

	GenFixtureModuleSystemCScript(systemcDir + "/SystemC.script");
	GenFixtureModuleSystemCMainCpp(systemcDir + "/Main.cpp", primFileName, pModule, portList);
}

void
CDesign::GenFixtureModuleSystemCScript(const string &systemcScript)
{
	FILE *fp = fopen(systemcScript.c_str(), "wb");
	if (!fp) {
		printf("Could not open '%s' for writing\n", systemcScript.c_str());
		exit(1);
	}

	fprintf(fp, "#!/bin/sh\n");
	fprintf(fp, "g++ -m64 -I ~/SystemC/systemc-2.2.0/src ../../../Common/Random.cpp Main.cpp ~/SystemC/systemc-2.2.0/lib-linux64/libsystemc.a -o systemc_sim\n");
	fprintf(fp, "\n");
	fprintf(fp, "./systemc_sim\n");

	fclose(fp);
}

void
CDesign::GenFixtureModuleSystemCMainCpp(const string &mainCppFile, const string &primFileName, CModule *pModule, CPortList &portList)
{
	FILE *fp = fopen(mainCppFile.c_str(), "wb");
	if (!fp) {
		printf("Could not open '%s' for writing\n", mainCppFile.c_str());
		exit(1);
	}

	fprintf(fp, "#include <stdio.h>\n");
	fprintf(fp, "#include <stdlib.h>\n");
	fprintf(fp, "#include <stdint.h>\n");
	fprintf(fp, "\n");
	fprintf(fp, "#include <SystemC.h>\n");
	fprintf(fp, "#include \"../../../Common/Random.h\"\n");
	fprintf(fp, "\n");
	fprintf(fp, "CRandom cnyRnd;\n");
	fprintf(fp, "\n");
	fprintf(fp, "sc_trace_file * scVcdFp = sc_create_vcd_trace_file(\"SystemC\");\n");
	fprintf(fp, "\n");
	fprintf(fp, "#include \"../../../%s.h\"\n", primFileName.c_str());
	fprintf(fp, "\n");
	fprintf(fp, "int\n");
	fprintf(fp, "sc_main(int argc, char *argv[])\n");
	fprintf(fp, "{\n");
	if (portList.m_clock.size() > 0)
		fprintf(fp, "	sc_uint<1>	%s;\n", portList.m_clock.c_str());
	for (size_t i = 0; i < portList.m_inputs.size(); i += 1)
		fprintf(fp, "	sc_uint<%d>	%s;\n",
			portList.m_inputs[i].m_highBit - portList.m_inputs[i].m_lowBit + 1, portList.m_inputs[i].m_name.c_str());
	for (size_t i = 0; i < portList.m_outputs.size(); i += 1)
		fprintf(fp, "	sc_uint<%d>	%s;\n",
			portList.m_outputs[i].m_highBit - portList.m_outputs[i].m_lowBit + 1, portList.m_outputs[i].m_name.c_str());
	fprintf(fp, "\n");
	if (portList.m_clock.size() > 0)
		fprintf(fp, "	sc_trace(scVcdFp, %s, \"%s\");\n", portList.m_clock.c_str(), portList.m_clock.c_str());
	for (size_t i = 0; i < portList.m_inputs.size(); i += 1)
		fprintf(fp, "	sc_trace(scVcdFp, %s, \"%s\");\n", portList.m_inputs[i].m_name.c_str(), portList.m_inputs[i].m_name.c_str());
	for (size_t i = 0; i < portList.m_outputs.size(); i += 1)
		fprintf(fp, "	sc_trace(scVcdFp, %s, \"%s\");\n", portList.m_outputs[i].m_name.c_str(), portList.m_outputs[i].m_name.c_str());
	fprintf(fp, "\n");
	fprintf(fp, "	for (int testCnt = 0; testCnt < 10000; testCnt += 1) {\n");
	fprintf(fp, "\n");
	fprintf(fp, "		sc_start(sc_time(2.5, SC_NS));\n");
	fprintf(fp, "\n");
	for (size_t i = 0; i < portList.m_inputs.size(); i += 1)
		fprintf(fp, "		%s = cnyRnd.RndUint64();\n", portList.m_inputs[i].m_name.c_str());
	fprintf(fp, "\n");
	if (portList.m_clock.size() > 0) {
		fprintf(fp, "		sc_start(sc_time(2.5, SC_NS));\n");
		fprintf(fp, "\n");
		fprintf(fp, "		ck = 1;\n");
		fprintf(fp, "\n");
	}
	fprintf(fp, "		%s(", pModule->m_name.c_str());
	size_t portCnt = portList.m_inputs.size() + portList.m_outputs.size();
	char *pSeparater = " ";
	for (size_t i = 0; i < portCnt; i += 1) {
		bool bFound = false;
		for (size_t j = 0; j < portList.m_inputs.size() && !bFound; j += 1) {
			if ((int)i == portList.m_inputs[j].m_portIdx) {
				bFound = true;
				fprintf(fp, "%s%s", pSeparater, portList.m_inputs[j].m_name.c_str());
			}
		}
		for (size_t j = 0; j < portList.m_outputs.size() && !bFound; j += 1) {
			if ((int)i == portList.m_outputs[j].m_portIdx) {
				bFound = true;
				fprintf(fp, "%s%s", pSeparater, portList.m_outputs[j].m_name.c_str());
			}
		}
		pSeparater = ", ";
	}
	fprintf(fp, " );\n");

	fprintf(fp, "\n");
	if (portList.m_clock.size() > 0)
		fprintf(fp, "		sc_start(sc_time(5, SC_NS));\n");
	else
		fprintf(fp, "		sc_start(sc_time(7.5, SC_NS));\n");
	if (portList.m_clock.size() > 0)
		fprintf(fp, "		ck = 0;\n");
	fprintf(fp, "\n");
	fprintf(fp, "	}\n");
	fprintf(fp, "\n");
	fprintf(fp, "	return 0;\n");
	fprintf(fp, "}\n");

	fclose(fp);
}

void 
CDesign::GenFixtureModuleVerilog(const string &verilogDir, const string &primFileName, CModule *pModule, CPortList &portList)
{
	MkDir(verilogDir);

	GenFixtureModuleVerilogScript(verilogDir + "/Verilog.script", primFileName);
	GenFixtureModuleVerilogSimvCmds(verilogDir + "/simv.cmds");
	GenFixtureModuleVerilogPliTab(verilogDir + "/pli.tab");
	GenFixtureModuleVerilogFixtureV(verilogDir + "/Fixture.v", pModule, portList);
	GenFixtureModuleVerilogMainCpp(verilogDir + "/Main.cpp", pModule, portList);
}

void
CDesign::GenFixtureModuleVerilogScript(const string &vcsScriptDir, const string &primFileName)
{
	FILE *fp = fopen(vcsScriptDir.c_str(), "wb");
	if (!fp) {
		printf("Could not open '%s' for writing\n", vcsScriptDir.c_str());
		exit(1);
	}

	fprintf(fp, "#!/bin/sh\n");
	fprintf(fp, "vcs Fixture.v ../../../%s.v Main.cpp ../../../Common/Random.cpp "
		"-debug -P pli.tab +acc+3 +libext+.v +v2k -y /hw/tools/xilinx/13.2/ISE/verilog/src/unisims "
		"/hw/tools/xilinx/13.2/ISE/verilog/src/glbl.v -cpp /usr/bin/c++\n", primFileName.c_str() );
	fprintf(fp, "\n");
	fprintf(fp, "./simv -ucli -do simv.cmds\n");

	fclose(fp);
}

void
CDesign::GenFixtureModuleVerilogSimvCmds(const string &simvCmdsFile)
{
	FILE *fp = fopen(simvCmdsFile.c_str(), "wb");
	if (!fp) {
		printf("Could not open '%s' for writing\n", simvCmdsFile.c_str());
		exit(1);
	}

	fprintf(fp, "run 10000000\n");
	fprintf(fp, "exit\n");

	fclose(fp);
}

void
CDesign::GenFixtureModuleVerilogPliTab(const string &pliTabDir)
{
	FILE *fp = fopen(pliTabDir.c_str(), "w");
	if (!fp) {
		printf("Could not open '%s' for writing\n", pliTabDir.c_str());
		exit(1);
	}

	fprintf(fp, "$Fixture size=0 call=InitFixture acc=rw,wn:*\n");

	fclose(fp);
}

void
CDesign::GenFixtureModuleVerilogFixtureV(const string &fixtureVFile, CModule *pModule, CPortList &portList)
{
	FILE *fp = fopen(fixtureVFile.c_str(), "w");
	if (!fp) {
		printf("Could not open '%s' for writing\n", fixtureVFile.c_str());
		exit(1);
	}

	fprintf(fp, "`timescale 1ns / 1ps\n");
	fprintf(fp, "\n");
	fprintf(fp, "module Fixture ( );\n");
	fprintf(fp, "\n");
	if (portList.m_clock.size() > 0)
		fprintf(fp, "  reg               %s;\n", portList.m_clock.c_str());
	fprintf(fp, "  reg [1:0]       phase;\n");
	char widthStr[32];
	for (size_t i = 0; i < portList.m_inputs.size(); i += 1) {
		bool bWidth = portList.m_inputs[i].m_bIndexed;
		if (bWidth && portList.m_inputs[i].m_highBit > 31) {
			for (int lowBit = 0; lowBit < portList.m_inputs[i].m_highBit; lowBit += 32) {
				int highBit = lowBit+31 < portList.m_inputs[i].m_highBit ? lowBit+31 : portList.m_inputs[i].m_highBit;
				sprintf(widthStr, "[%d:%d]", highBit, lowBit);
				fprintf(fp, "  wire %-8s   %s_%d_%d;\n", widthStr, portList.m_inputs[i].m_name.c_str(), highBit, lowBit);
			}
		}
		sprintf(widthStr, bWidth ? "[%d:%d]" : "", portList.m_inputs[i].m_highBit, portList.m_inputs[i].m_lowBit);
		fprintf(fp, "  wire %-8s   %s;\n", widthStr, portList.m_inputs[i].m_name.c_str());
	}
	for (size_t i = 0; i < portList.m_outputs.size(); i += 1) {
		bool bWidth = portList.m_outputs[i].m_bIndexed;
		sprintf(widthStr, bWidth ? "[%d:%d]" : "", portList.m_outputs[i].m_highBit, portList.m_outputs[i].m_lowBit);
		fprintf(fp, "  wire %-8s   %s;\n", widthStr, portList.m_outputs[i].m_name.c_str());
	}
	fprintf(fp, "\n");

	for (size_t i = 0; i < portList.m_inputs.size(); i += 1) {
		bool bWidth = portList.m_inputs[i].m_bIndexed;
		if (bWidth && portList.m_inputs[i].m_highBit > 31) {
			fprintf(fp, "  assign %s = { %s_%d_%d", portList.m_inputs[i].m_name.c_str(), portList.m_inputs[i].m_name.c_str(),
				portList.m_inputs[i].m_highBit, portList.m_inputs[i].m_highBit & ~0x1f /* ul */);
			for (int lowBit = (portList.m_inputs[i].m_highBit & ~0x1ful)-32; lowBit >= 0; lowBit -= 32)
				fprintf(fp, ", %s_%d_%d", portList.m_inputs[i].m_name.c_str(), lowBit+31, lowBit);
			fprintf(fp, " };\n");
		}
	}

	fprintf(fp, "\n");
	fprintf(fp, "  initial begin\n");
	fprintf(fp, "    $dumpfile(\"Verilog.vcd\");\n");
	if (portList.m_clock.size() > 0)
		fprintf(fp, "    $dumpvars(0, %s);\n", portList.m_clock.c_str());
	for (size_t i = 0; i < portList.m_inputs.size(); i += 1)
		fprintf(fp, "    $dumpvars(0, %s);\n", portList.m_inputs[i].m_name.c_str());
	for (size_t i = 0; i < portList.m_outputs.size(); i += 1)
		fprintf(fp, "    $dumpvars(0, %s);\n", portList.m_outputs[i].m_name.c_str());
	fprintf(fp, "  end\n");
	fprintf(fp, "\n");
	fprintf(fp, "  initial begin\n");
	if (portList.m_clock.size() > 0)
		fprintf(fp, "    %s = 0;\n", portList.m_clock.c_str());
	fprintf(fp, "    phase = 0;\n");
	fprintf(fp, "  end\n");
	fprintf(fp, "\n");
	fprintf(fp, "  always begin\n");
	fprintf(fp, "    #2.5 phase = phase + 1;\n");
	fprintf(fp, "  end\n");
	fprintf(fp, "\n");
	if (portList.m_clock.size() > 0) {
		fprintf(fp, "  always begin\n");
		fprintf(fp, "    #5 %s = ~%s;\n", portList.m_clock.c_str(), portList.m_clock.c_str());
		fprintf(fp, "  end\n");
		fprintf(fp, "\n");
	}
	fprintf(fp, "  initial $Fixture (\n");
	fprintf(fp, "    phase");
	char *pSeparater = ",\n";
	for (size_t i = 0; i < portList.m_inputs.size(); i += 1) {
		bool bWidth = portList.m_inputs[i].m_bIndexed;
		if (bWidth && portList.m_inputs[i].m_highBit > 31) {
			for (int lowBit = 0; lowBit < portList.m_inputs[i].m_highBit; lowBit += 32) {
				int highBit = lowBit+31 < portList.m_inputs[i].m_highBit ? lowBit+31 : portList.m_inputs[i].m_highBit;
				fprintf(fp, "%s    %s_%d_%d", pSeparater, portList.m_inputs[i].m_name.c_str(), highBit, lowBit);
			}
		} else
			fprintf(fp, "%s    %s", pSeparater, portList.m_inputs[i].m_name.c_str());
	}
	fprintf(fp, "\n  );\n");
	fprintf(fp, "\n");
	fprintf(fp, "  %s dut(\n", pModule->m_name.c_str());
	pSeparater = "";
	if (portList.m_clock.size() > 0) {
		fprintf(fp, "    .%s(%s)", portList.m_clock.c_str(), portList.m_clock.c_str());
		pSeparater = ",\n";
	}
	for (size_t i = 0; i < portList.m_inputs.size(); i += 1) {
		fprintf(fp, "%s    .%s(%s)", pSeparater, portList.m_inputs[i].m_name.c_str(), portList.m_inputs[i].m_name.c_str());
		pSeparater = ",\n";
	}
	for (size_t i = 0; i < portList.m_outputs.size(); i += 1) {
		fprintf(fp, "%s    .%s(%s)", pSeparater, portList.m_outputs[i].m_name.c_str(), portList.m_outputs[i].m_name.c_str());
		pSeparater = ",\n";
	}
	fprintf(fp, "\n  );\n");
	fprintf(fp, "\n");
	fprintf(fp, "endmodule\n");

	fclose(fp);
}

void
CDesign::GenFixtureModuleVerilogMainCpp(const string &mainCppFile, CModule *pModule, CPortList &portList)
{
	FILE *fp = fopen(mainCppFile.c_str(), "w");
	if (!fp) {
		printf("Could not open '%s' for writing\n", mainCppFile.c_str());
		exit(1);
	}

	fprintf(fp, "#include <stdint.h>\n");
	fprintf(fp, "#include <stdlib.h>\n");
	fprintf(fp, "#include <stdio.h>\n");
	fprintf(fp, "#include <ctype.h>\n");
	fprintf(fp, "#include \"../../../Common/Random.h\"\n");
	fprintf(fp, "\n");
	fprintf(fp, "CRandom cnyRnd;\n");
	fprintf(fp, "\n");
	fprintf(fp, "#include \"acc_user.h\"\n");
	fprintf(fp, "extern \"C\" int InitFixture();\n");
	fprintf(fp, "extern \"C\" int Fixture();\n");
	fprintf(fp, "\n");
	fprintf(fp, "struct CHandles {\n");
	fprintf(fp, "	handle m_phase;\n");
	for (size_t i = 0; i < portList.m_inputs.size(); i += 1) {
		bool bWidth = portList.m_inputs[i].m_bIndexed;
		if (bWidth && portList.m_inputs[i].m_highBit > 31) {
			for (int lowBit = 0; lowBit < portList.m_inputs[i].m_highBit; lowBit += 32) {
				int highBit = lowBit+31 < portList.m_inputs[i].m_highBit ? lowBit+31 : portList.m_inputs[i].m_highBit;
				fprintf(fp, "	handle m_%s_%d_%d;\n", portList.m_inputs[i].m_name.c_str(), highBit, lowBit);
			}
		} else
			fprintf(fp, "	handle m_%s;\n", portList.m_inputs[i].m_name.c_str());
	}
	fprintf(fp, "} sig;\n");
	fprintf(fp, "\n");
	fprintf(fp, "void SetSigValue(handle sig, uint32_t value) {\n");
	fprintf(fp, "	s_setval_value value_s;\n");
	fprintf(fp, "	value_s.format = accIntVal;\n");
	fprintf(fp, "	value_s.value.integer = value;\n");
	fprintf(fp, "\n");
	fprintf(fp, "	s_setval_delay delay_s;\n");
	fprintf(fp, "	delay_s.model = accNoDelay;\n");
	fprintf(fp, "	delay_s.time.type = accTime;\n");
	fprintf(fp, "	delay_s.time.low = 0;\n");
	fprintf(fp, "	delay_s.time.high = 0;\n");
	fprintf(fp, "\n");
	fprintf(fp, "	acc_set_value(sig, &value_s, &delay_s);\n");
	fprintf(fp, "}\n");
	fprintf(fp, "\n");
	fprintf(fp, "uint32_t GetSigValue(handle sig, char *pName) {\n");
	fprintf(fp, "	p_acc_value pAccValue;\n");
	fprintf(fp, "	const char *pNum = acc_fetch_value(sig, \"%%x\", pAccValue);\n");
	fprintf(fp, "	const char *pNum2 = pNum;\n");
	fprintf(fp, "\n");
	fprintf(fp, "	uint32_t value = 0;\n");
	fprintf(fp, "	while (isdigit(*pNum) || tolower(*pNum) >= 'a' && tolower(*pNum) <= 'f')\n");
	fprintf(fp, "		value = value * 16 + (isdigit(*pNum) ? (*pNum++ - '0') : (tolower(*pNum++) - 'a' + 10));\n");
	fprintf(fp, "\n");
	fprintf(fp, "	if (*pNum != '\\0')\n");
	fprintf(fp, "		printf(\"  Signal '%%s' had value '%%s'\\n\", pName, pNum2);\n");
	fprintf(fp, "\n");
	fprintf(fp, "	return value;\n");
	fprintf(fp, "}\n");
	fprintf(fp, "\n");
	for (size_t i = 0; i < portList.m_inputs.size(); i += 1) {
		fprintf(fp, "void SetSig_%s(uint64_t %s) { ", portList.m_inputs[i].m_name.c_str(), portList.m_inputs[i].m_name.c_str());
		bool bWidth = portList.m_inputs[i].m_bIndexed;
		if (bWidth && portList.m_inputs[i].m_highBit > 31) {
			for (int lowBit = 0; lowBit < portList.m_inputs[i].m_highBit; lowBit += 32) {
				int highBit = lowBit+31 < portList.m_inputs[i].m_highBit ? lowBit+31 : portList.m_inputs[i].m_highBit;
				fprintf(fp, "SetSigValue(sig.m_%s_%d_%d, (%s >> %d) & 0xffffffff); ",
					portList.m_inputs[i].m_name.c_str(), highBit, lowBit, portList.m_inputs[i].m_name.c_str(), lowBit);
			}
		} else
			fprintf(fp, "SetSigValue(sig.m_%s, %s); ", portList.m_inputs[i].m_name.c_str(), portList.m_inputs[i].m_name.c_str());

		fprintf(fp, "}\n");
	}
	fprintf(fp, "\n");
	fprintf(fp, "uint32_t GetSig_phase() { return GetSigValue(sig.m_phase, \"phase\"); }\n");
	fprintf(fp, "\n");
	fprintf(fp, "int InitFixture() {\n");
	fprintf(fp, "	acc_initialize();\n");
	fprintf(fp, "	int i = 1;\n");
	fprintf(fp, "	sig.m_phase = acc_handle_tfarg(i++);\n");
	for (size_t i = 0; i < portList.m_inputs.size(); i += 1) {
		bool bWidth = portList.m_inputs[i].m_bIndexed;
		if (bWidth && portList.m_inputs[i].m_highBit > 31) {
			for (int lowBit = 0; lowBit < portList.m_inputs[i].m_highBit; lowBit += 32) {
				int highBit = lowBit+31 < portList.m_inputs[i].m_highBit ? lowBit+31 : portList.m_inputs[i].m_highBit;
				fprintf(fp, "	sig.m_%s_%d_%d = acc_handle_tfarg(i++);\n",
					portList.m_inputs[i].m_name.c_str(), highBit, lowBit);
			}
		} else
			fprintf(fp, "	sig.m_%s = acc_handle_tfarg(i++);\n", portList.m_inputs[i].m_name.c_str());
	}
	fprintf(fp, "\n");
	fprintf(fp, "	acc_vcl_add(sig.m_phase, ::Fixture, null, vcl_verilog_logic);\n");
	fprintf(fp, "	acc_close();\n");
	fprintf(fp, "\n");
	fprintf(fp, "	return 0;\n");
	fprintf(fp, "}\n");
	fprintf(fp, "\n");
	fprintf(fp, "int clkCnt = 0;\n");
	fprintf(fp, "\n");
	fprintf(fp, "int Fixture() {\n");
	fprintf(fp, "	if (GetSig_phase() != 1)\n");
	fprintf(fp, "		return 0;\n");
	fprintf(fp, "\n");
	fprintf(fp, "	if (++clkCnt == 10000)\n");
	fprintf(fp, "		exit(0);\n");
	fprintf(fp, "\n");
	for (size_t i = 0; i < portList.m_inputs.size(); i += 1)
		fprintf(fp, "	uint64_t %s = cnyRnd.RndUint64();\n", portList.m_inputs[i].m_name.c_str());
	fprintf(fp, "\n");
	for (size_t i = 0; i < portList.m_inputs.size(); i += 1)
		fprintf(fp, "	SetSig_%s(%s);\n", portList.m_inputs[i].m_name.c_str(), portList.m_inputs[i].m_name.c_str());
	fprintf(fp, "\n");
	fprintf(fp, "	return 0;\n");
	fprintf(fp, "}\n");

	fclose(fp);
}

void
CDesign::MkDir(const string &newDir)
{
#ifdef _WIN32
	int err = mkdir(newDir.c_str());
#else
	int err = mkdir(newDir.c_str(), 0x755);
#endif
	if (err != 0) {
		int errnum;
#ifdef _WIN32
		_get_errno(&errnum);
#else
		errnum = errno;
#endif
		if (errnum == EEXIST) {
			printf("  Directory for test fixture already exists: %s\n", newDir.c_str());
			printf("  Remove directory to proceed with test fixture generation\n");
		} else
			printf("  Unable to create directory: %s", newDir.c_str());

		exit(1);
	}
}
