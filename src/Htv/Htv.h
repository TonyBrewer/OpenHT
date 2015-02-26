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
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <signal.h>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <direct.h>
#pragma warning(disable: 4786)
#pragma warning ( disable : 4996 )
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include <map>
using std::map;
using std::pair;

#include <string>
using std::string;

#include <vector>
using std::vector;

#include <algorithm>

#ifdef _WIN32
#define PATH_MAX	256
#else
#include <limits.h>
#include <strings.h>
#define stricmp strcasecmp
typedef long long __int64;
#endif

typedef unsigned long long		uint64;
