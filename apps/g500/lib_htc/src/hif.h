#include <stdint.h>
#include <omp.h>

#define ONE 0x0000000000000001
#define ZERO 0
#define XADJ_SIZE uint64_t

#include "Ht.h"

using namespace Ht;

extern "C" CHtAuUnit ** __htc_units;
extern "C" int __htc_get_unit_count();
