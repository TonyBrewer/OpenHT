/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
// ScDesign.cpp: implementation of the CScDesign class.
//
//////////////////////////////////////////////////////////////////////
#include "Htfe.h"
#include "HtfeDesign.h"

CHtfeDesign * g_pHtfeDesign = 0;
CHtfeDesign * CHtfeDesign::m_pHtDesign;

CHtfeIdent * CHtfeDesign::m_pVoidType;
CHtfeIdent * CHtfeDesign::m_pBoolType;
CHtfeIdent * CHtfeDesign::m_pScUintType;
CHtfeIdent * CHtfeDesign::m_pScIntType;
CHtfeIdent * CHtfeDesign::m_pScBigUintType;
CHtfeIdent * CHtfeDesign::m_pScBigIntType;
CHtfeIdent * CHtfeDesign::m_pBigIntBaseType;
CHtfeIdent * CHtfeDesign::m_pScStateType;
CHtfeIdent * CHtfeDesign::m_pCharType;
CHtfeIdent * CHtfeDesign::m_pUCharType;
CHtfeIdent * CHtfeDesign::m_pShortType;
CHtfeIdent * CHtfeDesign::m_pUShortType;
CHtfeIdent * CHtfeDesign::m_pIntType;
CHtfeIdent * CHtfeDesign::m_pUIntType;
CHtfeIdent * CHtfeDesign::m_pLongType;
CHtfeIdent * CHtfeDesign::m_pULongType;
CHtfeIdent * CHtfeDesign::m_pInt8Type;
CHtfeIdent * CHtfeDesign::m_pUInt8Type;
CHtfeIdent * CHtfeDesign::m_pInt16Type;
CHtfeIdent * CHtfeDesign::m_pUInt16Type;
CHtfeIdent * CHtfeDesign::m_pInt32Type;
CHtfeIdent * CHtfeDesign::m_pUInt32Type;
CHtfeIdent * CHtfeDesign::m_pInt64Type;
CHtfeIdent * CHtfeDesign::m_pUInt64Type;
CHtfeIdent * CHtfeDesign::m_pFloatType;
CHtfeIdent * CHtfeDesign::m_pDoubleType;
CHtfeIdent * CHtfeDesign::m_pRangeType;


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CHtfeDesign::CHtfeDesign()
{
	m_pHtDesign = this;
}

