#include "Ht.h"
using namespace Ht;

#ifdef HT_VSIM
# define CALLS 3
# define WORDS 100
# define SEED -1
#else
#ifdef HT_SYSC
# define CALLS 10
# define WORDS 1000
# define SEED -1
#else
# define CALLS 16
# define WORDS (128 * 1024)
# define SEED -1
#endif
#endif


int main(int argc, char **argv)
{
	CHtHif *pHtHif = new CHtHif();
	int nau = pHtHif->GetUnitCnt();

	CHtAuUnit ** pAuUnits = new CHtAuUnit * [nau];
	for (int unit = 0; unit < nau; unit++)
		pAuUnits[unit] = new CHtAuUnit(pHtHif);

	time_t time_now = time(NULL);

	printf("start time = %s", ctime(&time_now));

	struct timeval st;
	gettimeofday(&st, NULL);
	unsigned int seed = (SEED != -1) ? SEED : (int)st.tv_usec;
	printf("seed = 0x%x\n", seed);
	srand(seed);


	int active_cnt = 0;
	int err_cnt = 0;
	int tot_calls = 0, tot_rtns = 0;
	int *call_num = (int *)calloc(nau, sizeof(int));
	int *call_cnt = (int *)calloc(nau, sizeof(int));
	int *rcv_cnt = (int *)calloc(nau, sizeof(int));
	int *active = (int *)calloc(nau, sizeof(int));
	long long exp_pop = 0, tot_pop = 0;

	for (int au = 0; au < nau; au++) {
		call_num[au] = (rand() % (CALLS - 1)) + 1;
		tot_calls += call_num[au];
		active_cnt += 1;
	}
	printf("total calls = %d\n", tot_calls);
	fflush(stdout);

	while (active_cnt) {
		for (int au = 0; au < nau; au++) {
			if (!call_num[au])
				continue;

			if (!active[au]) {
				int cnt = rand() % WORDS;

				if (pAuUnits[au]->SendCall_htmain(au, cnt)) {
					active[au] = 1;
					call_cnt[au] = rcv_cnt[au] = cnt;
					exp_pop += call_cnt[au];
				}
			}

			if (pAuUnits[au]->RecvReturn_htmain()) {
				active[au] = 0;
				call_num[au] -= 1;
				if (!call_num[au]) active_cnt -= 1;
				tot_rtns += 1;

				if (!(tot_rtns % 100)) {
					printf("processed %d calls\n", tot_rtns);
					fflush(stdout);
				}

				if (rcv_cnt[au] != 0) {
					err_cnt += 1;
					printf("ERROR: %d missing %d words\n",
					       au, rcv_cnt[au]);
					fflush(stdout);
				}
				continue;
			}

			uint64_t d;
			if (pAuUnits[au]->RecvHostData(1, &d) != 1)
				continue;

			tot_pop += 1;

			uint64_t expect;
			expect = (uint64_t)rcv_cnt[au];
			expect |= (uint64_t)au << 56;

			rcv_cnt[au] -= 1;

			if (d != expect) {
				err_cnt += 1;
				printf("ERROR: %d expected 0x%016llx != 0x%016llx\n",
				       au, (long long)expect, (long long)d);
				fflush(stdout);
			}
		}
	}

	printf("exp_pop = %lld, tot_pop = %lld\n", exp_pop, tot_pop);

	time_now = time(NULL);
	printf("end time = %s", ctime(&time_now));
//fprintf(stderr, "Hit enter to continue\n"); getchar();

	for (int au = 0; au < nau; au += 1)
		delete pAuUnits[au];
	delete pAuUnits;
	delete pHtHif;

	if (err_cnt)
		printf("FAILED: detected %d issues!\n", err_cnt);
	else
		printf("PASSED\n");

	return err_cnt;
}
