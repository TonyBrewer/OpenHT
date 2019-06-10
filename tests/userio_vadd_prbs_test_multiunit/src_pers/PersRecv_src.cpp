#include "Ht.h"
#include "PersRecv.h"

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

void
CPersRecv::PersRecv()
{
	if (RecvMsgReady_initMsg()) {
		// Start message received
		RecvMsg_initMsg();
		S_init_seen = true;
		S_rst_prbs = true;

		// Initialize vars and prepare to send ack
		for (int i = 0; i < 4; i++) {
			S_count[i] = 0;
			S_error[i] = 0;
			S_done[i] = false;
		}
	}

	if (SR_rst_prbs) {
		S_rst_prbs = false;
	}


	if (PR_htValid) {
		switch (PR_htInst) {
		case RECV_ENTRY: {
			BUSY_RETRY(SR_init_seen == false);
			BUSY_RETRY(SendMsgBusy_recvRdy());

			SendMsg_recvRdy(true);
			HtContinue(RECV_RUN);
		}
		break;
		case RECV_RUN: {
			bool done = (
				     SR_done[0] &
				     SR_done[1] &
				     SR_done[2] &
				     SR_done[3]
				     );
			if (done) {
				S_init_seen = false;
				HtContinue(RECV_RTN);
			} else {
				HtContinue(RECV_RUN);
			}
		}
		break;
		case RECV_RTN: {
			BUSY_RETRY(SendReturnBusy_recv());

			SendReturn_recv(SR_error[0],
					SR_error[1],
					SR_error[2],
					SR_error[3]);
		}
		break;
		default:
			assert(0);
		}
	}

	bool prbs_rst = GR_htReset | SR_rst_prbs;

	// Link 0
	{
		// Get Data
		packet_t inPacket;
		if (RecvUioReady_link(0)) {
			inPacket = RecvUioData_link(0);
		}

		// Generate expected data
		bool i_reqValid = RecvUioReady_link(0);
		bool o_prbs_err, o_prbs_vld;
		prbs_rcv(
			 prbs_rst,
			 i_reqValid,
			 inPacket.data.lower,
			 inPacket.data.upper,
		 
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

		// Generate expected data
		bool i_reqValid = RecvUioReady_link(1);
		bool o_prbs_err, o_prbs_vld;
		prbs_rcv(
			 prbs_rst,
			 i_reqValid,
			 inPacket.data.lower,
			 inPacket.data.upper,
		 
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

		// Generate expected data
		bool i_reqValid = RecvUioReady_link(2);
		bool o_prbs_err, o_prbs_vld;
		prbs_rcv(
			 prbs_rst,
			 i_reqValid,
			 inPacket.data.lower,
			 inPacket.data.upper,
		 
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

		// Generate expected data
		bool i_reqValid = RecvUioReady_link(3);
		bool o_prbs_err, o_prbs_vld;
		prbs_rcv(
			 prbs_rst,
			 i_reqValid,
			 inPacket.data.lower,
			 inPacket.data.upper,
		 
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
}
