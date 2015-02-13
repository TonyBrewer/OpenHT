/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
// HtFeDesign.h.h: interface for the CScDesign class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "HtfeLex.h"
#include "HtfeArgs.h"

#include "HtfeFile.h"
#include "HtfeSbx.h"
#include "HtfeKeywordTbl.h"
#include "HtfeAttrib.h"
#include "HtfeInstance.h"
#include "HtfeParam.h"
#include "HtfeIdent.h"

class CHtfeOperand;
class CHtfeIdent;
class CHtfeIdentTbl;
class CHtfeStatement;

#ifndef _WIN32
#include <limits.h>
#endif

class VA {
public:
	VA(char const * pFmt, ... ) {
		va_list args;
		va_start( args, pFmt );
		char buf[4096];
		vsprintf(buf, pFmt, args);
		m_str = string(buf);
	}
	VA(string &str) { m_str = str; }

	size_t size() { return m_str.size(); }
	operator string() { return m_str; }
	char const * c_str() { return m_str.c_str(); }

	bool operator == (char const * pStr) { return m_str == pStr; }

private:
	string m_str;
};

class CHtfeDesign : public CHtfeLex  
{
	friend class CHtfeIdentTbl;
public:
    enum EClockState { CS_UNKNOWN, CS_PRE_CLOCK, CS_POST_CLOCK };

public:
    void Parse();
    void CheckWidthRules();
    void CheckSignalRules();
    void CheckReferenceRules();
    void CheckSequenceRules();

	virtual void DeleteFiles() = 0;

public:
	static CHtfeIdent * NewIdent() { return m_pHtDesign->HandleNewIdent(); }

#ifndef min
    int min(int i1, int i2) { return i1 < i2 ? i1 : i2; }
#endif
#ifndef max
    int max(int i1, int i2) { return i1 > i2 ? i1 : i2; }
#endif

protected:
    CHtfeDesign();
    virtual ~CHtfeDesign();

	void InitTables();

	static void SetHtArgs(CHtfeArgs * pHtfeArgs) { g_pHtfeArgs = pHtfeArgs; }

    bool OpenInputFile(const string &fileName);
    bool ReopenInputFile() { return CHtfeLex::Reopen(); }
    bool OpenOutputFile(const string &inputFilename, const string &outputFileName);

	virtual void HandleInputFileInit() {}
	virtual void HandleScMainInit() {}
	virtual void HandleScMainStart() {}
	virtual void HandleGenerateScModule(CHtfeIdent * pClass) {}
	virtual void HandleGenerateScMain() {}

	virtual CHtfeStatement * HandleNewStatement() = 0;
	virtual CHtfeOperand *   HandleNewOperand() = 0;
	virtual CHtfeIdent *     HandleNewIdent() = 0;

	CHtfeIdent * FindUniqueAccessMethod(const vector<CHtfeIdent *> *pWriterList, bool &bMultiple);
	bool CheckMultipleRefMethods(CHtfeIdent *pWriter, const vector<CHtfeIdent *> & readerList);

    CHtfeIdent * GetTopHier() { return m_pTopHier; };
	bool GetFieldWidthFromDimen(int dimen, int &width);

	CHtfeSignalTbl & GetSignalTbl() { return m_signalTbl; }
	CHtfeInstanceTbl & GetInstanceTbl() { return m_instanceTbl; }

	CHtfeIdent * GetVoidType() { return m_pVoidType; };

	CTokenList const & GetScStartList() { return m_scStart; }

    bool IsTask(CHtfeStatement *pStatement);

	void UpdateVarArrayDecl(CHtfeIdent *pIdent, vector<int> &dimenList);

    void SetOpWidthAndSign(CHtfeOperand *pExpr, EToken prevTk=tk_eof, bool bIsLeftOfEqual=false);
	void CheckForUnsafeExpression(CHtfeOperand *pExpr, EToken prevTk=tk_eof, bool bLeftOfEqual=false);

    bool EvalConstantExpr(CHtfeOperand *pExpr, CConstValue & value, bool bMarkAsRead=true);

	bool IsConstSubFieldRange(CHtfeOperand *pOp, int &lowBit);
	bool GetSubFieldRange(CHtfeOperand *pOp, int &lowBit, int &width, bool bErrorIfNotConst);
	int GetSubFieldLowBit(CHtfeOperand *pOp);
	int CalcElemIdx(CHtfeIdent * pIdent, vector<CHtfeOperand *> &indexList);
	int CalcElemIdx(CHtfeIdent * pIdent, vector<int> &refList);