void CHtfeDesign::InitTables()
{
	m_pTopHier = HandleNewIdent();

	m_pTopHier->NewFlatTable();
	m_pVoidType = m_pTopHier->InsertType("void")
        ->SetId(CHtfeIdent::id_class)
        ->SetWidth(0);
	m_pBoolType = m_pTopHier->InsertType("bool")
		->SetId(CHtfeIdent::id_intType)
        ->SetWidth(1)
        ->SetOpWidth(32);
	m_pCharType = m_pTopHier->InsertType("char")
		->SetId(CHtfeIdent::id_intType)
        ->SetWidth(8)
        ->SetOpWidth(32)
        ->SetSizeof(1)
        ->SetIsSigned();
	m_pUCharType = m_pTopHier->InsertType("unsigned char")
		->SetId(CHtfeIdent::id_intType)
        ->SetWidth(8)
        ->SetOpWidth(32)
        ->SetPromoteToSigned()
        ->SetSizeof(1);
	m_pShortType = m_pTopHier->InsertType("short")
		->SetId(CHtfeIdent::id_intType)
        ->SetWidth(16)
        ->SetOpWidth(32)
        ->SetSizeof(2)
        ->SetIsSigned();
	m_pUShortType = m_pTopHier->InsertType("unsigned short")
		->SetId(CHtfeIdent::id_intType)
        ->SetOpWidth(32)
        ->SetWidth(16)
        ->SetSizeof(2)
        ->SetPromoteToSigned();
	m_pIntType = m_pTopHier->InsertType("int")
		->SetId(CHtfeIdent::id_intType)
        ->SetWidth(32)
        ->SetOpWidth(32)
        ->SetSizeof(4)
        ->SetIsSigned();
	m_pUIntType = m_pTopHier->InsertType("unsigned int")
		->SetId(CHtfeIdent::id_intType)
        ->SetWidth(32)
        ->SetOpWidth(32)
        ->SetSizeof(4);
	m_pLongType = m_pTopHier->InsertType("long long")
		->SetId(CHtfeIdent::id_intType)
		->SetWidth(64)
		->SetOpWidth(64)
		->SetSizeof(8)
		->SetIsSigned();
	m_pULongType = m_pTopHier->InsertType("unsigned long long")
		->SetId(CHtfeIdent::id_intType)
		->SetWidth(64)
		->SetOpWidth(64)
		->SetSizeof(8);
	m_pInt8Type = m_pTopHier->InsertType("int8_t")
		->SetId(CHtfeIdent::id_intType)
		->SetWidth(8)
		->SetOpWidth(32)
		->SetSizeof(1)
		->SetIsSigned();
	m_pUInt8Type = m_pTopHier->InsertType("uint8_t")
		->SetId(CHtfeIdent::id_intType)
		->SetWidth(8)
		->SetOpWidth(32)
		->SetSizeof(1)
		->SetPromoteToSigned();
	m_pInt16Type = m_pTopHier->InsertType("int16_t")
		->SetId(CHtfeIdent::id_intType)
		->SetWidth(16)
		->SetOpWidth(32)
		->SetSizeof(2)
		->SetIsSigned();
	m_pUInt16Type = m_pTopHier->InsertType("uint16_t")
		->SetId(CHtfeIdent::id_intType)
		->SetWidth(16)
		->SetOpWidth(32)
		->SetSizeof(2)
		->SetPromoteToSigned();
	m_pInt32Type = m_pTopHier->InsertType("int32_t")
		->SetId(CHtfeIdent::id_intType)
		->SetWidth(32)
		->SetOpWidth(32)
		->SetSizeof(4)
		->SetIsSigned();
	m_pUInt32Type = m_pTopHier->InsertType("uint32_t")
		->SetId(CHtfeIdent::id_intType)
		->SetWidth(32)
		->SetOpWidth(32)
		->SetSizeof(4);
	m_pInt64Type = m_pTopHier->InsertType("int64_t")
		->SetId(CHtfeIdent::id_intType)
        ->SetWidth(64)
        ->SetOpWidth(64)
        ->SetSizeof(8)
        ->SetIsSigned();
	m_pUInt64Type = m_pTopHier->InsertType("uint64_t")
		->SetId(CHtfeIdent::id_intType)
        ->SetWidth(64)
        ->SetOpWidth(64)
        ->SetSizeof(8);
	m_pFloatType = m_pTopHier->InsertType("float")
        ->SetId(CHtfeIdent::id_class)
        ->SetWidth(32)
        ->SetOpWidth(32)
        ->SetSizeof(4);
	m_pDoubleType = m_pTopHier->InsertType("double")
        ->SetId(CHtfeIdent::id_class)
        ->SetWidth(64)
        ->SetOpWidth(64)
        ->SetSizeof(8);
	m_pScUintType = m_pTopHier->InsertType("sc_uint")
        ->SetId(CHtfeIdent::id_class)
        ->SetWidth(-1)
        ->SetOpWidth(64)
		->SetUserConvType(m_pUInt64Type);
	m_pScBigUintType = m_pTopHier->InsertType("sc_biguint")
		->SetId(CHtfeIdent::id_class)
		->SetWidth(-1)
		->SetOpWidth(256);  // variable width
	m_pScBigIntType = m_pTopHier->InsertType("sc_bigint")
		->SetId(CHtfeIdent::id_class)
		->SetWidth(-1)
		->SetOpWidth(256);  // variable width
	m_pBigIntBaseType = m_pTopHier->InsertType("sc_bigintbase")
		->SetId(CHtfeIdent::id_class)
		->SetWidth(-1)
		->SetOpWidth(256);
	m_pScIntType = m_pTopHier->InsertType("sc_int")
        ->SetId(CHtfeIdent::id_class)
        ->SetWidth(-1)
        ->SetOpWidth(64)
		->SetIsSigned()
		->SetUserConvType(m_pIntType);  // variable width
	m_pScStateType = m_pTopHier->InsertType("sc_state")
        ->SetWidth(-1);  // variable width
	//m_pTopHier->InsertType("sc_biguint")->SetId(CHtfeIdent::id_class)->SetIsVariableWidth();  // variable width
	m_pTopHier->InsertType("sc_clock");
	m_pRangeType = m_pTopHier->InsertType("sc_range")
        ->SetWidth(-1);

	m_pTopHier->InsertIdent("true")
        ->SetId(CHtfeIdent::id_const)
        ->SetType(m_pBoolType)
        ->SetConstValue(CConstValue( (uint32_t)1 ));
	m_pTopHier->InsertIdent("false")
        ->SetId(CHtfeIdent::id_const)
        ->SetType(m_pBoolType)
        ->SetConstValue(CConstValue( (uint32_t)0 ));
	m_pTopHier->InsertIdent("sc_simulation_time")
        ->SetId(CHtfeIdent::id_function)
        ->SetType(m_pIntType)
        ->SetIsStatic();

	m_pTopHier->AddNullConvOverloadedOperator(tk_plus, m_pBigIntBaseType, m_pIntType, m_pBigIntBaseType);
	m_pTopHier->AddNullConvOverloadedOperator(tk_minus, m_pBigIntBaseType, m_pIntType, m_pBigIntBaseType);
	m_pTopHier->AddNullConvOverloadedOperator(tk_lessLess, m_pBigIntBaseType, m_pIntType, m_pBigIntBaseType);
	m_pTopHier->AddNullConvOverloadedOperator(tk_greaterGreater, m_pBigIntBaseType, m_pIntType, m_pBigIntBaseType);
	m_pTopHier->AddNullConvOverloadedOperator(tk_vbar, m_pBigIntBaseType, m_pIntType, m_pBigIntBaseType);
	m_pTopHier->AddNullConvOverloadedOperator(tk_ampersand, m_pIntType, m_pIntType, m_pBigIntBaseType);
	m_pTopHier->AddNullConvOverloadedOperator(tk_carot, m_pBigIntBaseType, m_pIntType, m_pBigIntBaseType);
	m_pTopHier->AddNullConvOverloadedOperator(tk_asterisk, m_pBigIntBaseType, m_pIntType, m_pBigIntBaseType);
	m_pTopHier->AddNullConvOverloadedOperator(tk_slash, m_pBigIntBaseType, m_pIntType, m_pBigIntBaseType);
	m_pTopHier->AddNullConvOverloadedOperator(tk_percent, m_pBigIntBaseType, m_pIntType, m_pBigIntBaseType);

	m_pTopHier->AddNullConvOverloadedOperator(tk_vbarVbar, m_pBoolType, m_pIntType, m_pBigIntBaseType);
	m_pTopHier->AddNullConvOverloadedOperator(tk_ampersandAmpersand, m_pBoolType, m_pIntType, m_pBigIntBaseType);
	m_pTopHier->AddNullConvOverloadedOperator(tk_equalEqual, m_pBoolType, m_pIntType, m_pBigIntBaseType);
	m_pTopHier->AddNullConvOverloadedOperator(tk_bangEqual, m_pBoolType, m_pIntType, m_pBigIntBaseType);
	m_pTopHier->AddNullConvOverloadedOperator(tk_less, m_pBoolType, m_pIntType, m_pBigIntBaseType);
	m_pTopHier->AddNullConvOverloadedOperator(tk_greater, m_pBoolType, m_pIntType, m_pBigIntBaseType);
	m_pTopHier->AddNullConvOverloadedOperator(tk_lessEqual, m_pBoolType, m_pIntType, m_pBigIntBaseType);
	m_pTopHier->AddNullConvOverloadedOperator(tk_greaterEqual, m_pBoolType, m_pIntType, m_pBigIntBaseType);

	m_forStatementLevel = 0;
	m_parseSwitchCount = 0;
	m_bDeadStatement = false;

	// Initialize strings to avoid reallocation
	m_operatorParenString = "operator()";
	m_rangeString = "range";
	m_readString = "read";
	m_writeString = "write";
	m_andReduceString = "and_reduce";
	m_nandReduceString = "nand_reduce";
	m_orReduceString = "or_reduce";
	m_norReduceString = "nor_reduce";
	m_xorReduceString = "xor_reduce";
	m_xnorReduceString = "xnor_reduce";

	m_bCaseExpr = false;

	CHtfeIdent::InitVerilogKeywordTbl();
}

