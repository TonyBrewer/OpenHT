#include "Ht.h"
using namespace Ht;

bool done = false;
int err_cnt = 0;

class CHtSuUnitEx : public CHtSuUnit {
public:
	CHtSuUnitEx(CHtHif * pHtHif) : CHtSuUnit(pHtHif) {}
	void RecvCallback( Ht::ERecvType );
};

int main(int argc, char **argv)
{
	CHtHif *pHtHif = new CHtHif;
	CHtSuUnitEx *pHtUnit = new CHtSuUnitEx(pHtHif);

	pHtUnit->SetUsingCallback(true);

	uint64_t callTrail = 0xf;
	pHtUnit->SendCall_htmain(callTrail);

	// wait for return
	while (!done) {
		if (!pHtUnit->RecvPoll())
			// sleep if no message was ready
			usleep(1);
	}

	delete pHtUnit;
	delete pHtHif;

	if (err_cnt)
		printf("FAILED: detected %d issues!\n", err_cnt);
	else
		printf("PASSED\n");

	return err_cnt;
}

void CHtSuUnitEx::RecvCallback(Ht::ERecvType recvType)
{
	switch (recvType) {
	case Ht::eRecvHostMsg:
	{
		uint8_t msgType;
		uint64_t msgData;

		RecvHostMsg(msgType, msgData);

		printf("Recieved Msg(%d, 0x%016llx)\n", msgType, (long long)msgData);
	}
	break;
	case Ht::eRecvReturn:
	{
		uint64_t rtnTrail = 0;
		RecvReturn_htmain(rtnTrail);

		printf("Callback() - Return\n");

		if (rtnTrail != 0x00000f8193ac3d7eLL) {
			printf("expected one argument with value 0x00000f8193ac3d7e\n");
			err_cnt++;
		}

		done = true;
	}
	break;
	default:
		assert(0);
	}
}