    void CheckMissingInstancePorts();
    void CheckConsistentPortTypes();
    void DumpPortList(CHtfeSignal *pSignal);

private:
    void ParseTypeDef(CHtfeIdent *pHier);
    void ParseScModule(bool bScFixture, bool bScTopologyTop);
    void ParseScAttrib(CHtfeIdent *pHier);
    void ParseCtorStatement(CHtfeIdent *pHier);
    void ParseCtorExpression(CHtfeIdent *pHier);
    CHtfeIdent *ParseVariableRef(const CHtfeIdent *pHier, const CHtfeIdent *&pHier2, bool bRequired=true, bool bIgnoreIndexing=false, bool bLeftOfEqual=false);
	void ParseVariableIndexing(CHtfeIdent *pHier, const CHtfeIdent *pIdent, vector<CHtfeOperand *> &indexList);
	int ParseConstantIndexing(CHtfeIdent *pHier, const CHtfeIdent *pIdent, vector<int> &indexList);
    CHtfeStatement *ParseCompoundStatement(CHtfeIdent *pHier, bool bNewNameSpace=true);
    CHtfeStatement *ParseDoStatement(CHtfeIdent *pHier, bool bNewNameSpace=true);
    CHtfeStatement *ParseStatement(CHtfeIdent *pHier);
    void SkipStatement(CHtfeIdent *pHier);
    CHtfeStatement *ParseIdentifierStatement(CHtfeIdent *pHier, bool bAssignmentAllowed, bool bStaticAllowed);
    CHtfeStatement *ParseIfStatement(CHtfeIdent *pHier);
	void ParseForStatement();
    CHtfeStatement *ParseSwitchStatement(CHtfeIdent *pHier);
	void TransformCaseStatementListToRemoveBreaks(CHtfeStatement *pCaseHead);
	void TransformCaseIfStatementList(CHtfeStatement **ppList, CHtfeStatement *pPostList);
	bool IsEmbeddedBreakStatement(CHtfeStatement *pStIf2);
	void TransformFunctionStatementListForReturns(CHtfeStatement *pCaseHead);
	void TransformFunctionIfStatementList(CHtfeStatement **ppList, CHtfeStatement *pPostList);
	bool IsEmbeddedReturnStatement(CHtfeStatement *pStIf2);
    CHtfeStatement *ParseVariableDecl(CHtfeIdent *pHier, CHtfeIdent *pType, CHtfeTypeAttrib const & typeAttrib, bool bIsReference);
    bool ParseFunctionDecl(CHtfeIdent *pHier, const CHtfeIdent *pHier2, CHtfeIdent *pIdent);
    CHtfeStatement * ParseVarDeclInitializer(CHtfeIdent * pHier, CHtfeIdent * pIdent);
    CHtfeStatement * InsertVarDeclInitializer(CHtfeIdent * pHier, CHtfeIdent * pIdent, vector<int> &refList, CHtfeOperand * pExpr, char * pInitStr="0");
	CHtfeStatement * InitializeToUnknown(CHtfeIdent * pHier, CHtfeIdent * pIdent);
    CHtfeOperand *ParseExpression(CHtfeIdent *pHier, bool bCommaAsSeparator=false, bool bFoldConst=true, bool bAllowOpEqual=false);
    void ParseEvaluateExpression(CHtfeIdent *pHier, EToken tk, vector<CHtfeOperand *> &operandStack, vector<EToken> &operatorStack);
	bool ParseSizeofFunction(CHtfeIdent *pHier, uint64_t &bytes);
    void ParsePortDecl(CHtfeIdent *pHier, EPortDir portDir);
	void ParseMemberArrayDecl(CHtfeIdent *pIdent, int structBitPos);
	void ParseVarArrayDecl(CHtfeIdent *pIdent);
	CHtfeIdent * ParseVarArrayRef(CHtfeIdent *pIdent);
	CHtfeIdent * CreateUniquePortTemplateType(string portName, CHtfeIdent * pBaseType, bool bIsConst);
	CHtfeIdent * ParseTypeDecl(CHtfeIdent const * pHier, CHtfeTypeAttrib & typeAttrib, bool bRequired=true);
	bool ParseHtPrimClkList(CHtfeTypeAttrib & typeAttrib);
    CHtfeIdent *ParseTypeDecl(const CHtfeIdent *pHier, bool bRequired=true);
	CHtfeIdent *ParseHtQueueDecl(CHtfeIdent *pHier);
	CHtfeIdent *ParseHtDistRamDecl(CHtfeIdent *pHier);
	CHtfeIdent *ParseHtBlockRamDecl(CHtfeIdent *pHier);
	CHtfeIdent *ParseHtAsymBlockRamDecl(CHtfeIdent *pHier, bool bMultiRead);
	void ParseTemplateDecl(CHtfeIdent *pHier);
	int ParseStructDecl(CHtfeIdent *pHier, int startBitPos=0);
    void RecordStructMethod(CHtfeIdent *pStruct);
	void ParseStructMethod(CHtfeIdent *pHier);
	CHtfeIdent *CreateUniqueTemplateType(const CHtfeIdent *pType, int width);
	bool ParseConstExpr(CConstValue &value);
	void ParseEvalConstExpr(EToken tk, vector<CConstValue> &operandStack, vector<EToken> &operatorStack);
    void ParseEnumDecl(CHtfeIdent *pHier);
    void ParseParamListDecl(CHtfeIdent *pHier, CHtfeIdent *pIdent);
    void ParseModuleCtor(CHtfeIdent *pHier);
	void ParseDestructor(CHtfeIdent *pHier);
    CHtfeIdent *ParseScMethod(CHtfeIdent *pHier);
    void ParseSensitiveStatement(CHtfeIdent *pHier, CHtfeIdent *pScMethod, EToken token);
    void ParseScMain();
    void ParseScSignal();
    void ParseScSignal2(CHtfeIdent *pHier);
    void ParseScClock();
    void ParseScStart();
    void ParseModuleInstance(CHtfeIdent *pHier);
    void ParseInstancePort(CHtfeInstance *pInstance);
    bool IsRegisterName(const string &name);
	void			HtQueueVarDecl(CHtfeIdent *pHier, CHtfeIdent *pIdent);
	void			HtMemoryVarDecl(CHtfeIdent *pHier, CHtfeIdent *pIdent);
	CHtfeOperand *	ParseHtQueueExpr(CHtfeIdent *pHier, CHtfeIdent *pIdent, vector<CHtfeOperand *> &indexList);
	CHtfeOperand *	ParseHtMemoryExpr(CHtfeIdent *pHier, CHtfeIdent *pIdent, vector<CHtfeOperand *> &indexList);
	CHtfeStatement *	ParseHtQueueStatement(CHtfeIdent *pHier);
	void			InitHtQueuePushPreClockStatements(CHtfeIdent * pHier, CHtfeIdent * pIdent, string &cQueName, string &rQueName);
	void			InitHtQueuePopPreClockStatements(CHtfeIdent * pHier, CHtfeIdent * pIdent, string &cQueName, string &rQueName);
	void			InitHtQueuePushClockStatements(CHtfeIdent * pHier, CHtfeIdent * pIdent, CHtfeStatement * &pStatement,
						string &resetStr, string &cQueName, string &rQueName, string &refDimStr);
	void			InitHtQueuePopClockStatements(CHtfeIdent * pHier, CHtfeIdent * pIdent, CHtfeStatement * &pStatement,
						string &resetStr, string &cQueName, string &rQueName, string &refDimStr);
	CHtfeStatement *	ParseHtMemoryStatement(CHtfeIdent *pHier);
	void			HtMemoryReadVarInit(CHtfeIdent *pHier, CHtfeIdent *pIdent, string & cQueName, string & refDimStr, string & declDimStr,
						vector<CHtfeOperand *> indexList);
	void			HtMemoryWriteVarInit(CHtfeIdent *pHier, CHtfeIdent *pIdent, string & cQueName, string & refDimStr, string & declDimStr,
						vector<CHtfeOperand *> indexList);
	void			HandleOverloadedAssignment(CHtfeIdent *pHier, CHtfeStatement *pStatement);
	void			HandleRegisterAssignment(CHtfeIdent *pHier, CHtfeStatement *pStatement);
	void			HandleSimpleIncDecStatement(CHtfeStatement *pStatement);
    void            HandleIntegerConstantPropagation(CHtfeOperand * pExpr);
    void			HandleScPrimMember(CHtfeIdent *pHier, CHtfeStatement *pStatement);
	void			DumpFunction(CHtfeIdent *pFunction);
	void			DumpStatementList(CHtfeStatement *pFunction);
	void			DumpStatement(CHtfeStatement *pStatement);
	void			DumpExpression(CHtfeOperand *pExpr);

