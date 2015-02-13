/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "Htfe.h"
#include "HtfeDesign.h"
#include "HtfeErrorMsg.h"
#include "HtfeArgs.h"

CErrorMsg g_errorMsg;

CErrorMsg::CErrorMsg(void)
{
    m_pMsgFp = 0;
    m_warningCnt = 0;
    m_errorCnt = 0;
}

CErrorMsg::~CErrorMsg(void)
{
}

void CErrorMsg::Assert_(bool bOkay, char *file, int line)
{
	if (bOkay)
		return;

	int argc;
	char **argv;
	g_pHtfeArgs->GetArgs(argc, argv);

	printf("Fatal : internal inconsistency detected\n   File: %s\n   Line: %d\n   Command Line:", file, line);

	for (int i = 0; i < argc; i += 1)
		printf(" %s", argv[i]);

	printf("\n");

	if (m_pMsgFp) {
		fprintf(m_pMsgFp, "Fatal : internal inconsistency detected\n   File: %s\n   Line: %d\n   Command Line:", file, line);

		for (int i = 0; i < argc; i += 1)
			fprintf(m_pMsgFp, " %s", argv[i]);

		fprintf(m_pMsgFp, "\n");
	}

	ErrorExit();
}

void
CErrorMsg::ErrorMsg_(EErrorMsgType msgType, const char *msgStr, ...)
{
	va_list marker;
	va_start( marker, msgStr );

	CLineInfo lineInfo;
    lineInfo.m_lineNum = 0;
    lineInfo.m_pathName = "";

	ErrorMsg(msgType, lineInfo, msgStr, marker);
}

void
CErrorMsg::ErrorMsg_(EErrorMsgType msgType, const CLineInfo &lineInfo, const char *msgStr, ...)
{
	va_list marker;
	va_start( marker, msgStr );

	ErrorMsg(msgType, lineInfo, msgStr, marker);
}

void
CErrorMsg::ErrorMsg_(EErrorMsgType msgType, const CLineInfo &lineInfo, const char *msgStr, va_list marker)
{
	if (msgType == PARSE_ERROR)
		m_errorCnt += 1;

	if (msgType == PARSE_FATAL && msgStr == 0) {
		printf("%s(%d) : Fatal: unable to proceed due to previous errors\n",
			lineInfo.m_pathName.c_str(), lineInfo.m_lineNum);
		ErrorExit();
	}

	char *pMsgType;
	switch (msgType) {
		case PARSE_INFO: pMsgType = ""; break;
		case PARSE_WARNING: pMsgType = "Warning :"; break;
		case PARSE_ERROR: pMsgType = "Error :"; break;
		case PARSE_FATAL: pMsgType = "Error :"; break;
		default: pMsgType = "Unknown"; break;
	}

	char buf1[256];
	sprintf(buf1, "%s(%d) : %s",
		lineInfo.m_pathName.c_str(), lineInfo.m_lineNum, pMsgType);

	char buf2[256];
	vsprintf(buf2, msgStr, marker);

	if (buf2[0] != '\0')
		printf("%s %s\n", buf1, buf2);

	if (m_pMsgFp && buf2[0] != '\0')
		fprintf(m_pMsgFp, "%s %s\n", buf1, buf2);

	if (msgType == PARSE_FATAL) {
		printf("%s(%d) : Fatal: unable to proceed due to previous errors\n",
			lineInfo.m_pathName.c_str(), lineInfo.m_lineNum);
		ErrorExit();
	}

	if (m_errorCnt > 30) {
		printf("%s(%d) : Error: maximum error count exceeded\n",
			lineInfo.m_pathName.c_str(), lineInfo.m_lineNum);
		ErrorExit();
	}

	fflush(stdout);
}

void ErrorExit()
{
	if (g_pHtfeDesign)
		g_pHtfeDesign->DeleteFiles();
	exit(1);
}
