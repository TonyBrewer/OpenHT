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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include<signal.h>

#include <queue>
#include <string>
#include <vector>
#include <map>

#include "MtRand.h"

#ifdef _WIN32
#include <windows.h>
#include <WinDef.h>
#include <io.h>
#include <direct.h>	// _mkdir
#include "Linux.h"
#include <time.h>
#pragma warning(disable: 4996)
#else
#include <sys/time.h>	// gettimeofday
#include <algorithm>	// for sort
#include <errno.h>
#include <unistd.h>
#define _mkdir(a) mkdir(a, 0777)
#endif

using namespace std;

#ifdef _WIN32
#define PRzd "%I64d"
#define PRzx "%I64x"
#else
#define PRzd "%Zd"
#define PRzx "%Zx"
#endif

#define HtlAssert(ok) if ((ok) == 0) AssertMsg(__FILE__, __LINE__)
void AssertMsg(char *file, int line);
void ErrorExit();