    CHtfeIdent * AutoInsertScPrim(string &name);

    void CheckWidthRules(CHtfeIdent *pHier, CHtfeStatement *pStatement, int caseWidth=0);
    void CheckWidthRules(CHtfeOperand *pExpr, bool bForceBoolean=false);
    bool FoldConstantExpr(CHtfeOperand *pExpr);
    char *GetParseMsgWidthStr(CHtfeOperand *pOp, char *pBuf);

    void ClearRefInfo(CHtfeIdent *pFunc);
	void ClearRefInfo(CHtfeStatement * pStatement);
	void ClearRefInfo(CHtfeOperand * pExpr);
    void SetRefInfo(CHtfeOperand * pObj, CHtfeIdent * pHier, bool bLocal, bool bAlwaysAt, CHtfeStatement *pStatement);
    void SetRefInfo(CHtfeOperand * pObj, CHtfeIdent * pHier, bool bLocal, bool bAlwaysAt, CHtfeOperand *pExpr, bool bIsLeftSideOfEqual=false);
    void CheckRefCnts(bool bLocal, CHtfeIdent *pFunc);
    void CheckPortList(CHtfeIdent *pInst);
    void CheckSensitivityList(CHtfeIdent *pModule, CHtfeIdent *pMethod);

    void PrintReferenceCounts();
    void PrintReferenceCounts(CHtfeIdent *pIdent, int level);

