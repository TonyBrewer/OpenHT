#pragma once
#include "JpegResizeHif.h"
#include "job.hpp"

extern JpegResizeHif * g_pJpegResizeHif;

bool coprocInit(int threads);
bool coprocResize(jobDefinition & job);