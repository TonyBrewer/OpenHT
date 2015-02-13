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

#include "HtvArgs.h"
#include "HtfeDesign.h"

#include "HtvIdent.h"
#include "HtvOperand.h"
#include "HtvObject.h"
#include "HtvStatement.h"
#include "HtvInstance.h"

class CHtvDesign : public CHtfeDesign {
public:
	struct CTempVar;
	enum EAlwaysType { atUnknown, atClocked, atNotClocked, atError };
public:
	CHtvDesign();

    void OpenFiles();
    void CloseFiles();
    void DeleteFiles();

	void HandleInputFileInit();
	void HandleScMainInit();
	void HandleScMainStart();
	void HandleParsedScModule(CHtvIdent * pClass);
	void HandleGenerateScMain();
	void HandleGenerateScModule(CHtfeIdent * pClass);

	CHtfeStatement * HandleNewStatement() { return new CHtvStatement(); }
	CHtfeOperand * HandleNewOperand() { return new CHtvOperand(); }
	CHtvIdent * HandleNewIdent() { return new CHtvIdent(); }

	CHtvIdent * FindUniqueAccessMethod(const vector<CHtfeIdent *> *pWriterList, bool &bMultiple) { return (CHtvIdent *)CHtfeDesign::FindUniqueAccessMethod(pWriterList, bMultiple); }

	int ParseSbxConfigLine(char *pLine, string &cfgType, string &cfgName, string &cfgValue);

	void GenSandbox();
    void GenSandboxVpp(CHtvIdent *pModule);
	void GenSandboxInitialConfigFile(const string &sbxCfg, CHtvIdent *pModule);
	void GenSandboxMakefile(string &sbxDir, vector<SbxOptMap_ValuePair> &makeOpt);
	void GenSandboxPrjFile(string &sbxDir, vector<SbxOptMap_ValuePair> &prjOpt, string &dsnName);
	void GenSandboxXstFile(string &sbxDir, CSbxOpt &xstOpt);
	void GenSandboxUcfFile(string &sbxDir, CSbxClks &sbxClks, vector<SbxOptMap_ValuePair> &ucfOpt);
	void GenSandboxFpgaVFile(string &sbxDir, CSbxClks &sbxClks, CHtvIdent *pModule);
	void GenSandboxModuleVFile(string &sbxDir, CSbxClks &sbxClks, CHtvIdent *pModule);

    void PreSynthesis(CHtvIdent *pModule, CHtvIdent *pMethod);
    void PreSynthesis(CHtvIdent *pHier, CHtvStatement *pStatement);
	void RemoveUnusedVariableStatements(CHtvStatement **ppStatement);

	string GenVerilogIdentName(const string &name, const vector<int> &refList);
	string GenVerilogIdentName(const string &name, const HtfeOperandList_t &indexList);
	string GenVerilogIdentName(CHtIdentElem & identElem, int distRamWrEnRangeIdx=-1);
	string GenVerilogArrayIdxName(const string &name, int idx);
	string GenVerilogFieldIdxName(const string &name, int idx);

    void PreSynthesis();
    void GenVerilog();

    bool SynXilinxRom(CHtvIdent *pHier, CHtvStatement *pSwitch);

	void GenFixtureFiles(CHtvIdent *pModule);
    FILE *BackupAndOpen(const string &fileName);
    void PrintHeader(FILE *fp, char *pComment);
    void GenScMainCpp();
    void GenRandomH();
    void GenRandomCpp();
    void GenFixtureH(CHtvIdent *pModule);
    void GenFixtureCpp(CHtvIdent *pModule);
    void GenFixtureModelH(CHtvIdent *pModule);
    void GenFixtureModelCpp(CHtvIdent *pModule);
    void GenScCodeFiles(const string &fileName);

	void GenVerilogFunctionCall(CHtvObject * pObj, CHtvObject * pRtnObj, CHtvOperand *pExpr);
	void SynInlineFunctionCall(CHtvObject * pObj, CHtvObject * pRtnObj, CHtvOperand *pExpr);
    void SynBuiltinFunctionCall(CHtvObject * pObj, CHtvObject * pRtnObj, CHtvOperand * pExpr);
    void PrintHotStatesCaseItem(CHtvOperand *pExpr, CHtvIdent *pCaseIdent, int hotStatesWidth);

    void GenerateScMain();
    void GenerateScModule(CHtvIdent *pClass);
	void GenScModuleInsertMissingInstancePorts(CHtvIdent *pClass);
	void GenScModuleWritePortAndSignalList(CHtvIdent *pClass);
	void GenScModuleWriteInstanceDecl(CHtvIdent *pClass);
	void GenScModuleWriteInstanceDestructors(CHtvIdent *pClass);
	void GenScModuleWriteSignalsAssignedConstant(CHtvIdent *pClass);
	void GenScModuleWriteInstanceAndPortList(CHtvIdent *pClass);

