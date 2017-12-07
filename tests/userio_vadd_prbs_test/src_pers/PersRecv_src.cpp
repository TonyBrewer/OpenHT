#include "Ht.h"
#include "PersRecv.h"

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

void
CPersRecv::PersRecv()
{
	if (PR2_htValid) {
		switch (PR2_htInst) {
		case RECV_WAIT: {
			bool done = (
				     SR_done[0] &
				     SR_done[1] &
				     SR_done[2] &
				     SR_done[3] &
				     SR_done[4] &
				     SR_done[5] &
				     SR_done[6] &
				     SR_done[7]
				     );
			if (done) {
				HtContinue(RECV_RTN);
			} else {
				HtContinue(RECV_WAIT);
			}
		}
		break;
		case RECV_RTN: {
			BUSY_RETRY(SendReturnBusy_recv());

			SendReturn_recv(SR_error[0],
					SR_error[1],
					SR_error[2],
					SR_error[3],
					SR_error[4],
					SR_error[5],
					SR_error[6],
					SR_error[7]);
		}
		break;
		default:
			assert(0);
		}
	}

	// Link 0
	{
		// Get Data
		if (RecvUioReady_link(0)) {
			T1_inPacket[0] = RecvUioData_link(0);
		}

		// Generate expected data
		bool i_reqValid = RecvUioReady_link(0);
		bool o_prbs_vld;
		ht_uint64 o_prbs_lower, o_prbs_upper;
		prbs_gen(
			 GR_htReset,
			 i_reqValid,
		 
			 o_prbs_vld,
			 o_prbs_lower,
			 o_prbs_upper,
			 prbs_prm_state0
			 );

		packet_t outPacket;
		outPacket.lower = o_prbs_lower;
		outPacket.upper = o_prbs_upper;
		
		packet_t inPacket = TR2_inPacket[0];
		
		// Compare data
		if (o_prbs_vld) {
			if (inPacket.upper != outPacket.upper || inPacket.lower != outPacket.lower) {
				fprintf(stderr, "Error - Link 0 - Packet did not match expected data\n");
				fprintf(stderr, "  Act: 0x%016lX%016lX\n", inPacket.upper, inPacket.lower);
				fprintf(stderr, "  Exp: 0x%016lX%016lX\n", outPacket.upper, outPacket.lower);
				HtAssert(0, (uint32_t)SR_count[0]);
				S_error[0] = SR_error[0] + 1;
			}
			S_count[0] = SR_count[0] + 1;
		}

		if (S_count[0] == SR_len) {
			S_done[0] = true;
		}
	}

	// Link 1
	{
		// Get Data
		if (RecvUioReady_link(1)) {
			T1_inPacket[1] = RecvUioData_link(1);
		}

		// Generate expected data
		bool i_reqValid = RecvUioReady_link(1);
		bool o_prbs_vld;
		ht_uint64 o_prbs_lower, o_prbs_upper;
		prbs_gen(
			 GR_htReset,
			 i_reqValid,
		 
			 o_prbs_vld,
			 o_prbs_lower,
			 o_prbs_upper,
			 prbs_prm_state1
			 );

		packet_t outPacket;
		outPacket.lower = o_prbs_lower;
		outPacket.upper = o_prbs_upper;
		
		packet_t inPacket = TR2_inPacket[1];
		
		// Compare data
		if (o_prbs_vld) {
			if (inPacket.upper != outPacket.upper || inPacket.lower != outPacket.lower) {
				fprintf(stderr, "Error - Link 1 - Packet did not match expected data\n");
				fprintf(stderr, "  Act: 0x%016lX%016lX\n", inPacket.upper, inPacket.lower);
				fprintf(stderr, "  Exp: 0x%016lX%016lX\n", outPacket.upper, outPacket.lower);
				HtAssert(0, (uint32_t)SR_count[1]);
				S_error[1] = SR_error[1] + 1;
			}
			S_count[1] = SR_count[1] + 1;
		}

		if (S_count[1] == SR_len) {
			S_done[1] = true;
		}
	}

	// Link 2
	{
		// Get Data
		if (RecvUioReady_link(2)) {
			T1_inPacket[2] = RecvUioData_link(2);
		}

		// Generate expected data
		bool i_reqValid = RecvUioReady_link(2);
		bool o_prbs_vld;
		ht_uint64 o_prbs_lower, o_prbs_upper;
		prbs_gen(
			 GR_htReset,
			 i_reqValid,
		 
			 o_prbs_vld,
			 o_prbs_lower,
			 o_prbs_upper,
			 prbs_prm_state2
			 );

		packet_t outPacket;
		outPacket.lower = o_prbs_lower;
		outPacket.upper = o_prbs_upper;
		
		packet_t inPacket = TR2_inPacket[2];
		
		// Compare data
		if (o_prbs_vld) {
			if (inPacket.upper != outPacket.upper || inPacket.lower != outPacket.lower) {
				fprintf(stderr, "Error - Link 2 - Packet did not match expected data\n");
				fprintf(stderr, "  Act: 0x%016lX%016lX\n", inPacket.upper, inPacket.lower);
				fprintf(stderr, "  Exp: 0x%016lX%016lX\n", outPacket.upper, outPacket.lower);
				HtAssert(0, (uint32_t)SR_count[2]);
				S_error[2] = SR_error[2] + 1;
			}
			S_count[2] = SR_count[2] + 1;
		}

		if (S_count[2] == SR_len) {
			S_done[2] = true;
		}
	}

	// Link 3
	{
		// Get Data
		if (RecvUioReady_link(3)) {
			T1_inPacket[3] = RecvUioData_link(3);
		}

		// Generate expected data
		bool i_reqValid = RecvUioReady_link(3);
		bool o_prbs_vld;
		ht_uint64 o_prbs_lower, o_prbs_upper;
		prbs_gen(
			 GR_htReset,
			 i_reqValid,
		 
			 o_prbs_vld,
			 o_prbs_lower,
			 o_prbs_upper,
			 prbs_prm_state3
			 );

		packet_t outPacket;
		outPacket.lower = o_prbs_lower;
		outPacket.upper = o_prbs_upper;
		
		packet_t inPacket = TR2_inPacket[3];
		
		// Compare data
		if (o_prbs_vld) {
			if (inPacket.upper != outPacket.upper || inPacket.lower != outPacket.lower) {
				fprintf(stderr, "Error - Link 3 - Packet did not match expected data\n");
				fprintf(stderr, "  Act: 0x%016lX%016lX\n", inPacket.upper, inPacket.lower);
				fprintf(stderr, "  Exp: 0x%016lX%016lX\n", outPacket.upper, outPacket.lower);
				HtAssert(0, (uint32_t)SR_count[3]);
				S_error[3] = SR_error[3] + 1;
			}
			S_count[3] = SR_count[3] + 1;
		}

		if (S_count[3] == SR_len) {
			S_done[3] = true;
		}
	}

	// Link 4
	{
		// Get Data
		if (RecvUioReady_link(4)) {
			T1_inPacket[4] = RecvUioData_link(4);
		}

		// Generate expected data
		bool i_reqValid = RecvUioReady_link(4);
		bool o_prbs_vld;
		ht_uint64 o_prbs_lower, o_prbs_upper;
		prbs_gen(
			 GR_htReset,
			 i_reqValid,
		 
			 o_prbs_vld,
			 o_prbs_lower,
			 o_prbs_upper,
			 prbs_prm_state4
			 );

		packet_t outPacket;
		outPacket.lower = o_prbs_lower;
		outPacket.upper = o_prbs_upper;
		
		packet_t inPacket = TR2_inPacket[4];
		
		// Compare data
		if (o_prbs_vld) {
			if (inPacket.upper != outPacket.upper || inPacket.lower != outPacket.lower) {
				fprintf(stderr, "Error - Link 4 - Packet did not match expected data\n");
				fprintf(stderr, "  Act: 0x%016lX%016lX\n", inPacket.upper, inPacket.lower);
				fprintf(stderr, "  Exp: 0x%016lX%016lX\n", outPacket.upper, outPacket.lower);
				HtAssert(0, (uint32_t)SR_count[4]);
				S_error[4] = SR_error[4] + 1;
			}
			S_count[4] = SR_count[4] + 1;
		}

		if (S_count[4] == SR_len) {
			S_done[4] = true;
		}
	}

	// Link 5
	{
		// Get Data
		if (RecvUioReady_link(5)) {
			T1_inPacket[5] = RecvUioData_link(5);
		}

		// Generate expected data
		bool i_reqValid = RecvUioReady_link(5);
		bool o_prbs_vld;
		ht_uint64 o_prbs_lower, o_prbs_upper;
		prbs_gen(
			 GR_htReset,
			 i_reqValid,
		 
			 o_prbs_vld,
			 o_prbs_lower,
			 o_prbs_upper,
			 prbs_prm_state5
			 );

		packet_t outPacket;
		outPacket.lower = o_prbs_lower;
		outPacket.upper = o_prbs_upper;
		
		packet_t inPacket = TR2_inPacket[5];
		
		// Compare data
		if (o_prbs_vld) {
			if (inPacket.upper != outPacket.upper || inPacket.lower != outPacket.lower) {
				fprintf(stderr, "Error - Link 5 - Packet did not match expected data\n");
				fprintf(stderr, "  Act: 0x%016lX%016lX\n", inPacket.upper, inPacket.lower);
				fprintf(stderr, "  Exp: 0x%016lX%016lX\n", outPacket.upper, outPacket.lower);
				HtAssert(0, (uint32_t)SR_count[5]);
				S_error[5] = SR_error[5] + 1;
			}
			S_count[5] = SR_count[5] + 1;
		}

		if (S_count[5] == SR_len) {
			S_done[5] = true;
		}
	}

	// Link 6
	{
		// Get Data
		if (RecvUioReady_link(6)) {
			T1_inPacket[6] = RecvUioData_link(6);
		}

		// Generate expected data
		bool i_reqValid = RecvUioReady_link(6);
		bool o_prbs_vld;
		ht_uint64 o_prbs_lower, o_prbs_upper;
		prbs_gen(
			 GR_htReset,
			 i_reqValid,
		 
			 o_prbs_vld,
			 o_prbs_lower,
			 o_prbs_upper,
			 prbs_prm_state6
			 );

		packet_t outPacket;
		outPacket.lower = o_prbs_lower;
		outPacket.upper = o_prbs_upper;
		
		packet_t inPacket = TR2_inPacket[6];
		
		// Compare data
		if (o_prbs_vld) {
			if (inPacket.upper != outPacket.upper || inPacket.lower != outPacket.lower) {
				fprintf(stderr, "Error - Link 6 - Packet did not match expected data\n");
				fprintf(stderr, "  Act: 0x%016lX%016lX\n", inPacket.upper, inPacket.lower);
				fprintf(stderr, "  Exp: 0x%016lX%016lX\n", outPacket.upper, outPacket.lower);
				HtAssert(0, (uint32_t)SR_count[6]);
				S_error[6] = SR_error[6] + 1;
			}
			S_count[6] = SR_count[6] + 1;
		}

		if (S_count[6] == SR_len) {
			S_done[6] = true;
		}
	}

	// Link 7
	{
		// Get Data
		if (RecvUioReady_link(7)) {
			T1_inPacket[7] = RecvUioData_link(7);
		}

		// Generate expected data
		bool i_reqValid = RecvUioReady_link(7);
		bool o_prbs_vld;
		ht_uint64 o_prbs_lower, o_prbs_upper;
		prbs_gen(
			 GR_htReset,
			 i_reqValid,
		 
			 o_prbs_vld,
			 o_prbs_lower,
			 o_prbs_upper,
			 prbs_prm_state7
			 );

		packet_t outPacket;
		outPacket.lower = o_prbs_lower;
		outPacket.upper = o_prbs_upper;
		
		packet_t inPacket = TR2_inPacket[7];

		// Compare data
		if (o_prbs_vld) {
			if (inPacket.upper != outPacket.upper || inPacket.lower != outPacket.lower) {
				fprintf(stderr, "Error - Link 7 - Packet did not match expected data\n");
				fprintf(stderr, "  Act: 0x%016lX%016lX\n", inPacket.upper, inPacket.lower);
				fprintf(stderr, "  Exp: 0x%016lX%016lX\n", outPacket.upper, outPacket.lower);
				HtAssert(0, (uint32_t)SR_count[7]);
				S_error[7] = SR_error[7] + 1;
			}
			S_count[7] = SR_count[7] + 1;
		}

		if (S_count[7] == SR_len) {
			S_done[7] = true;
		}
	}
}
