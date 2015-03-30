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

#define HtAssert(ok) if (!(ok)) HtAssert_(__FILE__, __LINE__)


inline void HtAssert_(char *file, int line)
{
	//int argc;
	//char **argv;
	//g_pHtfeArgs->GetArgs(argc, argv);

	//printf("Fatal : internal inconsistency detected\n   File: %s\n   Line: %d\n   Command Line:", file, line);
	printf("Fatal : internal inconsistency detected\n   File: %s\n   Line: %d\n", file, line);

	//for (int i = 0; i < argc; i += 1)
	//	printf(" %s", argv[i]);

	//printf("\n");

	//if (m_pMsgFp) {
	//	fprintf(m_pMsgFp, "Fatal : internal inconsistency detected\n   File: %s\n   Line: %d\n   Command Line:", file, line);

	//	for (int i = 0; i < argc; i += 1)
	//		fprintf(m_pMsgFp, " %s", argv[i]);

	//	fprintf(m_pMsgFp, "\n");
	//}

	exit(1);
}