    // Synthesize methods
	void FindGlobalRefSet(CHtvIdent *pHier, CHtvStatement *pStatement);
	void FindGlobalRefSet(CHtvIdent *pFunc, CHtvOperand *pExpr);
    void FindWriteRefSets(CHtvIdent *pHier);
	void FindWriteRefSets(CHtvStatement *pStatement, int synSetId, int synSetSize, bool bClockedAlwaysAt);
    void FindXilinxFdPrims(CHtvIdent *pHier, CHtvObject * pObj);
	void FindXilinxFdPrims(CHtvObject * pObj, CHtvStatement *pStatement);
    void SynIndividualStatements(CHtvIdent *pHier, CHtvObject * pObj, CHtvObject * pRtnObj);
	void SynHtDistQueRams(CHtvIdent *pHier);
	void SynHtBlockQueRams(CHtvIdent *pHier);
	void SynHtDistRams(CHtvIdent *pHier);
	void SynHtDistRamModule(bool bQueue);
	void SynHtBlockRams(CHtvIdent *pHier);
	void SynHtBlockRamModule();
	void SynHtAsymBlockRams(CHtvIdent *pHier);
	void SynHtAsymBlockRamModule();
	void SynHtDistRamWe(CHtvIdent *pHier, CHtvObject * pObj, CHtvObject * pRtnObj, CHtvOperand *pExpr, bool bIsAlwaysAt);
	void SynScPrim(CHtvIdent *pHier, CHtvObject * pObj, CHtvStatement *pStatement);
	void SynXilinxRegisters(CHtvIdent *pHier, CHtvObject * pObj, CHtvStatement *pStatement, bool &bAlwaysAtNeeded);
	void SynIndividualStatements(CHtvIdent *pHier, CHtvObject * pObj, CHtvObject * pRtnObj, CHtvStatement *pStatement, int synSetSize);

    void GenVFileHeader();
    void GenModuleHeader(CHtvIdent *pIdent);
    void GenHtAttributes(CHtvIdent * pIdent, string instName="");
	void GenArrayIndexingSubExpr(CHtvObject * pObj, CHtvObject * pRtnObj, CHtvOperand * pOperand, bool bLeftOfEqual);
	void GenSubFieldIndexing(CHtvOperand * pExpr);
	void GenVariableFieldIndexing(CHtvOperand * pExpr);
	void GenModuleTrailer();
    const char *GenVerilogName(const char *pOrigName, const char *pPrefix=0);
	void GenFuncVarDecl(CHtvIdent * pFunc);
    void GenFunction(CHtvIdent *pMethod, CHtvObject * pObj, CHtvObject * pRtnObj);
    void GenMethod(CHtvIdent *pMethod, CHtvObject * pObj);
    void GenAlwaysAtHeader(bool bBeginEnd);
    void GenAlwaysAtHeader(CHtvIdent *pIdent);
    void GenAlwaysAtTrailer(bool bBeginEnd);

	bool IsAssignToHtPrimOutput(CHtvStatement * pStatement);
	bool HandleAllConstantAssignments(CHtvIdent *pHier, CHtvObject * pObj, CHtvObject * pRtnObj, CHtvStatement *pStatement, int synSetSize);
    void SynScLocalAlways(CHtvIdent *pMethod, CHtvObject * pObj);
    void RemoveMemoryWriteStatements(CHtvStatement **ppIfList);

    bool GenXilinxClockedPrim(CHtvIdent *pHier, CHtvObject * pObj, CHtvStatement *pStatement, CHtvOperand *pEnable=0, bool bCheckOnly=false);
    bool IsXilinxClockedPrim(CHtvObject * pObj, CHtvStatement *pStatement, CHtvOperand *pEnable=0) {
		return GenXilinxClockedPrim(0, pObj, pStatement, pEnable, true);
	}

	bool IsConcatLeaves(CHtvOperand *pOperand);
	string GetConcatLeafName(int exprIdx, CHtvOperand *pExpr, bool &bDimen, int &bitIdx);

    void SynCompoundStatement(CHtvIdent *pHier, CHtvObject * pObj, CHtvObject * pRtnObj, bool bForceSyn, bool bDeclareLocalVariables=false);
    void SynStatementList(CHtvIdent *pHier, CHtvObject * pObj, CHtvObject * pRtnObj, CHtvStatement *pStatement);
    void SynAssignmentStatement(CHtvIdent *pHier, CHtvObject * pObj, CHtvObject * pRtnObj, CHtvStatement *pStatement, bool bIsAlwaysAt=true);
    void SynIfStatement(CHtvIdent *pHier, CHtvObject * pObj, CHtvObject * pRtnObj, CHtvStatement *pStatement);
	void SynReturnStatement(CHtvIdent *pHier, CHtvObject * pObj, CHtvObject * pRtnObj, CHtvStatement *pStatement);
    void SynSwitchStatement(CHtvIdent *pHier, CHtvObject * pObj, CHtvObject * pRtnObj, CHtvStatement *pStatement);

