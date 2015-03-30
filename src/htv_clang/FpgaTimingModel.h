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

class FpgaTimingModel {
public:
	int getRegSetup() { return 100; }
	int getRegCkOut() { return 125; }

private:
};
