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

#ifdef _WIN32

// Linux routines for windows platform

#define PATH_MAX MAX_PATH
#define strcasecmp strcmpi

#ifdef __cplusplus
extern "C" {
#endif

    struct timezone {
        int tz_minuteswest;     /* minutes W of Greenwich */
        int tz_dsttime;         /* type of dst correction */
    };

    int gettimeofday(struct timeval *tv, struct timezone *tz);

#ifdef __cplusplus
}
#endif	// __cplusplus

#endif // _WIN32