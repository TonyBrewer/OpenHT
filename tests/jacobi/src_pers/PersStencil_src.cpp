#include "Ht.h"
#include "PersStencil.h"

// Jacobi Relaxation

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

#define fmax(a, b) ((a) > (b) ? (a) : (b))
#define fabs(a) (((a) > 0) ? (a) : (0 - (a)))
#define mulX(a, b) ((sc_FixedPnt)(((a) * (b)) >> FRACTION_W))

#define addr(x, y, z) ((memAddrT)(((uint64_t)x) + (((y) * N) + z) * sizeof(uint64_t)))
#define tempaIndex(i) (((PR_htId) << IBLK_W) | (i))

// float stencil (float a[M][N], float newa[M][N], int P_p_is, int P_p_js, int P_p_m, int P_p_n, float P_p_w0, float P_p_w1, float P_p_w2) {
// float tempa[j_dim-1][P_p_m];   // cache reads into AE memory

void
CPersStencil::PersStencil()
{
	if (PR_htValid) {
		//fprintf(stderr,"stencil was called and r_ts_htValid is %d and state is %d and thread id is %d\n",
		//        r_ts_htValid, (int)r_ts_htInst, PR_htId.to_int());
		switch (PR_htInst) {
		case START:  // 1
		{
			P_change = 0;
			P_i = 0;
			P_ram_rdVal = PR_htId;          // indexed only by thread id
			HtContinue(FIRST_TWO_ROWS_LOOP_TOP);
		}
		break;

		// fill in first two rows of temp array
		// for (i = 0; i < m; i++) {
		//   tempa[0][i] = a[0][i];    // main memory read
		//   tempa[1][i] = a[1][i];    // main memory read
		// }
		case FIRST_TWO_ROWS_LOOP_TOP:  // 3
		{
			if (!(P_i < P_p_m)) {
				HtContinue(FIRST_TWO_ROWS_LOOP_EXIT);
				break;
			}

			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			// First read request
			memAddrT memAddr = addr(P_p_a, 0 + P_p_js, P_i + P_p_is);
			sc_uint<STENCIL_HTID_W> ramWrAddr = PR_htId;
			// Mif0RdReq(rsmInst, memAddr, rdDstId, ramIdx, bFirst, bLast)
			//            Mif0RdReq(CHAINED_READ,   /* rsmInst */

			ReadMem_j0(memAddr,                     /* memAddr */
				   ramWrAddr);                  /* ramIdx */

			HtContinue(FIRST_TWO_ROWS_READ2);
		}
		break;

		case FIRST_TWO_ROWS_READ2:    // 4
		{
			// Second read request
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			// Set up for write to tempa[P_i] on next clock
			P_ram_i = tempaIndex(P_i);

			memAddrT memAddr = addr(P_p_a, 1 + P_p_js, P_i + P_p_is);
			sc_uint<STENCIL_HTID_W> ramWrAddr = PR_htId;
			// Mif0RdReq(rsmInst, memAddr, rdDstId, ramIdx, bFirst, bLast)
			ReadMem_j1(memAddr,                     /* memAddr */
				   ramWrAddr);                  /* ramIdx */

			ReadMemPause(FIRST_TWO_ROWS_ASSIGN);
		}
		break;

		case FIRST_TWO_ROWS_ASSIGN:    // 5
		{
			//            tempa[0][P_i] = MifRdVal(1);
			//            tempa[1][P_i] = MifRdVal(2);

			GW_tempa_j0(P_ram_i, GR_mifRdVal_j0());
			GW_tempa_j1(P_ram_i, GR_mifRdVal_j1());

			// increment i and loop back
			P_i = P_i + 1;
			HtContinue(FIRST_TWO_ROWS_LOOP_TOP);
		}
		break;

		case FIRST_TWO_ROWS_LOOP_EXIT:   // 6
		{
			// for (j = 2; j < n; j++) {
			P_j = 2;

			// Set up for read from tempa[0] on next clock
			P_ram_i = tempaIndex(0);

			HtContinue(J_LOOP_TOP);
		}
		break;

		case J_LOOP_TOP:    // 7
		{
			if (!(P_j < P_p_n)) {
				HtContinue(J_LOOP_EXIT);
				break;
			}

			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			P_t00 = GR_tempa_j0();
			P_t10 = GR_tempa_j1();

			// Set up to read from tempa[1] on next clock
			P_ram_i = tempaIndex(1);

#if 0
			// These have to be delayeduntil next slot
			// copy initial values for first two columns into "registers"
			//          j  i                  j  i
			P_t01 = tempa[0][1];
			P_t11 = tempa[1][1];
#endif


			// P_t20 = a[j][0];         P_t21 = a[j][1];  // main memory reads

			// First read request
			memAddrT memAddr = addr(P_p_a, P_j + P_p_js, 0 + P_p_is);
			sc_uint<STENCIL_HTID_W> ramWrAddr = PR_htId;    // P_t20
			// Mif0RdReq(rsmInst, memAddr, rdDstId, ramIdx, bFirst, bLast)
			ReadMem_j0(memAddr,                             /* memAddr */
				   ramWrAddr);                          /* ramIdx */

			HtContinue(J_LOOP_READ2);
		}
		break;

		case J_LOOP_READ2:    // 8
		{
			// Second read request
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			P_t01 = GR_tempa_j0();
			P_t11 = GR_tempa_j1();

			memAddrT memAddr = addr(P_p_a, P_j + P_p_js, 1 + P_p_is);
			sc_uint<STENCIL_HTID_W> ramWrAddr = PR_htId;    // P_t21
			// Mif0RdReq(rsmInst, memAddr, rdDstId, ramIdx, bFirst, bLast)
			ReadMem_j1(memAddr,                             /* memAddr */
				   ramWrAddr);                          /* ramIdx */

			// Set up to write to tempa[0] on next clock
			P_ram_i = tempaIndex(0);

			ReadMemPause(J_LOOP_ASSIGN);
		}
		break;

		case J_LOOP_ASSIGN:     // 9
		{
			P_t20 = GR_mifRdVal_j0();
			P_t21 = GR_mifRdVal_j1();

			// Save these first two values (shift columns up) for next j
			//            tempa[0][0] = P_t10;
			//            tempa[1][0] = P_t20;
			GW_tempa_j0(P_ram_i, P_t10);
			GW_tempa_j1(P_ram_i, P_t20);
#if 0
			// block ram writes to slot 1 delayed until next clock
			tempa[0][1] = P_t11;
			tempa[1][1] = P_t21;
#endif

			// Set up to write to tempa[1] on next clock
			P_ram_i = tempaIndex(1);

			// for (i = 2; i < m; i++) {
			P_i = 2;
			HtContinue(J_LOOP_ASSIGN2);
		}
		break;

		case J_LOOP_ASSIGN2:     // 15
		{
			//            tempa[0][1] = P_t11;
			//            tempa[1][1] = P_t21;

			GW_tempa_j0(P_ram_i, P_t11);
			GW_tempa_j1(P_ram_i, P_t21);

			// Set up to read from tempa[P_i] on next clock
			P_ram_i = tempaIndex(P_i);

			HtContinue(I_LOOP_TOP);
		}
		break;

		case I_LOOP_TOP:    // 11
		{
			if (!(P_i < P_p_m)) {
				HtContinue(I_LOOP_EXIT);
				break;
			}

			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			// Get third column
			//            P_t02 = tempa[0][P_i];      // AE memory
			//            P_t12 = tempa[1][P_i];      // AE memory
			P_t02 = GR_tempa_j0();
			P_t12 = GR_tempa_j1();

			// P_t22 = a[j][i];   // main memory read

			memAddrT memAddr = addr(P_p_a, P_j + P_p_js, P_i + P_p_is);
			sc_uint<STENCIL_HTID_W> ramWrAddr = PR_htId;    // P_t22
			// Mif0RdReq(rsmInst, memAddr, rdDstId, ramIdx, bFirst, bLast)
			ReadMem_j0(memAddr,                             /* memAddr */
				   ramWrAddr);                          /* ramIdx */

			// Set up to write to tempa[P_i] on next clock
			assert(P_ram_i == tempaIndex(P_i));            // should already be set
			P_ram_i = tempaIndex(P_i);

			ReadMemPause(I_LOOP_ASSIGN);
		}
		break;

		case I_LOOP_ASSIGN:       // 12
		{
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}

			// Get third column
			P_t22 = GR_mifRdVal_j0();

			// Save these values (shift column up) for next j
			//            tempa[0][P_i] = P_t12;
			//            tempa[1][P_i] = P_t22;
			GW_tempa_j0(P_ram_i, P_t12);
			GW_tempa_j1(P_ram_i, P_t22);

			P_res =
				mulX(P_p_w0, P_t11) +
				mulX(P_p_w1, (P_t10 + P_t01 + P_t12 + P_t21)) +
				mulX(P_p_w2, (P_t00 + P_t20 + P_t02 + P_t22));

			// newa[j-1][i-1] = res;  // main memory store
			memAddrT memAddr = addr(P_p_newa, P_p_js + P_j - 1, P_p_is + P_i - 1);
			WriteMem(memAddr, (sc_uint<MEM_DATA_W>)P_res);

			WriteMemPause(I_LOOP_CHANGE);
		}
		break;

		case I_LOOP_CHANGE:     // 13
		{
			P_diff = fabs(P_res - P_t11);
			P_change = fmax(P_change, P_diff);

			// prepare for next iteration; shift columns left
			P_t00 = P_t01; P_t01 = P_t02;
			P_t10 = P_t11; P_t11 = P_t12;
			P_t20 = P_t21; P_t21 = P_t22;

			P_i = P_i + 1;

			// Set up to read from tempa[P_i] on next clock
			P_ram_i = tempaIndex(P_i);

			HtContinue(I_LOOP_TOP);
		}
		break;

		case I_LOOP_EXIT:    // 14
		{
			// increment j and loop back
			P_j = P_j + 1;

			// Set up for read from tempa[0] on next clock
			P_ram_i = tempaIndex(0);

			HtContinue(J_LOOP_TOP);
		}
		break;

		case J_LOOP_EXIT:     // 10
		{
			HtContinue(DONE);
		}
		break;

		case DONE:     // 2
		{
			if (SendReturnBusy_stencil()) {
				HtRetry();
				break;
			}
#if 0
			fprintf(stderr, "not busy -- doing return from thread %d  change is %g\n",
				(int)(P_htId),
				((double)P_change) / ((double)(ONE)));
#endif
			SendReturn_stencil(P_change);
			//            HtPause();
		}
		break;
		default:
#if 0
			fprintf(stderr, "Stencil did not handle state %d\n", (int)r_ts_htInst);
#endif
			break;
		}
	} // r_ts_htValid
}