    void CheckSequenceRules(CHtfeIdent *pModule, CHtfeIdent *pMethod);
    void CheckSequenceRules(CHtfeIdent *pHier, CHtfeOperand * pObj, CHtfeStatement *pStatement);
    void CheckSequenceRules(CHtfeIdent * pHier, CHtfeOperand * pObj, CHtfeOperand *pExpr, bool bIsLeftSideOfEqual=false, bool bIsParamExpr=false);
    void CheckSequenceRules(CHtfeIdent * pIdent, CHtfeOperand * pExpr, bool bIsParamExpr=false);

    void SkipTo(EToken);
    void SkipTo(EToken, EToken);

    bool IsConstCondExpression(CHtfeOperand *pExpr);
    bool IsExpressionEqual(CHtfeOperand *pOp1, CHtfeOperand *pOp2);
    void PushConstCondExpression(CHtfeOperand *pExpr, CHtfeOperand *pOp1, CHtfeOperand *pOp2, bool bOp1Cond);
    void MergeConstCondExpression(CHtfeOperand *pExpr);
    int SetExpressionWidth(CHtfeOperand *pExpr, int width=0, bool bForceBoolean=false);
    void GetExpressionWidth(CHtfeOperand *pExpr, int &minWidth, int &sigWidth, int &maxWidth, int &opWidth);
    void PushExpressionWidth(CHtfeOperand *pExpr, int width, bool bForceBoolean=false);
	void InsertCompareOperator(CHtfeOperand *pExpr);

private:
    CTokenList m_scStart;
    int m_parseSwitchCount;
	int m_parseReturnCount;
    CHtfeIdent *m_pVoidType;
    CHtfeIdent *m_pBoolType;
    CHtfeIdent *m_pScUintType;
    CHtfeIdent *m_pScIntType;
    CHtfeIdent *m_pScBigUintType;
    CHtfeIdent *m_pScStateType;
    string m_operatorParenString;
    string m_rangeString;
    string m_readString;
    string m_writeString;
    string m_andReduceString;
    string m_nandReduceString;
    string m_orReduceString;
    string m_norReduceString;
    string m_xorReduceString;
    string m_xnorReduceString;
    string m_constString;
    string m_unsignedString;
    string m_charString;
    string m_shortString;
    string m_intString;
    string m_longString;
    string m_floatString;
    string m_doubleString;
    CHtfeIdent *m_pCharType;
    CHtfeIdent *m_pUCharType;
    CHtfeIdent *m_pShortType;
    CHtfeIdent *m_pUShortType;
    CHtfeIdent *m_pIntType;
    CHtfeIdent *m_pUIntType;
    CHtfeIdent *m_pInt64Type;
    CHtfeIdent *m_pUInt64Type;
    CHtfeIdent *m_pFloatType;
    CHtfeIdent *m_pDoubleType;
    CHtfeIdent *m_pRangeType;
    bool m_bCaseExpr;

    vector<ESeqState> m_seqStateStack;

    vector<CHtfeOperand *> m_callStack;

private:
    CHtfeIdent * m_pTopHier;
    int         m_statementNestLevel;
    int         m_forStatementLevel;
    int         m_statementCount;
    bool        m_bMemWriteStatement;
    CHtfeOperand * m_pIfExprOperand;
	bool		m_bDeadStatement;

	CHtfeInstanceTbl m_instanceTbl;
	CHtfeSignalTbl	m_signalTbl;

	static CHtfeDesign * m_pHtDesign;
};

extern CHtfeDesign * g_pHtfeDesign;

#include "HtfeOperand.h"
#include "HtfeStatement.h"
