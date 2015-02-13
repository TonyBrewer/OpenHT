#include "Ht.h"

extern FILE *tfp;

struct args_s {
	int		au;
	CHtModelAuUnit *pUnit;
	bool		halt;
};

static bool Unit(struct args_s *args)
{
	CHtModelAuUnit *pUnit = args->pUnit;
	bool active = false;

	int32_t errs = 0;
	int32_t word = 0;
	uint8_t p_au;
	uint16_t p_call;
	int32_t p_cnt;
	uint64_t recv;

	// data should only arrive in a call
	if (pUnit->PeekHostData(recv)) {
		fprintf(stderr, "ERROR: AU%d unexpected data 0x%016llx\n",
			args->au, (long long)recv);

		assert(0);
	}

	if (pUnit->RecvCall_htmain(p_au, p_call, p_cnt)) {
		active = true;

		if (tfp) fprintf(tfp, "AU%d RecvCall(%d, 0x%x, %d)\n",
				 p_au, p_au, p_call, p_cnt);

		while (word < p_cnt) {
			if (!pUnit->RecvHostData(1, &recv))
				continue;

			if (tfp) fprintf(tfp, "AU%d RecvHostData(0x%016llx)\n",
					 p_au, (long long)recv);

			uint64_t exp = 0;
			exp |= (uint64_t)p_au << 56;
			exp |= (uint64_t)p_call << 32;
			exp |= (uint64_t)word;

			if (exp != recv) {
				fprintf(stderr, "ERROR: AU%d expected 0x%016llx != 0x%016llx\n",
					(int)p_au, (long long)exp, (long long)recv);
				errs += 1;
				assert(0);
			}

			while (!pUnit->SendHostData(1, &exp)) ;
			if (tfp) fprintf(tfp, "AU%d SendHostData(0x%016llx)\n",
					 p_au, (long long)exp);

			word += 1;
		}
		while (!pUnit->SendReturn_htmain(errs)) ;
		if (tfp) fprintf(tfp, "AU%d SendReturn(%d)\n",
				 p_au, errs);
	}
	args->halt = pUnit->RecvHostHalt();

	return active;
}


#ifdef NOTHREADS
static void ModelLoop(int nau, struct args_s *args)
{
	int haltCnt = 0;

	while (haltCnt < nau) {
		bool active = false;
		for (int au = 0; au < nau; au += 1) {
			args[au].halt = false;
			active |= Unit(&args[au]);
			haltCnt += args[au].halt;
		}
		if (!active) usleep(100);
	}
	return;
}
#else
static void *ModelThread(void *vp)
{
	struct args_s *args = (struct args_s *)vp;

	do {
		args->halt = false;
		if (!Unit(args)) usleep(100);
	} while (!args->halt);
	return NULL;
}
#endif


void HtCoprocModel()
{
	CHtModelHif *pModel = new CHtModelHif;
	CHtModelAuUnit *const *pUnits = pModel->AllocAllAuUnits();
	int nau = pModel->GetUnitCnt();

	struct args_s *args = new args_s [nau];

	for (int au = 0; au < nau; au += 1) {
		args[au].au = au;
		args[au].pUnit = pUnits[au];
	}

#ifdef NOTHREADS
	ModelLoop(nau, args);
#else
	printf("Note: Using %d threads for model\n", nau); fflush(stdout);

	pthread_t *tid = new pthread_t [nau];

	for (int au = 0; au < nau; au += 1)
		pthread_create(&tid[au], NULL, ModelThread, (void *)&args[au]);

	for (int au = 0; au < nau; au += 1)
		pthread_join(tid[au], NULL);
#endif

	delete args;
	delete tid;

	pModel->FreeAllAuUnits();
	delete pModel;
}
