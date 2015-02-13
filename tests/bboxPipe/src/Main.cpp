#include "Ht.h"
using namespace Ht;

#include "Main.h"
#include "BlackBox_execute.h"

#define VL      128

void usage(char *);

int main(int argc, char **argv)
{
	int vecLen, i;
	uint64_t scalar;
	uint64_t *va, *vb, *vt;

	CHtHif *pHtHif = new CHtHif();
	int auCnt = pHtHif->GetUnitCnt();

	CHtAuUnit **pAuUnits = new CHtAuUnit * [auCnt];

	for (int i = 0; i < auCnt; i += 1)
		pAuUnits[i] = new CHtAuUnit(pHtHif);

	CHtAuUnit::CCallArgs_htmain *call_argv;

	scalar = (uint64_t)0xabcd7495;
	// check command line args
	if (argc == 1) {
		vecLen = VL; // default vecLen
	} else if (argc == 2) {
		vecLen = atoi(argv[1]);
		if (vecLen <= 0) {
			usage(argv[0]);
			return 0;
		}
	} else {
		usage(argv[0]);
		return 0;
	}

	va = (uint64_t *)malloc(vecLen * sizeof(uint64_t));
	vb = (uint64_t *)malloc(vecLen * sizeof(uint64_t));
	vt = (uint64_t *)malloc(vecLen * sizeof(uint64_t));
	call_argv = (CHtAuUnit::CCallArgs_htmain *)malloc(auCnt * sizeof(CHtAuUnit::CCallArgs_htmain));


	for (i = 0; i < vecLen; i++) {
		va[i] = (uint64_t)(i);
		vb[i] = (uint64_t)(i + i);
		vt[i] = (uint64_t)(0);
	}

	// Coprocessor memory arrays
	uint64_t *cp_va = (uint64_t*)pHtHif->MemAllocAlign(4 * 1024, vecLen * sizeof(uint64_t));
	uint64_t *cp_vb = (uint64_t*)pHtHif->MemAllocAlign(4 * 1024, vecLen * sizeof(uint64_t));
	uint64_t *cp_vt = (uint64_t*)pHtHif->MemAllocAlign(4 * 1024, vecLen * sizeof(uint64_t));

	pHtHif->MemCpy(cp_va, va, vecLen * sizeof(uint64_t));
	pHtHif->MemCpy(cp_vb, vb, vecLen * sizeof(uint64_t));
	pHtHif->MemSet(cp_vt, 0, vecLen * sizeof(uint64_t));

	// at the moment all calls use the same argument so I could have used a broadcast message
	// doing it this way to give more flexibility if/when we change which thread touches which memory
	for (i = 0; i < auCnt; i++) {
		call_argv[i].m_operation = 0;
		call_argv[i].m_auStride = auCnt;
		call_argv[i].m_vecLen = vecLen / auCnt;
		call_argv[i].m_scalar = scalar;
		// calculate the starting address for each unit
		call_argv[i].m_vaBase = (uint64_t)(cp_va + i);
		call_argv[i].m_vbBase = (uint64_t)(cp_vb + i);
		call_argv[i].m_vtBase = (uint64_t)(cp_vt + i);

		while (!pAuUnits[i]->SendCall_htmain(call_argv[i]))
			usleep(100);
	}

	// wait for all of the AUs to complete and report how many elements each worked on
	uint32_t numElements;
	uint64_t totElements = 0;
	for (i = 0; i < auCnt; i++) {
		while (!pAuUnits[i]->RecvReturn_htmain(numElements))
			usleep(1000);
		printf("unit=%-2d: Number of elements processed: %lu \n", i, (long)numElements);
		totElements += numElements;
		fflush(stdout);
	}

	pHtHif->MemCpy(vt, cp_vt, vecLen * sizeof(uint64_t));

	bool foundError = false;
	for (i = 0; i < vecLen; i++) {
		Arith_t temp_vt;
		bool temp_vm;
		BlackBox_execute(0, 0, (Arith_t)va[i], (Arith_t)vb[i], (Arith_t)0, temp_vt, temp_vm);
		if (temp_vt != vt[i]) {
			printf("Error line %ld, %llx %llx: %llx should be %llx\n", (long)i, (long long)va[i], (long long)vb[i],
			       (long long)vt[i], (long long)temp_vt);
			foundError = true;
		}
	}
	if (!foundError) {
		printf("Processed %llu elements without errors!\n", (long long)totElements);
		printf("PASSED\n");
	} else {
		printf("FAILED\n");
	}

	// free memory
	free(va);
	free(vb);
	free(vt);
	pHtHif->MemFreeAlign(cp_va);
	pHtHif->MemFreeAlign(cp_vb);
	pHtHif->MemFreeAlign(cp_vt);

	delete pHtHif;

	return 0;
}

