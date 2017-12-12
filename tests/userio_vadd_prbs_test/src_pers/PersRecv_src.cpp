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
		packet_t inPacket;
		if (RecvUioReady_link(0)) {
			inPacket = RecvUioData_link(0);
		}

		// Rcverate expected data
		bool i_reqValid = RecvUioReady_link(0);
		bool o_prbs_err, o_prbs_vld;
		prbs_rcv(
			 GR_htReset,
			 i_reqValid,
			 inPacket.lower,
			 inPacket.upper,
		 
			 o_prbs_err,
			 o_prbs_vld,
			 prbs_rcv_prm_state0
			 );
		
		// Compare data
		if (o_prbs_vld) {
			if (o_prbs_err) {
				fprintf(stderr, "Error - Link 0 - Packet did not match expected data\n");
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
		packet_t inPacket;
		if (RecvUioReady_link(1)) {
			inPacket = RecvUioData_link(1);
		}

		// Rcverate expected data
		bool i_reqValid = RecvUioReady_link(1);
		bool o_prbs_err, o_prbs_vld;
		prbs_rcv(
			 GR_htReset,
			 i_reqValid,
			 inPacket.lower,
			 inPacket.upper,
		 
			 o_prbs_err,
			 o_prbs_vld,
			 prbs_rcv_prm_state1
			 );
		
		// Compare data
		if (o_prbs_vld) {
			if (o_prbs_err) {
				fprintf(stderr, "Error - Link 1 - Packet did not match expected data\n");
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
		packet_t inPacket;
		if (RecvUioReady_link(2)) {
			inPacket = RecvUioData_link(2);
		}

		// Rcverate expected data
		bool i_reqValid = RecvUioReady_link(2);
		bool o_prbs_err, o_prbs_vld;
		prbs_rcv(
			 GR_htReset,
			 i_reqValid,
			 inPacket.lower,
			 inPacket.upper,
		 
			 o_prbs_err,
			 o_prbs_vld,
			 prbs_rcv_prm_state2
			 );
		
		// Compare data
		if (o_prbs_vld) {
			if (o_prbs_err) {
				fprintf(stderr, "Error - Link 2 - Packet did not match expected data\n");
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
		packet_t inPacket;
		if (RecvUioReady_link(3)) {
			inPacket = RecvUioData_link(3);
		}

		// Rcverate expected data
		bool i_reqValid = RecvUioReady_link(3);
		bool o_prbs_err, o_prbs_vld;
		prbs_rcv(
			 GR_htReset,
			 i_reqValid,
			 inPacket.lower,
			 inPacket.upper,
		 
			 o_prbs_err,
			 o_prbs_vld,
			 prbs_rcv_prm_state3
			 );
		
		// Compare data
		if (o_prbs_vld) {
			if (o_prbs_err) {
				fprintf(stderr, "Error - Link 3 - Packet did not match expected data\n");
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
		packet_t inPacket;
		if (RecvUioReady_link(4)) {
			inPacket = RecvUioData_link(4);
		}

		// Rcverate expected data
		bool i_reqValid = RecvUioReady_link(4);
		bool o_prbs_err, o_prbs_vld;
		prbs_rcv(
			 GR_htReset,
			 i_reqValid,
			 inPacket.lower,
			 inPacket.upper,
		 
			 o_prbs_err,
			 o_prbs_vld,
			 prbs_rcv_prm_state4
			 );
		
		// Compare data
		if (o_prbs_vld) {
			if (o_prbs_err) {
				fprintf(stderr, "Error - Link 4 - Packet did not match expected data\n");
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
		packet_t inPacket;
		if (RecvUioReady_link(5)) {
			inPacket = RecvUioData_link(5);
		}

		// Rcverate expected data
		bool i_reqValid = RecvUioReady_link(5);
		bool o_prbs_err, o_prbs_vld;
		prbs_rcv(
			 GR_htReset,
			 i_reqValid,
			 inPacket.lower,
			 inPacket.upper,
		 
			 o_prbs_err,
			 o_prbs_vld,
			 prbs_rcv_prm_state5
			 );
		
		// Compare data
		if (o_prbs_vld) {
			if (o_prbs_err) {
				fprintf(stderr, "Error - Link 5 - Packet did not match expected data\n");
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
		packet_t inPacket;
		if (RecvUioReady_link(6)) {
			inPacket = RecvUioData_link(6);
		}

		// Rcverate expected data
		bool i_reqValid = RecvUioReady_link(6);
		bool o_prbs_err, o_prbs_vld;
		prbs_rcv(
			 GR_htReset,
			 i_reqValid,
			 inPacket.lower,
			 inPacket.upper,
		 
			 o_prbs_err,
			 o_prbs_vld,
			 prbs_rcv_prm_state6
			 );
		
		// Compare data
		if (o_prbs_vld) {
			if (o_prbs_err) {
				fprintf(stderr, "Error - Link 6 - Packet did not match expected data\n");
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
		packet_t inPacket;
		if (RecvUioReady_link(7)) {
			inPacket = RecvUioData_link(7);
		}

		// Rcverate expected data
		bool i_reqValid = RecvUioReady_link(7);
		bool o_prbs_err, o_prbs_vld;
		prbs_rcv(
			 GR_htReset,
			 i_reqValid,
			 inPacket.lower,
			 inPacket.upper,
		 
			 o_prbs_err,
			 o_prbs_vld,
			 prbs_rcv_prm_state7
			 );
		
		// Compare data
		if (o_prbs_vld) {
			if (o_prbs_err) {
				fprintf(stderr, "Error - Link 7 - Packet did not match expected data\n");
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