	CHtvOperand * CreateTempOp(CHtvOperand * pOp, string const &tempName);
	CHtvOperand * CreateObject(CHtvOperand * pObj, CHtvOperand * pOp);
	void GenSubObject(CHtvObject * pSubObj, CHtvObject * pObj, CHtvOperand * pOp);
	CHtvOperand * PrependObjExpr(CHtvObject * pObj, CHtvOperand * pExpr);
	CHtvOperand * PrependObjExpr(CHtvObject * pObj, CHtvOperand * pObjOp, CHtvOperand * pExpr);
	void FindSubExpr(CHtvObject * pObj, CHtvObject * pRtnObj, CHtvOperand *pExpr, bool &bFuncSubExpr, bool &bWriteIndex, bool bForceTemp=false, bool bOutputParam=false);
	void MarkArrayIndexing(CHtvOperand *pExpr, vector<CTempVar> &tempVarList);
	void MarkVariableIndexing(CHtvOperand * pOperand, vector<CTempVar> &tempVarList);

	bool FindIfSubExprRequired(CHtvOperand *pExpr, bool bIsLeftOfEqual, bool bPrevTkIsEqual, bool bIsFuncParam, bool bHasObj);
	void MarkSubExprTempVars(CHtvObject * &pObj, CHtvOperand *pExpr, vector<CTempVar> &tempVarList, EToken prevTk, bool bNotLogical, bool bIsLeftOfEqual=false, bool bIsFuncParam=false);

	void GenSubExpr(CHtvObject * pObj, CHtvObject * pRtnObj, CHtvOperand *pExpr, bool bIsLeftOfEqual=false, bool bArrayIndexing=false, bool bGenSubExpr=false, EToken prevTk=tk_eof);
	void PrintSubExpr(CHtvObject * pObj, CHtvObject * pRtnObj, CHtvOperand *pExpr, bool bIsLeftOfEqual=false, bool bEntry=false, bool bArrayIndexing=false, string arrayName="", EToken prevTk=tk_eof, bool bIgnoreExprWidth=false);
	void PrintLogicalSubExpr(CHtvObject * pObj, CHtvObject * pRtnObj, CHtvOperand *pExpr);
	void PrintQuestionSubExpr(CHtvObject * pObj, CHtvObject * pRtnObj, CHtvOperand *pExpr);
	void PrintIncDecSubExpr(CHtvObject * pObj, CHtvObject * pRtnObj, CHtvOperand *pExpr);
	void PrintArrayIndex(CHtvObject * pObj, CHtvObject * pRtnObj, CHtvOperand * pOperand, bool bLeftOfEqual);
	void PrintFieldIndex(CHtvObject * pObj, CHtvObject * pRtnObj, CHtvOperand *pExpr);
	string GetHexMask(int width);

	enum EIdentFmt { eInvalid, eShfFldTmpAndVerMask, eName, eShfLowBitAndVerMask, eNameAndVerRng, eShfLowBitAndMask1, eNameAndConstBit,
		eNameAndVarBit, eShfLowBitAndSubMask, eNameAndSubRng, eShfFldPosAndSubMask, eNameAndCondTmpRng };

	EIdentFmt FindIdentFmt(CHtvObject * pObj, CHtvObject * pRtnObj, CHtvOperand *pExpr, EToken prevTk,
		bool bIsLeftOfEqual, bool bArrayIndexing, string arrayName, bool bIgnoreExprWidth, string &name, bool &bTempVar,
		bool &bSubField, int &subFieldLowBit, int &subFieldWidth, string &fieldPosName);

    EAlwaysType FindWriteRefVars(CHtvIdent *pHier, CHtvStatement *pStatement, CHtvOperand *pExpr, bool bIsLeftSideOfEqual=false, bool bFunction=false);
    EAlwaysType FindWriteRefVars(CHtvIdent *pHier, CHtvStatement *pBaseStatement, CHtvStatement *pStatement, bool bFunction=false);
	void MergeWriteRefSets(bool &bIsAllAssignConst, int &synSetSize, CHtvStatement * pStatement);
	bool IsRightHandSideConst(CHtvStatement * pStatement);
	bool IsExpressionEquivalent(CHtvOperand * pExpr1, CHtvOperand * pExpr2);
	bool isMethodCalledByModule(const vector<CHtfeIdent *> *pCallerList, CHtfeIdent *pModule);

