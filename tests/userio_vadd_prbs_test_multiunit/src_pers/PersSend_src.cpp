#include "Ht.h"
#include "PersSend.h"

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

void
CPersSend::PersSend()
{
	if (RecvMsgReady_initMsg()) {
		// Start message received
		RecvMsg_initMsg();
		S_init_seen = true;
		S_rst_prbs = true;

		// Initialize vars and prepare to send ack
		S_recv_seen = false;
		S_run = false;
		for (int i = 0; i < 4; i++) {
			S_count[i] = 0;
			S_done[i] = false;
			S_holdPacketVld[i] = false;
		}
	}

	if (SR_rst_prbs) {
		S_rst_prbs = false;
	}

	if (RecvMsgReady_recvRdy()) {
		// Recv message received
		RecvMsg_recvRdy();
		S_recv_seen = true;
	}

	if (SR_init_seen && SR_recv_seen && SR_len != 0 && !SR_run) {
		S_run = true;
	}


	if (PR_htValid) {
		switch (PR_htInst) {
		case SEND_ENTRY: {
			BUSY_RETRY(SR_init_seen == false);

			HtContinue(SEND_RUN);
		}
		break;
		case SEND_RUN: {
			bool done = (
				     SR_done[0] &
				     SR_done[1] &
				     SR_done[2] &
				     SR_done[3]
				     );
			if (done) {
				S_init_seen = false;
				S_recv_seen = false;
				S_run = false;
				HtContinue(SEND_RTN);
			} else {
				HtContinue(SEND_RUN);
			}
		}
		break;
		case SEND_RTN: {
			BUSY_RETRY(SendReturnBusy_send());

			SendReturn_send();
		}
		break;
		default:
			assert(0);
		}
	}

	bool prbs_rst = GR_htReset | SR_rst_prbs;

	// Link 0
	{
		// Generate data
		bool i_reqValid = !SendUioBusy_link(0) && !SR_done[0] && SR_run && SR_prbsRdy[0];
		bool o_prbs_rdy, o_prbs_vld;
		ht_uint64 o_prbs_lower, o_prbs_upper;
		prbs_gen(
			 prbs_rst,
			 i_reqValid,
		 
			 o_prbs_rdy,
			 o_prbs_vld,
			 o_prbs_lower,
			 o_prbs_upper,
			 prbs_gen_prm_state0
			 );

		packet_t outPacket;
		outPacket.data.lower = o_prbs_lower;
		outPacket.data.upper = o_prbs_upper;

		S_prbsRdy[0] = o_prbs_rdy;

		// If a valid request is coming out
		if (o_prbs_vld && !SR_done[0]) {

			// Hold data if link is full
			if (SendUioBusy_link(0)) {
				S_holdPacket[0] = outPacket;
				S_holdPacketVld[0] = true;
			}

			// Otherwise, just send data out
			else {
				SendUioData_link(0, outPacket);
				S_holdPacketVld[0] = false;
				S_count[0] = SR_count[0] + 1;
			}
		}

		// If a packet is held and the link opens, drain the hold
		if (SR_holdPacketVld[0] && !SendUioBusy_link(0) && !SR_done[0]) {
			SendUioData_link(0, SR_holdPacket[0]);
			S_holdPacketVld[0] = false;
			S_count[0] = SR_count[0] + 1;
		}

		if (S_count[0] == SR_len && SR_run) {
			S_done[0] = true;
		}
	}

	// Link 1
	{
		// Generate data
		bool i_reqValid = !SendUioBusy_link(1) && !SR_done[1] && SR_run && SR_prbsRdy[1];
		bool o_prbs_rdy, o_prbs_vld;
		ht_uint64 o_prbs_lower, o_prbs_upper;
		prbs_gen(
			 prbs_rst,
			 i_reqValid,
		 
			 o_prbs_rdy,
			 o_prbs_vld,
			 o_prbs_lower,
			 o_prbs_upper,
			 prbs_gen_prm_state1
			 );

		packet_t outPacket;
		outPacket.data.lower = o_prbs_lower;
		outPacket.data.upper = o_prbs_upper;

		S_prbsRdy[1] = o_prbs_rdy;

		// If a valid request is coming out
		if (o_prbs_vld && !SR_done[1]) {

			// Hold data if link is full
			if (SendUioBusy_link(1)) {
				S_holdPacket[1] = outPacket;
				S_holdPacketVld[1] = true;
			}

			// Otherwise, just send data out
			else {
				SendUioData_link(1, outPacket);
				S_holdPacketVld[1] = false;
				S_count[1] = SR_count[1] + 1;
			}
		}

		// If a packet is held and the link opens, drain the hold
		if (SR_holdPacketVld[1] && !SendUioBusy_link(1) && !SR_done[1]) {
			SendUioData_link(1, SR_holdPacket[1]);
			S_holdPacketVld[1] = false;
			S_count[1] = SR_count[1] + 1;
		}

		if (S_count[1] == SR_len && SR_run) {
			S_done[1] = true;
		}
	}

	// Link 2
	{
		// Generate data
		bool i_reqValid = !SendUioBusy_link(2) && !SR_done[2] && SR_run && SR_prbsRdy[2];
		bool o_prbs_rdy, o_prbs_vld;
		ht_uint64 o_prbs_lower, o_prbs_upper;
		prbs_gen(
			 prbs_rst,
			 i_reqValid,
		 
			 o_prbs_rdy,
			 o_prbs_vld,
			 o_prbs_lower,
			 o_prbs_upper,
			 prbs_gen_prm_state2
			 );

		packet_t outPacket;
		outPacket.data.lower = o_prbs_lower;
		outPacket.data.upper = o_prbs_upper;

		S_prbsRdy[2] = o_prbs_rdy;

		// If a valid request is coming out
		if (o_prbs_vld && !SR_done[2]) {

			// Hold data if link is full
			if (SendUioBusy_link(2)) {
				S_holdPacket[2] = outPacket;
				S_holdPacketVld[2] = true;
			}

			// Otherwise, just send data out
			else {
				SendUioData_link(2, outPacket);
				S_holdPacketVld[2] = false;
				S_count[2] = SR_count[2] + 1;
			}
		}

		// If a packet is held and the link opens, drain the hold
		if (SR_holdPacketVld[2] && !SendUioBusy_link(2) && !SR_done[2]) {
			SendUioData_link(2, SR_holdPacket[2]);
			S_holdPacketVld[2] = false;
			S_count[2] = SR_count[2] + 1;
		}

		if (S_count[2] == SR_len && SR_run) {
			S_done[2] = true;
		}
	}

	// Link 3
	{
		// Generate data
		bool i_reqValid = !SendUioBusy_link(3) && !SR_done[3] && SR_run && SR_prbsRdy[3];
		bool o_prbs_rdy, o_prbs_vld;
		ht_uint64 o_prbs_lower, o_prbs_upper;
		prbs_gen(
			 prbs_rst,
			 i_reqValid,
		 
			 o_prbs_rdy,
			 o_prbs_vld,
			 o_prbs_lower,
			 o_prbs_upper,
			 prbs_gen_prm_state3
			 );

		packet_t outPacket;
		outPacket.data.lower = o_prbs_lower;
		outPacket.data.upper = o_prbs_upper;

		S_prbsRdy[3] = o_prbs_rdy;

		// If a valid request is coming out
		if (o_prbs_vld && !SR_done[3]) {

			// Hold data if link is full
			if (SendUioBusy_link(3)) {
				S_holdPacket[3] = outPacket;
				S_holdPacketVld[3] = true;
			}

			// Otherwise, just send data out
			else {
				SendUioData_link(3, outPacket);
				S_holdPacketVld[3] = false;
				S_count[3] = SR_count[3] + 1;
			}
		}

		// If a packet is held and the link opens, drain the hold
		if (SR_holdPacketVld[3] && !SendUioBusy_link(3) && !SR_done[3]) {
			SendUioData_link(3, SR_holdPacket[3]);
			S_holdPacketVld[3] = false;
			S_count[3] = SR_count[3] + 1;
		}

		if (S_count[3] == SR_len && SR_run) {
			S_done[3] = true;
		}
	}
}
