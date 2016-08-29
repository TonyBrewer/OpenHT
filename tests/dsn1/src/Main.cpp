#include "Ht.h"
using namespace Ht;

#include <stdexcept>

#define CNT 16
uint64_t arr[CNT * 2];

bool sysHas1GBPages();
std::string execCmd(const char* cmd);

int main(int argc, char **argv)
{
	for (int i = 0; i < CNT; i++) {
		arr[i * 2] = i;
		arr[i * 2 + 1] = 0xdeadbeefdeadbeefLL;
	}

	CHtHifParams htHifParams;
#ifdef WIN32
	// This will be caught in HT source and not actually use Huge Pages in Windows...
	htHifParams.m_bHtHifHugePage = true;
#else
	if (sysHas1GBPages()) {
		htHifParams.m_bHtHifHugePage = true;
	} else {
		printf("\nWARNING: System doesn't appear to have 1GB pages...bypassing HT Huge Page flag!\n\n");
	}
#endif

	CHtHif *pHtHif;
	try {
		pHtHif = new CHtHif(&htHifParams);
	}
	catch (CHtException &htException) {
		printf("new CHtHif threw an exception: '%s'\n", htException.GetMsg().c_str());
		exit(1);
	}

	CHtSuUnit *pUnit = new CHtSuUnit(pHtHif);

	printf("#AUs = %d\n", pHtHif->GetUnitCnt());
	fflush(stdout);

	pHtHif->SendAllHostMsg(SU_ARRAY_ADDR, (uint64_t)&arr);
	//pUnit->SendHostMsg(SU_ARRAY_ADDR, (uint64_t)&arr);

	pUnit->SendCall_htmain(CNT);

	// wait for return
	uint32_t rtn_elemCnt;
	while (!pUnit->RecvReturn_htmain(rtn_elemCnt))
		usleep(1);

	delete pUnit;
	delete pHtHif;

	printf("RTN: elemCnt = %d\n", rtn_elemCnt);

	// check results
	int err_cnt = 0;
	for (unsigned i = 0; i < CNT; i++) {
		if (arr[i * 2 + 1] != i + 1) {
			printf("arr[%d] is %lld, should be %d\n",
			       i, (long long)arr[i * 2 + 1], i + 1);
			err_cnt++;
		}
	}

	if (err_cnt)
		printf("FAILED: detected %d issues!\n", err_cnt);
	else
		printf("PASSED\n");

	return err_cnt;
}

bool sysHas1GBPages() {
	std::string grepRslt = execCmd("grep \"Hugepagesize:\" /proc/meminfo");
	if (grepRslt.find("1048576 kB") != std::string::npos) {
		return true;
	} else {
		return false;
	}
}

std::string execCmd(const char* cmd) {
	char buffer[128];
	std::string result = "";
	FILE* pipe = popen(cmd, "r");
	if (!pipe) throw std::runtime_error("popen() failed!");
	try {
		while (!feof(pipe)) {
			if (fgets(buffer, 128, pipe) != NULL)
				result += buffer;
		}
	} catch (...) {
		pclose(pipe);
		throw;
	}
	pclose(pipe);
	return result;
}
