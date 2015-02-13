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

#include "HtfeLineInfo.h"

enum EErrorMsgType { PARSE_INFO, PARSE_WARNING, PARSE_ERROR, PARSE_FATAL };

class CErrorMsg
{
public:
    CErrorMsg(void);
    ~CErrorMsg(void);

	void ErrorMsg_(EErrorMsgType msgType, const char *msgStr, ...);
	void ErrorMsg_(EErrorMsgType msgType, const CLineInfo &lineInfo, const char *msgStr, ...);
	void ErrorMsg_(EErrorMsgType msgType, const CLineInfo &lineInfo, const char *msgStr, va_list marker);

	void Assert_(bool bOkay, char *file, int line);

	int GetErrorCnt_() { return m_errorCnt; }
	int GetWarningCnt_() { return m_warningCnt; }

private:
    FILE    *m_pMsgFp;
    int     m_errorCnt;
    int     m_warningCnt;
};

extern CErrorMsg g_errorMsg;

#define Assert(ok) g_errorMsg.Assert_((ok != 0), __FILE__, __LINE__)
#define ErrorMsg g_errorMsg.ErrorMsg_

#define GetErrorCnt() g_errorMsg.GetErrorCnt_()
#define GetWarningCnt() g_errorMsg.GetWarningCnt_()

void ErrorExit();
