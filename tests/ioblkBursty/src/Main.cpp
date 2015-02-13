#include "Ht.h"
using namespace Ht;

#ifndef SEED
# define SEED -1
#endif

#ifdef HT_VSIM
# define MAX_CALLS 10
# define MAX_WORDS 50
# define MAX_FLUSH 50
#else
#ifdef HT_SYSC
# define MAX_CALLS 100
# define MAX_WORDS 500
# define MAX_FLUSH 200
#else
# define MAX_CALLS 100000
# define MAX_WORDS 50000
# define MAX_FLUSH 2000
#endif
#endif

typedef struct state {
	bool	done;
	bool	done_calls;

	int	num_call;
	int	num_words[MAX_CALLS];

	int	send_call;
	int	send_word;

	int	recv_call;
	int	recv_word;
} state_t;

static const char *tfn = "/scratch/trace.txt";
FILE *tfp; // = stderr;

int main(int argc, char **argv)
{
	if (0 && !tfp && !(tfp = fopen(tfn, "w"))) {
		fprintf(stderr, "Could not open %s for writing\n", tfn);
		exit(-1);
	}

	CHtHifParams params;
	//params.m_iBlkTimerUSec = 0;
	params.m_oBlkTimerUSec = 1;

	CHtHif *pHtHif = new CHtHif(&params);
	int nau = pHtHif->GetUnitCnt();
	CHtAuUnit ** pAuUnits = new CHtAuUnit * [nau];

	for (int unitId = 0; unitId < nau; unitId += 1)
		pAuUnits[unitId] = new CHtAuUnit(pHtHif);

	time_t time_now = time(NULL);

	printf("start time = %s", ctime(&time_now));

	struct timeval st;
	gettimeofday(&st, NULL);
	unsigned int seed = (SEED != -1) ? SEED : (int)st.tv_usec;
	printf("seed = 0x%x\n", seed);
	srand(seed);

	int active_cnt = 0;
	int err_cnt = 0, aerr_cnt = 0;
	int tot_call = 0, tot_rtn = 0;
	long long tot_push = 0, tot_pop = 0;

	state_t *ast = (state_t *)calloc(nau, sizeof(state_t));

	for (int au = 0; au < nau; au++) {
		active_cnt += 1;
		ast[au].done = ast[au].done_calls = false;
		ast[au].num_call = 1;
		if (MAX_CALLS > 1)
			ast[au].num_call = (rand() % (MAX_CALLS - 1)) + 1;
		ast[au].send_word = -1;
		ast[au].recv_word = -1;
	}

	while (active_cnt) {
		for (int au = 0; au < nau; au++) {
			uint64_t d;

			// fineto
			if (ast[au].done)
				continue;

			// Call
			if (!ast[au].done_calls && ast[au].send_word < 0) {
				int cnt = rand() % MAX_WORDS;
				if (pAuUnits[au]->SendCall_htmain(au, ast[au].send_call, cnt)) {
					if (tfp) fprintf(tfp, "SendCall(%d, 0x%x, %d) unit=%d\n",
							 au, ast[au].send_call, cnt, au);

					tot_call += 1;
					ast[au].send_word = 0;
					ast[au].num_words[ast[au].send_call] = -2;
					if (cnt)
						ast[au].num_words[ast[au].send_call] = cnt;
					if (ast[au].num_call - 1 == ast[au].send_call)
						ast[au].done_calls = true;
				}
			}

			// iBlk
			if (ast[au].num_words[ast[au].send_call] == -2) {
				ast[au].send_call += 1;
				ast[au].send_word = -1;
			} else while (ast[au].send_word >= 0) {
				d = 0;
				d |= ast[au].send_word;
				d |= (uint64_t)ast[au].send_call << 32;
				d |= (uint64_t)au << 56;
				if (pAuUnits[au]->SendHostData(1, &d)) {
					if (tfp) fprintf(tfp, "SendHostData(0x%016llx)\n", (long long)d);
					tot_push += 1;
					ast[au].send_word += 1;
					if (ast[au].send_word ==
					    ast[au].num_words[ast[au].send_call]) {
						ast[au].send_call += 1;
						ast[au].send_word = -1;

						// flush data
						pAuUnits[au]->FlushHostData();
					}
				} else {
					break;
				}
			}

			// Return
			int errs;
			if (pAuUnits[au]->RecvReturn_htmain(errs)) {
				if (tfp) fprintf(tfp, "RecvReturn_htmain(%d) unit=%d\n", errs, au);
				tot_rtn += 1;

				aerr_cnt += errs;

				if (ast[au].recv_word >= 0) {
					err_cnt += 1;
					int exp = ast[au].num_words[ast[au].recv_call];
					fprintf(stderr, "ERROR: unit%d call %d missing %d words\n",
						au, ast[au].recv_call, exp - ast[au].recv_word);
				}

				ast[au].recv_call += 1;

				if (ast[au].num_call == ast[au].recv_call) {
					active_cnt -= 1;
					ast[au].done = true;
				}

				int pcnt = (rand() % MAX_FLUSH) + 1;
				pAuUnits[au]->SendHostMsg(FLUSH_PERIOD, pcnt);
			}

			// oBlk
			while (pAuUnits[au]->RecvHostData(1, &d)) {
				if (tfp) fprintf(tfp, "RecvHostData(0x%016llx)\n",
						 (long long)d);
				tot_pop += 1;

				if (ast[au].recv_word < 0)
					ast[au].recv_word = 0;

				uint64_t exp = 0;
				exp |= (uint64_t)ast[au].recv_word;
				exp |= (uint64_t)ast[au].recv_call << 32;
				exp |= (uint64_t)au << 56;

				ast[au].recv_word += 1;

				if (ast[au].recv_word ==
				    ast[au].num_words[ast[au].recv_call])
					ast[au].recv_word = -1;

				if (d != exp) {
					err_cnt += 1;
					fprintf(stderr, "ERROR: unit%d expected 0x%016llx != 0x%016llx\n",
						au, (long long)exp, (long long)d);
				}
				assert(!err_cnt);
			}
		}
	}

	time_now = time(NULL);
	printf("end time = %s", ctime(&time_now));

	for (int unitId = 0; unitId < nau; unitId += 1)
		delete pAuUnits[unitId];

	delete pHtHif;

	if (err_cnt || aerr_cnt)
		printf("FAILED: detected %d/%d issues!\n", err_cnt, aerr_cnt);
	else
		printf("PASSED\n");

	return err_cnt;
}
