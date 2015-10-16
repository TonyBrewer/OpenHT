#include "Ht.h"
using namespace Ht;

#include "vgups.h"

void CHtModelAuUnit::UnitThread()
{
    uint8_t msgType;
    uint64_t msgData;
    uint64_t tblSize = 0;
    uint64_t *tblBase = NULL;
    uint64_t tblAddr[ThreadCnt];
    uint64_t updCnt[ThreadCnt];
    int thread;

    do {
	if (RecvHostMsg(msgType, msgData)) {
	    switch (msgType) {
		case OP_BASE: tblBase = (uint64_t *)msgData; break;
		case OP_SIZE: tblSize = msgData; break;
		case OP_UPDCNT:
		    for (int thrSel = 0; thrSel < ThreadCnt; thrSel++)
			updCnt[thrSel] = msgData;
		    break;
		case SET_INITSEED0:
		    thread = msgData >> 32;
		    tblAddr[thread] = msgData & ((1LL << 32) - 1);
		    break;
		case SET_INITSEED1:
		    thread = msgData >> 32;
		    tblAddr[thread] |= (msgData & ((1LL << 32) - 1)) << 32;
		    break;
		default: assert(0); break;
	    }
	}
	if (RecvCall_htmain()) {
	    for (int thrSel = 0; thrSel < ThreadCnt; thrSel++)
		while (updCnt[thrSel] > 0) {
		    tblAddr[thrSel] = (tblAddr[thrSel] << 1) ^ ((int64_t) tblAddr[thrSel] < 0 ? POLY : 0);
		    tblBase[tblAddr[thrSel] & (tblSize-1)] ^= tblAddr[thrSel];
		    updCnt[thrSel]--;
		}
	    while (!SendReturn_htmain());
	}
    } while (!RecvHostHalt());
}