// Print usage message and exit with error.
void
usage(char *p)
{
	printf("usage: %s [count (default %d)] \n", p, VL);
	exit(1);
}

// copied from Joe's code
// stripped as much useless stuff as I could from this
// changed the parameters to match Mike's verilog where it makes sense
// vm is always 0, but I kept it...
#define ARC0_AMux         7             // 1 - 0 to A addr input
#define ARC0_BMux         8             // 00 - inB to B addr input
                                        // 01 - Scalar to B addr input
                                        // 10 - 0 to B addr input
                                        // 11 - 0 to B addr input
#define ARC0_BBop         10            //  0 - centrifuge
                                        //  1 - pop count


void
BlackBox_execute(const int &elem, const int &arc0, const Arith_t &va,
		 const Arith_t &vb, const Arith_t &scalar,
		 Arith_t &vt, bool &vm)
{
	//
	//	personality instruction execution
	//
	int c;
	Arith_t addrAin;
	Arith_t addrBin = 0;
	Arith_t ctlBit;
	Arith_t rBit;

	//
	//	compute a input
	//
	addrAin = (((arc0 >> ARC0_AMux) & 0x1) == 0) ? va : 0;
	//
	//	compute b input
	//
	switch (arc0 >> ARC0_BMux & 0x3) {
	case 0:                 // sel b in
		addrBin = vb;
		break;
	case 1:                 // sel scalar
		addrBin = scalar;
		break;
	case 2:         // sel 0
		addrBin = 0;
		break;
	case 3:                 // sel invalid
		addrBin = 0;
		break;
	}
	//
	//	do the centrifuge, generate vt, vm
	//
	switch (arc0 >> ARC0_BBop & 0x1) {
	case 0:                 // Centrifuge
		vt = 0;
		ctlBit = 0x0000000000000001ull;
		rBit = ctlBit;
		//
		//  Do the 0 control bits right to left
		//
		for (c = 0; c < 64; c++) {
			if ((addrBin & ctlBit) == 0) {
				if ((addrAin & ctlBit) != 0)
					// ctl bit is 0, data bit is 1
					vt |= rBit;
				rBit <<= 1;
			}
			ctlBit <<= 1;
		}
		ctlBit = 0x8000000000000000ull;
		rBit = ctlBit;
		//
		//  Do the 1 control bits left to right
		//
		for (c = 0; c < 64; c++) {
			if ((addrBin & ctlBit) != 0) {
				if ((addrAin & ctlBit) != 0)
					// ctl bit is 1, data bit is 1
					vt |= rBit;
				rBit >>= 1;
			}
			ctlBit >>= 1;
		}
		vm = 0;                         // vm bit
		break;

	case 1:                 // Pop Count
		vt = 0;
		vm = 0;
		ctlBit = 0x0000000000000001ull;
		for (c = 0; c < 64; c++) {
			if ((addrAin & ctlBit) != 0)
				vt++;
			ctlBit <<= 1;
		}
		vt += addrBin;
		break;
	}
}
