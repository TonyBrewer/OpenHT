/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#pragma once

#include <string>
using namespace std;

class JpegResizeHif;
struct AppJob;

bool processJob( int threadId, AppJob &appJob );
#ifdef MAGICK_FALLBACK
bool magickFallback(const void *jpegImage, int jpegImageSize, AppJob &appJob);

#endif