	string GetNextExprBlockId(CHtvOperand * pExpr) {
		char buf[32];
		sprintf(buf, "%d$%d", pExpr->GetLineInfo().m_fileNum, pExpr->GetLineInfo().m_lineNum);

		m_blockId = buf;
		m_blockCntStr.clear();

		BlockIdMap_InsertPair i = m_blockIdMap.insert(BlockIdMap::value_type(buf, 0));

		int cnt = i.first->second++;

		if (cnt > 0) {
			int j;
			buf[31] = '\0';
			for (j = 30; cnt > 0; cnt /= 26, j -= 1)
				buf[j] = 'a' + cnt%26;

			m_blockCntStr = buf + j+1;
		}

		return m_blockId + m_blockCntStr;
	}
	string GetExprBlockId() { return m_blockId + m_blockCntStr; }
	string GetTempVarName(string blockId, int varIdx) {
		char tempName[64];
		sprintf(tempName, "temp$%s$%d", blockId.c_str(), varIdx);
		return string(tempName);
	}
	int GetTempVarIdx(string tempName) {
		int idx = 0;
		int power = 1;
		size_t pos = tempName.size()-1;
		for(;;) {
			char c = tempName[pos];
			if (!isdigit(c))
				break;
			idx += (c-'0') * power;
			pos -= 1;
			power *= 10;
		}
		//printf("%s => %d\n", tempName.c_str(), idx);
		return idx;
	}
	int GetNextAlwaysBlockIdx() { return ++m_alwaysBlockIdx; }

public:
	enum ETempType { eTempVar, eTempArrIdx, eTempFldIdx };

	struct CTempVar {
		CTempVar() {}
		CTempVar(ETempType type, string name, int width, CHtvOperand *pExpr=0)
            : m_tempType(type), m_name(name), m_width(width), m_pOperand(pExpr) {}

		ETempType		m_tempType;
		string			m_name;
        int             m_width;
		int				m_declWidth;
		CHtvOperand *	m_pOperand;
	};

	typedef map<string, int>			TempVarMap;
	typedef TempVarMap::iterator		TempVarMap_Iter;
	typedef pair<TempVarMap_Iter, bool>	TempVarMap_InsertPair;
    typedef pair<string, int>			TempVarMap_ValuePair;

private:
	class CAlwaysType {
	public:
		CAlwaysType() { m_at = atUnknown; }
		CAlwaysType(EAlwaysType at) { m_at = at; }
		operator EAlwaysType() { return m_at; }
		void operator = (EAlwaysType at) { m_at = at; }
		void operator += (EAlwaysType at) {
			if (m_at == atUnknown || at == atError)
				m_at = at;
			else if (m_at != atError && at != atUnknown && m_at != at)
				m_at = atError;
		}
		bool IsClocked() { return m_at == atClocked; }
		bool IsError() { return m_at == atError; }
	private:
		EAlwaysType	m_at;
	};

private:

	typedef map<string, int>			BlockIdMap;
	typedef BlockIdMap::iterator		BlockIdMap_Iter;
	typedef pair<BlockIdMap_Iter, bool>	BlockIdMap_InsertPair;

	// list of distributed rams types
	struct CHtDistRamType {
		CHtDistRamType(short addrWidth, short dataWidth) : m_addrWidth(addrWidth), m_dataWidth(dataWidth) {}
		short	m_addrWidth;
		short	m_dataWidth;
	};

private:
    string m_pathName;
    string m_backupPath;
    CHtFile m_incFile;
    CHtFile m_cppFile;
    CHtFile m_vFile;
	FILE * m_prmFp;

	CHtvIdent * m_pInlineHier;

    uint32_t m_genCppStartOffset;

	string m_blockId;
	string m_blockCntStr;
	BlockIdMap m_blockIdMap;
	TempVarMap m_tempVarMap;
	int m_alwaysBlockIdx;

    bool m_bGenedMinFixtureH;
    bool m_bClockedAlwaysAt;

	bool m_bIsHtQueueRamsPresent;
	bool m_bIsHtDistRamsPresent;
	bool m_bIs1CkHtBlockRamsPresent;
	bool m_bIs2CkHtBlockRamsPresent;
	bool m_bIs1CkDoRegHtBlockRamsPresent;
	bool m_bIs2CkDoRegHtBlockRamsPresent;
	bool m_bIs1CkHtMrdBlockRamsPresent;
	bool m_bIs1CkHtMwrBlockRamsPresent;
	bool m_bIs2CkHtMrdBlockRamsPresent;
	bool m_bIs2CkHtMwrBlockRamsPresent;
	bool m_bIs1CkDoRegHtMrdBlockRamsPresent;
	bool m_bIs1CkDoRegHtMwrBlockRamsPresent;
	bool m_bIs2CkDoRegHtMrdBlockRamsPresent;
	bool m_bIs2CkDoRegHtMwrBlockRamsPresent;
	
	vector<CHtDistRamType>	m_htDistRamTypes;
};