CHtfeDesign::~CHtfeDesign()
{
}

bool
CHtfeDesign::OpenInputFile(const string &fileName)
{
	return CHtfeLex::Open(fileName);
}

CKeywordMap CHtfeIdent::m_verilogKeywordTbl;

void CHtfeIdent::InitVerilogKeywordTbl()
{
	char * pKeywords[] = { "always", "and", "assign", "automatic", "begin", "buf",
		"bufif0", "bufif1", "case", "casex", "casez", "cell", "cmos", "config", "deassign",
		"default", "defparam", "design", "disable", "edge", "else", "end", "endcase",
		"endconfig", "endfunction", "endgenerate", "endmodule", "endprimitive", "endspecify", 
		"endtable", "endtask", "event", "for", "force", "forever", "fork", "fuction",
		"generate", "genvar", "highz0", "highz1", "if", "ifnone", "incdir", "include",
		"initial", "inout", "input", "instance", "integer", "join", "large", "liblist",
		"library", "localparam", "macromodule", "medium", "module", "nand", "negedge",
		"nmos", "nor", "noshowcancelled", "not", "notif0", "notif1", "or", "output",
		"parameter", "pmos", "posedge", "primitive", "pull0", "pull1", "pulldown",
		"pullup", "pulsestyle_oneevent", "pulsestyle_ondetect", "rcmos", "real", "realtime",
		"reg", "release", "repeat", "rnmos", "rpmos", "rtran", "rtranif0,", "rtranif1",
		"scalared", "showcancelled", "signed", "small", "specify", "specparam", "strong0",
		"strong1", "supply0", "supply1", "table", "task", "time", "tran", "tranif0",
		"tranif1", "tri", "tri0", "tri1", "triand", "trior", "trireg", "unsigned", "use",
		"uwire", "vectored", "wait", "wand", "weak0", "weak1", "while", "wire", "wor",
		"xnor", "xor", 0
	};

	for (int i = 0; pKeywords[i]; i += 1)
		m_verilogKeywordTbl.Insert(pKeywords[i]);
}

