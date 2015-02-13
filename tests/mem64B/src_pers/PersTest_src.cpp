#include "Ht.h"
#include "PersTest.h"

// Test description
//  1. Read 64-bytes from coproc memory (initialized by host)
//  2. Read various alignments and qwCnts from coproc memory
//  3. Repeat steps 1 and 2 for host memory (initialized by host)

void CPersTest::PersTest()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case RD_CP0:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			P_testPassed = true;
			P_cntr0 = 0;

			// Make request for multi-quadword access to coproc memory
			ReadMem_gblArray_coproc(SR_coprocAddr + 8, 2, 3);

			ReadMemPause(RD_CP0V);
		}
		break;
		case RD_CP0V:
		{
			// call routine to verify read results
			if (SendCallBusy_verify()) {
				HtRetry();
				break;
			}

			SendCall_verify(RD_CP1, 2, 3, 0xdeadbeef00550001ULL);

			break;
		}
		case RD_CP1:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			P_testPassed &= P_verifyPassed;
			P_failMask |= P_verifyPassed ? 0 : 0x1;

			// Make request for multi-quadword access to host memory
			ReadMem_gblArray_host(SR_hostAddr + 16, 1, 6);

			ReadMemPause(RD_CP1V);
		}
		break;
		case RD_CP1V:
		{
			// call routine to verify read results
			if (SendCallBusy_verify()) {
				HtRetry();
				break;
			}

			SendCall_verify(RD_CP2A, 1, 6, 0xdeadbeef00AA0002ULL);

			break;
		}
		case RD_CP2A:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			P_testPassed &= P_verifyPassed;
			P_failMask |= P_verifyPassed ? 0 : 0x2;

			// Make request for multi-quadword access to host memory
			ReadMem_gblArray_host(SR_hostAddr + 0, 1, 8);

			HtContinue(RD_CP2B);
		}
		break;
		case RD_CP2B:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			// Make request for multi-quadword access to host memory
			ReadMem_gblArray_coproc(SR_coprocAddr + 8, 9, 7);

			HtContinue(RD_CP2C);
		}
		break;
		case RD_CP2C:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			// Make request for multi-quadword access to host memory
			ReadMem_gblArray_host(SR_hostAddr + 16, 16, 5);

			ReadMemPause(RD_CP2AV);
		}
		break;
		case RD_CP2AV:
		{
			// call routine to verify read results
			if (SendCallBusy_verify()) {
				HtRetry();
				break;
			}

			SendCall_verify(RD_CP2BV, 1, 8, 0xdeadbeef00AA0000ULL);

			break;
		}
		case RD_CP2BV:
		{
			// call routine to verify read results
			if (SendCallBusy_verify()) {
				HtRetry();
				break;
			}

			P_testPassed &= P_verifyPassed;
			P_failMask |= P_verifyPassed ? 0 : 0x4;

			SendCall_verify(RD_CP2CV, 9, 7, 0xdeadbeef00550001ULL);

			break;
		}
		case RD_CP2CV:
		{
			// call routine to verify read results
			if (SendCallBusy_verify()) {
				HtRetry();
				break;
			}

			P_testPassed &= P_verifyPassed;
			P_failMask |= P_verifyPassed ? 0 : 0x8;

			SendCall_verify(RD_CP3W, 16, 5, 0xdeadbeef00AA0002ULL);

			break;
		}
		case RD_CP3W:
		{
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}

			P_testPassed &= P_verifyPassed;
			P_failMask |= P_verifyPassed ? 0 : 0x10;

			// Make request for single quadword access to coproc memory
			WriteMem(SR_coprocAddr + 0x100, 0xdeadbeef00BB0000ULL);

			WriteMemPause(RD_CP3R);
		}
		break;
		case RD_CP3R:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			// Make request for single quadword access to coproc memory
			ReadMem_sharedData_coproc(SR_coprocAddr + 0x100);

			ReadMemPause(RD_CP3V);
		}
		break;
		case RD_CP3V:
		{
			HtAssert(S_sharedData == 0xdeadbeef00BB0000ULL, 0);
			if (S_sharedData != 0xdeadbeef00BB0000ULL) {
				P_testPassed = false;
				P_failMask |= 0x20;
			}

			HtContinue(RD_CP4);
		}
		break;
		case RD_CP4:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			// Make request for multi-quadword access to host memory
			ReadMem_allDimen_varAddr1(SR_hostAddr + 16, 1, 2, 0, 1, 2, 1, 2);

			P_rdAdr1 = 1;
			P_rdAdr2 = 2;

			ReadMemPause(RD_CP4V1);
		}
		break;
		case RD_CP4V1:
		{
			HtAssert(GR_allDimen_item(0, 1, 2, 1) == 0xbeef00AA0002ULL, 0);
			if (GR_allDimen_item(0, 1, 2, 1) != 0xbeef00AA0002ULL) {
				P_testPassed = false;
				P_failMask |= 0x40;
			}

			P_rdAdr1 = 2;
			P_rdAdr2 = 2;

			HtContinue(RD_CP4V2);

			break;
		}
		case RD_CP4V2:
		{
			HtAssert(GR_allDimen_item(0, 1, 2, 1) == 0xbeef00AA0003ULL, 0);
			if (GR_allDimen_item(0, 1, 2, 1) != 0xbeef00AA0003ULL) {
				P_testPassed = false;
				P_failMask |= 0x80;
			}

			HtContinue(RD_CP5);

			break;
		}
		case RD_CP5:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			// Make request for multi-quadword access to host memory
			ReadMem_allDimen_varAddr2(SR_hostAddr + 8, 1, 2, 0, 1, 2, 1, 2);

			P_rdAdr1 = 1;
			P_rdAdr2 = 2;

			ReadMemPause(RD_CP5V1);
		}
		break;
		case RD_CP5V1:
		{
			HtAssert(GR_allDimen_item(0, 1, 2, 1) == 0xbeef00AA0001ULL, 0);
			if (GR_allDimen_item(0, 1, 2, 1) != 0xbeef00AA0001ULL) {
				P_testPassed = false;
				P_failMask |= 0x100;
			}

			P_rdAdr1 = 1;
			P_rdAdr2 = 3;

			HtContinue(RD_CP5V2);

			break;
		}
		case RD_CP5V2:
		{
			HtAssert(GR_allDimen_item(0, 1, 2, 1) == 0xbeef00AA0002ULL, 0);
			if (GR_allDimen_item(0, 1, 2, 1) != 0xbeef00AA0002ULL) {
				P_testPassed = false;
				P_failMask |= 0x200;
			}

			HtContinue(RD_CP6);

			break;
		}
		case RD_CP6:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			// Make request for multi-quadword access to host memory
			ReadMem_allDimen_varIdx1(SR_hostAddr + 24, 0, 2, 0, 1, 2, 1, 2);

			P_rdAdr1 = 0;
			P_rdAdr2 = 2;

			ReadMemPause(RD_CP6V1);
		}
		break;
		case RD_CP6V1:
		{
			HtAssert(GR_allDimen_item(0, 1, 2, 1) == 0xbeef00AA0003ULL, 0);
			if (GR_allDimen_item(0, 1, 2, 1) != 0xbeef00AA0003ULL) {
				P_testPassed = false;
				P_failMask |= 0x400;
			}

			P_rdAdr1 = 0;
			P_rdAdr2 = 2;

			HtContinue(RD_CP6V2);

			break;
		}
		case RD_CP6V2:
		{
			HtAssert(GR_allDimen_item(1, 1, 2, 1) == 0xbeef00AA0004ULL, 0);
			if (GR_allDimen_item(1, 1, 2, 1) != 0xbeef00AA0004ULL) {
				P_testPassed = false;
				P_failMask |= 0x800;
			}

			HtContinue(RD_CP7);

			break;
		}
		case RD_CP7:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			// Make request for multi-quadword access to host memory
			ReadMem_allDimen_varIdx2(SR_hostAddr + 32, 0, 0, 2, 1, 2, 1, 2);

			P_rdAdr1 = 0;
			P_rdAdr2 = 0;

			ReadMemPause(RD_CP7V1);
		}
		break;
		case RD_CP7V1:
		{
			HtAssert(GR_allDimen_item(2, 1, 2, 1) == 0xbeef00AA0004ULL, 0);
			if (GR_allDimen_item(2, 1, 2, 1) != 0xbeef00AA0004ULL) {
				P_testPassed = false;
				P_failMask |= 0x1000;
			}

			P_rdAdr1 = 0;
			P_rdAdr2 = 0;

			HtContinue(RD_CP7V2);

			break;
		}
		case RD_CP7V2:
		{
			HtAssert(GR_allDimen_item(2, 2, 2, 1) == 0xbeef00AA0005ULL, 0);
			if (GR_allDimen_item(2, 2, 2, 1) != 0xbeef00AA0005ULL) {
				P_testPassed = false;
				P_failMask |= 0x2000;
			}

			HtContinue(RD_CP8);

			break;
		}
		case RD_CP8:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			// Make request for multi-quadword access to host memory
			ReadMem_allDimen_fldIdx1(SR_hostAddr + 40, 2, 3, 2, 1, 2, 1, 2);

			P_rdAdr1 = 2;
			P_rdAdr2 = 3;

			ReadMemPause(RD_CP8V1);
		}
		break;
		case RD_CP8V1:
		{
			HtAssert(GR_allDimen_item(2, 1, 2, 1) == 0xbeef00AA0005ULL, 0);
			if (GR_allDimen_item(2, 1, 2, 1) != 0xbeef00AA0005ULL) {
				P_testPassed = false;
				P_failMask |= 0x4000;
			}

			HtContinue(RD_CP8V2);

			break;
		}
		case RD_CP8V2:
		{
			HtAssert(GR_allDimen_item(2, 1, 3, 1) == 0xbeef00AA0006ULL, 0);
			if (GR_allDimen_item(2, 1, 3, 1) != 0xbeef00AA0006ULL) {
				P_testPassed = false;
				P_failMask |= 0x8000;
			}

			HtContinue(RD_CP9);

			break;
		}
		case RD_CP9:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			// Make request for multi-quadword access to host memory
			ReadMem_allDimen_fldIdx2(SR_hostAddr + 48, 2, 3, 2, 1, 2, 1, 2);

			P_rdAdr1 = 2;
			P_rdAdr2 = 3;

			ReadMemPause(RD_CP9V1);
		}
		break;
		case RD_CP9V1:
		{
			HtAssert(GR_allDimen_item(2, 1, 2, 1) == 0xbeef00AA0006ULL, 0);
			if (GR_allDimen_item(2, 1, 2, 1) != 0xbeef00AA0006ULL) {
				P_testPassed = false;
				P_failMask |= 0x10000;
			}

			HtContinue(RD_CP9V2);

			break;
		}
		case RD_CP9V2:
		{
			HtAssert(GR_allDimen_item(2, 1, 2, 2) == 0xbeef00AA0007ULL, 0);
			if (GR_allDimen_item(2, 1, 2, 2) != 0xbeef00AA0007ULL) {
				P_testPassed = false;
				P_failMask |= 0x20000;
			}

			HtContinue(RD_CP10);

			break;
		}
		case RD_CP10:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			// Make request for multi-quadword access to host memory
			ReadMem_sharedFunc_host(SR_hostAddr + 48, 5, 2);

			S_sharedData = 0;

			ReadMemPause(RD_CP10V);
		}
		break;
		case RD_CP10V:
		{
			uint64_t sum = 0xbd5b7dde0154550dULL;
			HtAssert(S_sharedData == sum, 0);
			if (S_sharedData != sum) {
				P_testPassed = false;
				P_failMask |= 0x40000;
			}

			HtContinue(RD_CP11);

			break;
		}
		case RD_CP11:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			// Make request for multi-quadword access to host memory
			ReadMem_sharedData_host(SR_hostAddr + 24, 3, 3);

			S_sharedData = 0;

			ReadMemPause(RD_CP11V1);
		}
		break;
		case RD_CP11V1:
		{
			m_sharedDataRam.read_addr(3);
			HtAssert(m_sharedDataRam.read_mem() == 0xdeadbeef00AA0003ULL, 0);
			if (m_sharedDataRam.read_mem() != 0xdeadbeef00AA0003ULL) {
				P_testPassed = false;
				P_failMask |= 0x80000;
			}

			HtContinue(RD_CP11V2);
		}
		break;
		case RD_CP11V2:
		{
			m_sharedDataRam.read_addr(4);
			HtAssert(m_sharedDataRam.read_mem() == 0xdeadbeef00AA0004ULL, 0);
			if (m_sharedDataRam.read_mem() != 0xdeadbeef00AA0004ULL) {
				P_testPassed = false;
				P_failMask |= 0x100000;
			}

			HtContinue(RD_CP11V3);
		}
		break;
		case RD_CP11V3:
		{
			m_sharedDataRam.read_addr(5);
			HtAssert(m_sharedDataRam.read_mem() == 0xdeadbeef00AA0005ULL, 0);
			if (m_sharedDataRam.read_mem() != 0xdeadbeef00AA0005ULL) {
				P_testPassed = false;
				P_failMask |= 0x200000;
			}

			HtContinue(RD_CP13T1);
		}
		break;
		case RD_CP12:
		{
			// this part of the test still has bugs (Bryan needs to fix)
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			// Make many requests for multi-quadword access to host memory
			//fprintf (stderr, "reading #%d 0x%012llx\n", (int)P_cntr0,
			// (long long)(SR_hostAddr+(P_cntr0<<6)));
			ReadMem_bigGblArray_host(SR_hostAddr + (P_cntr0 << 6), (P_cntr0 << 3) & 0x1ff, (P_cntr0 & 7) + 1);
			// check last read first
			P_cntr1 = 7;
			P_bigRdAdr1 = (ht_uint9)((63 << 3) + P_cntr1);

			if (P_cntr0++ < 63) {
				HtContinue(RD_CP12);
			} else {
				P_cntr0 = 63;
				ReadMemPause(RD_CP12V);
			}
		}
		break;
		case RD_CP12V:
		{
			uint64_t expected = (P_cntr1 > (P_cntr0 & 0x7)) ? 0 :
					    (0xdeadbeef00AA0000ULL | P_cntr0 << 3 | P_cntr1);
			HtAssert(GR_bigGblArray_data() == expected, 0);
			if (GR_bigGblArray_data() != expected) {
				P_testPassed = false;
				P_failMask |= 0x400000;
			}

			//fprintf (stderr,"cntr0=%d cntr1=%d expect=0x%016llx actual=0x%016llx\n",
			//  (int)P_cntr0, (int)P_cntr1,
			//     (long long)expected,
			//  (long long)GR_bigGblArray_data);
			if (P_cntr0 == 0 && P_cntr1 == 0)
				HtContinue(RD_CP13T1);
			else
				HtContinue(RD_CP12V);

			if (!(P_cntr1-- > 0)) {
				P_cntr0--;
				//P_cntr1=P_cntr&0x7;
				P_cntr1 = 7;
			}
			P_bigRdAdr1 = (ht_uint9)((P_cntr0 << 3) + P_cntr1);
		}
		break;
		case RD_CP13T1:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			// fill 
			ReadMem_regDimen1_host(SR_hostAddr, 0, 0, 8);

			HtContinue(RD_CP13T2);
		}
		break;
		case RD_CP13T2:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			// fill 
			ReadMem_regDimen1_coproc(SR_coprocAddr, 1, 0, 8);

			ReadMemPause(RD_CP13T3);
		}
		break;
		case RD_CP13T3:
			{
				if (SendCallBusy_copy()) {
					HtRetry();
					break;
				}

				SendCall_copy(RD_CP13V, 13);
			}
			break;
		case RD_CP13V:
			{
				for (int i = 0; i < 2; i += 1) {
					for (int j = 0; j < 8; j += 1) {
						if (GR_regDimen1_data(i, j) != GR_regDimen2_data(j, i)) {
							P_testPassed = false;
							P_failMask |= 0x800000;
						}
					}
				}

				HtContinue(RD_CP14T1);
			}
			break;
		case RD_CP14T1:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			// fill 
			ReadMem_regFldDimen1_host(SR_hostAddr, 0, 0, 8);

			HtContinue(RD_CP14T2);
		}
		break;
		case RD_CP14T2:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			// fill 
			ReadMem_regFldDimen1_coproc(SR_coprocAddr, 1, 0, 8);

			ReadMemPause(RD_CP14T3);
		}
		break;
		case RD_CP14T3:
			{
				if (SendCallBusy_copy()) {
					HtRetry();
					break;
				}

				SendCall_copy(RD_CP14V, 14);
			}
			break;
		case RD_CP14V:
			{
				for (int i = 0; i < 2; i += 1) {
					for (int j = 0; j < 8; j += 1) {
						if (GR_regFldDimen1_data(i, j) != GR_regFldDimen2_data(j, i)) {
							P_testPassed = false;
							P_failMask |= 0x1000000;
						}
					}
				}

				HtContinue(RD_CP15T1);
			}
			break;
		case RD_CP15T1:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			// fill 
			ReadMem_varFld1_host(SR_hostAddr, 0, 0, 8);

			HtContinue(RD_CP15T2);
		}
		break;
		case RD_CP15T2:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			// fill 
			ReadMem_varFld1_coproc(SR_coprocAddr, 1, 0, 8);

			ReadMemPause(RD_CP15T3);
		}
		break;
		case RD_CP15T3:
			{
				if (SendCallBusy_copy()) {
					HtRetry();
					break;
				}

				SendCall_copy(RD_CP15V, 15);
			}
			break;
		case RD_CP15V:
			{
				for (int i = 0; i < 2; i += 1) {
					for (int j = 0; j < 8; j += 1) {
						if (GR_varFld1_data(i, j) != GR_varFld2_data(j, i)) {
							P_testPassed = false;
							P_failMask |= 0x2000000;
						}
					}
				}

				HtContinue(RD_CP16T1);
			}
			break;
		case RD_CP16T1:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			// fill 
			ReadMem_varAddr1_host(SR_hostAddr, 0, 0, 2);

			HtContinue(RD_CP16T2);
		}
		break;
		case RD_CP16T2:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			// fill 
			ReadMem_varAddr1_coproc(SR_coprocAddr, 1, 0, 2);

			ReadMemPause(RD_CP16T3);
		}
		break;
		case RD_CP16T3:
			{
				if (SendCallBusy_copy()) {
					HtRetry();
					break;
				}

				SendCall_copy(RD_CP16V1, 16);

				P_varAdr1 = 0;
				P_varAdr2 = 0;
			}
			break;
		case RD_CP16V1:
			{
				// varAdr1[v0][a0] == varAdr2[v0][a0]
				if (GR_varAddr1_data(0) != GR_varAddr2_data(0)) {
					P_testPassed = false;
					P_failMask |= 0x4000000;
				}

				P_varAdr1 = 1;
				P_varAdr2 = 0;

				HtContinue(RD_CP16V2);
			}
			break;
		case RD_CP16V2:
			{
				// varAddr1[v0][a1] == varAddr2[v1][a0]
				if (GR_varAddr1_data(0) != GR_varAddr2_data(1)) {
					P_testPassed = false;
					P_failMask |= 0x8000000;
				}

				P_varAdr1 = 0;
				P_varAdr2 = 1;

				HtContinue(RD_CP16V3);
			}
			break;
		case RD_CP16V3:
			{
				// varAddr1[v1][a0] == varAddr2[v0][a1]
				if (GR_varAddr1_data(1) != GR_varAddr2_data(0)) {
					P_testPassed = false;
					P_failMask |= 0x10000000;
				}

				P_varAdr1 = 1;
				P_varAdr2 = 1;

				HtContinue(RD_CP16V4);
			}
			break;
		case RD_CP16V4:
			{
				// varAddr1[v1][a1] == varAddr2[v1][a1]
				if (GR_varAddr1_data(1) != GR_varAddr2_data(1)) {
					P_testPassed = false;
					P_failMask |= 0x20000000;
				}

				P_varAdr1 = 1;
				P_varAdr2 = 1;

				HtContinue(RD_CP17T1);
			}
			break;
		case RD_CP17T1:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			// fill 
			ReadMem_fldAddr1_host(SR_hostAddr, 0, 0, 2);

			HtContinue(RD_CP17T2);
		}
		break;
		case RD_CP17T2:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			// fill 
			ReadMem_fldAddr1_coproc(SR_coprocAddr, 1, 0, 2);

			ReadMemPause(RD_CP17T3);
		}
		break;
		case RD_CP17T3:
			{
				if (SendCallBusy_copy()) {
					HtRetry();
					break;
				}

				SendCall_copy(RD_CP17V1, 17);

				P_fldAdr1 = 0;
				P_fldAdr2 = 0;
			}
			break;
		case RD_CP17V1:
			{
				// fldAddr1[v0][a0] == fldAddr2[v0][a0]
				if (GR_fldAddr1_data(0) != GR_fldAddr2_data(0)) {
					P_testPassed = false;
					P_failMask |= 0x40000000;
				}

				P_fldAdr1 = 1;
				P_fldAdr2 = 0;

				HtContinue(RD_CP17V2);
			}
			break;
		case RD_CP17V2:
			{
				// fldAddr1[v0][a1] == fldAddr2[v1][a0]
				if (GR_fldAddr1_data(0) != GR_fldAddr2_data(1)) {
					P_testPassed = false;
					P_failMask |= 0x80000000;
				}

				P_fldAdr1 = 0;
				P_fldAdr2 = 1;

				HtContinue(RD_CP17V3);
			}
			break;
		case RD_CP17V3:
			{
				// fldAddr1[v1][a0] == fldAddr2[v0][a1]
				if (GR_fldAddr1_data(1) != GR_fldAddr2_data(0)) {
					P_testPassed = false;
					P_failMask |= 0x100000000ULL;
				}

				P_fldAdr1 = 1;
				P_fldAdr2 = 1;

				HtContinue(RD_CP17V4);
			}
			break;
		case RD_CP17V4:
			{
				// fldAddr1[v1][a1] == fldAddr2[v1][a1]
				if (GR_fldAddr1_data(1) != GR_fldAddr2_data(1)) {
					P_testPassed = false;
					P_failMask |= 0x200000000ULL;
				}

				P_fldAdr1 = 1;
				P_fldAdr2 = 1;

				HtContinue(RD_RETURN);
			}
			break;
		case RD_RETURN:
		{
			// test completed
			if (SendReturnBusy_htmain()) {
				HtRetry();
				break;
			}

			// Return to host interface
			SendReturn_htmain(P_testPassed, PR_failMask);
		}
		break;
		default:
			P_testPassed = GR_allDimen_item(0, 0, 0, 0) != 0 &&
				       GR_allDimen_item(0, 2, 0, 0) != 0 &&
				       GR_allDimen_item(0, 3, 0, 0) != 0 &&
				       GR_allDimen_item(1, 0, 0, 0) != 0 &&
				       GR_allDimen_item(1, 2, 0, 0) != 0 &&
				       GR_allDimen_item(1, 3, 0, 0) != 0 &&
				       GR_allDimen_item(2, 0, 0, 0) != 0 &&
				       GR_allDimen_item(2, 3, 0, 0) != 0;
			assert(0);
		}
	}
}

void CPersTest::ReadMemResp_sharedFunc_host(sc_uint<3> rspIdx, sc_uint<TEST_RD_GRP_ID_W> rdRspGrpId, sc_uint<TEST_MIF_DST_SHAREDFUNC_HOST_INFO_W> c_m1_rdRsp_info, sc_uint<64> rdRspData)
{
	S_sharedData += rdRspData + rdRspGrpId;
	S_sharedData |= (c_m1_rdRsp_info << (4 * (rspIdx + 2)));
}
