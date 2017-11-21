#include "Ht.h"
#include "PersSend.h"

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

void
CPersSend::PersSend()
{
	if (RecvMsgReady_startMsg()) {
		RecvMsg_startMsg();

		S_start_sig = true;
	}

	if (SR_len != 0) {
		S_start_len = true;
	}

	if (SR_start_sig && SR_start_len) {
		S_start = true;
	}


	if (PR_htValid) {
		switch (PR_htInst) {
		case SEND_WAIT: {
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
				HtContinue(SEND_RTN);
			} else {
				HtContinue(SEND_WAIT);
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

	// Link 0
	{
		// Generate data
		bool i_reqValid = !SendUioBusy_link(0) && !SR_done[0] && SR_start;
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
				S_count[0] = SR_count[0] + 1;
			}
		}

		// If a packet is held and the link opens, drain the hold
		if (SR_holdPacketVld[0] && !SendUioBusy_link(0) && !SR_done[0]) {
			SendUioData_link(0, SR_holdPacket[0]);
			S_count[0] = SR_count[0] + 1;
		}

		if (S_count[0] == SR_len && SR_start) {
			S_done[0] = true;
		}
	}

	// Link 1
	{
		// Generate data
		bool i_reqValid = !SendUioBusy_link(1) && !SR_done[1] && SR_start;
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
				S_count[1] = SR_count[1] + 1;
			}
		}

		// If a packet is held and the link opens, drain the hold
		if (SR_holdPacketVld[1] && !SendUioBusy_link(1) && !SR_done[1]) {
			SendUioData_link(1, SR_holdPacket[1]);
			S_count[1] = SR_count[1] + 1;
		}

		if (S_count[1] == SR_len && SR_start) {
			S_done[1] = true;
		}
	}

	// Link 2
	{
		// Generate data
		bool i_reqValid = !SendUioBusy_link(2) && !SR_done[2] && SR_start;
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
				S_count[2] = SR_count[2] + 1;
			}
		}

		// If a packet is held and the link opens, drain the hold
		if (SR_holdPacketVld[2] && !SendUioBusy_link(2) && !SR_done[2]) {
			SendUioData_link(2, SR_holdPacket[2]);
			S_count[2] = SR_count[2] + 1;
		}

		if (S_count[2] == SR_len && SR_start) {
			S_done[2] = true;
		}
	}

	// Link 3
	{
		// Generate data
		bool i_reqValid = !SendUioBusy_link(3) && !SR_done[3] && SR_start;
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
				S_count[3] = SR_count[3] + 1;
			}
		}

		// If a packet is held and the link opens, drain the hold
		if (SR_holdPacketVld[3] && !SendUioBusy_link(3) && !SR_done[3]) {
			SendUioData_link(3, SR_holdPacket[3]);
			S_count[3] = SR_count[3] + 1;
		}

		if (S_count[3] == SR_len && SR_start) {
			S_done[3] = true;
		}
	}

	// Link 4
	{
		// Generate data
		bool i_reqValid = !SendUioBusy_link(4) && !SR_done[4] && SR_start;
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

		// If a valid request is coming out
		if (o_prbs_vld && !SR_done[4]) {

			// Hold data if link is full
			if (SendUioBusy_link(4)) {
				S_holdPacket[4] = outPacket;
				S_holdPacketVld[4] = true;
			}

			// Otherwise, just send data out
			else {
				SendUioData_link(4, outPacket);
				S_count[4] = SR_count[4] + 1;
			}
		}

		// If a packet is held and the link opens, drain the hold
		if (SR_holdPacketVld[4] && !SendUioBusy_link(4) && !SR_done[4]) {
			SendUioData_link(4, SR_holdPacket[4]);
			S_count[4] = SR_count[4] + 1;
		}

		if (S_count[4] == SR_len && SR_start) {
			S_done[4] = true;
		}
	}

	// Link 5
	{
		// Generate data
		bool i_reqValid = !SendUioBusy_link(5) && !SR_done[5] && SR_start;
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

		// If a valid request is coming out
		if (o_prbs_vld && !SR_done[5]) {

			// Hold data if link is full
			if (SendUioBusy_link(5)) {
				S_holdPacket[5] = outPacket;
				S_holdPacketVld[5] = true;
			}

			// Otherwise, just send data out
			else {
				SendUioData_link(5, outPacket);
				S_count[5] = SR_count[5] + 1;
			}
		}

		// If a packet is held and the link opens, drain the hold
		if (SR_holdPacketVld[5] && !SendUioBusy_link(5) && !SR_done[5]) {
			SendUioData_link(5, SR_holdPacket[5]);
			S_count[5] = SR_count[5] + 1;
		}

		if (S_count[5] == SR_len && SR_start) {
			S_done[5] = true;
		}
	}

	// Link 6
	{
		// Generate data
		bool i_reqValid = !SendUioBusy_link(6) && !SR_done[6] && SR_start;
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

		// If a valid request is coming out
		if (o_prbs_vld && !SR_done[6]) {

			// Hold data if link is full
			if (SendUioBusy_link(6)) {
				S_holdPacket[6] = outPacket;
				S_holdPacketVld[6] = true;
			}

			// Otherwise, just send data out
			else {
				SendUioData_link(6, outPacket);
				S_count[6] = SR_count[6] + 1;
			}
		}

		// If a packet is held and the link opens, drain the hold
		if (SR_holdPacketVld[6] && !SendUioBusy_link(6) && !SR_done[6]) {
			SendUioData_link(6, SR_holdPacket[6]);
			S_count[6] = SR_count[6] + 1;
		}

		if (S_count[6] == SR_len && SR_start) {
			S_done[6] = true;
		}
	}

	// Link 7
	{
		// Generate data
		bool i_reqValid = !SendUioBusy_link(7) && !SR_done[7] && SR_start;
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

		// If a valid request is coming out
		if (o_prbs_vld && !SR_done[7]) {

			// Hold data if link is full
			if (SendUioBusy_link(7)) {
				S_holdPacket[7] = outPacket;
				S_holdPacketVld[7] = true;
			}

			// Otherwise, just send data out
			else {
				SendUioData_link(7, outPacket);
				S_count[7] = SR_count[7] + 1;
			}
		}

		// If a packet is held and the link opens, drain the hold
		if (SR_holdPacketVld[7] && !SendUioBusy_link(7) && !SR_done[7]) {
			SendUioData_link(7, SR_holdPacket[7]);
			S_count[7] = SR_count[7] + 1;
		}

		if (S_count[7] == SR_len && SR_start) {
			S_done[7] = true;
		}
	}
}