bool CHtfeDesign::GetFieldWidthFromDimen(int dimen, int &width)
{
    width = 1;
    do {
        if ((1 << width) >= dimen)
            return true;
        width += 1;
    } while (width < 16);
    return false;
}

bool CHtfeDesign::IsConstSubFieldRange(CHtfeOperand *pOp, int &lowBit)
{
	int width;
	return GetSubFieldRange(pOp, lowBit, width, false);
}

bool CHtfeDesign::GetSubFieldRange(CHtfeOperand *pOp, int &lowBit, int &width, bool bErrorIfNotConst)
{
    // reduce target bit range
    lowBit = 0;
    width = pOp->GetMember()->GetWidth();
    bool bConstRange = true;

    CScSubField *pSubField = pOp->GetSubField();
    for ( ; pSubField; pSubField = pSubField->m_pNext) {

        // check if all indecies are constants
        int idx = 0;
        int idxBase = 1;
        for (int i = (int)(pSubField->m_indexList.size()-1); i >= 0; i -= 1) {
            CConstValue value;
            if (EvalConstantExpr(pSubField->m_indexList[i], value)) {
                idx += value.GetSint32() * idxBase;
                idxBase *= pSubField->m_pIdent ? pSubField->m_pIdent->GetDimenList()[i] : 1;
            } else {
                Assert(!bErrorIfNotConst);
                bConstRange = false;
            }
        }

        if (pSubField->m_pIdent)
            lowBit += pSubField->m_pIdent->GetStructPos() + pSubField->m_subFieldWidth * idx;
        else
            lowBit += idx;

        width = pSubField->m_subFieldWidth;
    }

    return bConstRange;
}

bool CHtfeDesign::IsTask(CHtfeStatement *pStatement) {
        CHtfeOperand *pExpr = pStatement->GetExpr();
        if (pExpr == 0)
            return false;

        if (pExpr->GetOperator() == tk_period)
            return true;

        CHtfeIdent *pIdent = pExpr->GetMember();
        return pIdent && pIdent->GetId() == CHtfeIdent::id_function && pIdent->GetType() == GetVoidType();
}

int CHtfeDesign::GetSubFieldLowBit(CHtfeOperand *pOp) {
	int lowBit, width;
	GetSubFieldRange(pOp, lowBit, width, true);
	return lowBit;
}

int CHtfeDesign::CalcElemIdx(CHtfeIdent * pIdent, vector<CHtfeOperand *> &indexList) {
	int elemIdx = 0;
	for (size_t dimIdx = 0; dimIdx < indexList.size(); dimIdx += 1) {
		if (!indexList[dimIdx]->IsConstValue())
			return -1;
		elemIdx *= pIdent->GetDimenList()[dimIdx];
		elemIdx += indexList[dimIdx]->GetConstValue().GetSint32();
	}
	return elemIdx;
}

int CHtfeDesign::CalcElemIdx(CHtfeIdent * pIdent, vector<int> &refList) {
	int elemIdx = 0;
	for (size_t dimIdx = 0; dimIdx < refList.size(); dimIdx += 1) {
		elemIdx *= pIdent->GetDimenList()[dimIdx];
		elemIdx += refList[dimIdx];
	}
	return elemIdx;
}
