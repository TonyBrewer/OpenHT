#include <pthread.h>

#include "Ht.h"
using namespace Ht;

#undef DEBUG

static CHtHif *pHtHif;
static CHtAuUnit **pAuUnits;

extern "C" void pers_attach()
{
	static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

	pthread_mutex_lock(&mutex);

	// did a racing thread alread do this?
	if (pHtHif)
		return;

	pHtHif = new CHtHif();

	int unitCnt = pHtHif->GetUnitCnt();
	pAuUnits = (CHtAuUnit **)malloc(unitCnt * sizeof(CHtAuUnit *));
	for (int unit = 0; unit < unitCnt; unit++)
		pAuUnits[unit] = new CHtAuUnit(pHtHif);
#ifdef DEBUG
	fprintf(stderr, "Hif constructed #AUs = %d\n", unitCnt);
#endif

	pthread_mutex_unlock(&mutex);
}

extern "C" void *pers_cp_malloc(size_t size)
{
	if (!pHtHif) pers_attach();

	void *ptr = pHtHif->MemAllocAlign(4 * 1024, size);

	return ptr;
}

extern "C" void pers_cp_free(void *ptr)
{
	if (!pHtHif) pers_attach();

	pHtHif->MemFreeAlign(ptr);
	return;
}

extern "C" void pers_cp_memcpy(void *dst, void *src, size_t len)
{
	if (!pHtHif) pers_attach();

	pHtHif->MemCpy(dst, src, len);
	return;
}


extern "C" void pers_init_bfs_tree(int64_t nv, int64_t *bfs_tree)
{
	if (!pHtHif) pers_attach();

	pHtHif->SendAllHostMsg(BFS_ADDR, (uint64_t)bfs_tree);
	pHtHif->SendAllHostMsg(BFS_SIZE, (uint64_t)nv);

	int unitCnt = pHtHif->GetUnitCnt();
	uint64_t chunk = (nv + unitCnt) / unitCnt; /* split across units */
	chunk = (chunk + 7) / 8 * 8; /* round to cache lines */
	for (int unit = 0; unit < unitCnt; unit++) {
		uint64_t start = chunk * unit;
		int64_t end = start + chunk;
		if (end > nv) end = nv;
		pAuUnits[unit]->SendCall_htmain(0 /*init*/,
						start /*index*/,
						end /*stride*/);
	}

	// wait for return
	uint64_t unit_updCnt = 0;
	for (int unit = 0; unit < unitCnt; unit++)
		while (!pAuUnits[unit]->RecvReturn_htmain(unit_updCnt))
			usleep(1);

#ifdef DEBUG
	fprintf(stderr, "pers_init_bfs_tree:  all units returned\n");
#endif
}

extern "C" void pers_scatter_bfs(int64_t *k2, int64_t *bfs_tree, int64_t *bfs_packed)
{
	if (!pHtHif) pers_attach();


#define BFS_PACKED_X(k) (bfs_packed[2 * k])
#define VLIST_X(k) (bfs_packed[2 * k + 1])
//
//  for (k=0; k<(*k2); k++) {
//    bfs_tree[VLIST_X(k)] = BFS_PACKED_X(k);
//  }
	// BFS_SIZE used for K2 on scatter instruction
	pHtHif->SendAllHostMsg(BFS_SIZE, (uint64_t)*k2);
	pHtHif->SendAllHostMsg(BFS_ADDR, (uint64_t)bfs_tree);
	pHtHif->SendAllHostMsg(BFS_PACKED_ADDR, (uint64_t)bfs_packed);

	int unitCnt = pHtHif->GetUnitCnt();
	for (int unit = 0; unit < unitCnt; unit++) {
		pAuUnits[unit]->SendCall_htmain(1 /*scatter*/,
						unit /*index*/,
						unitCnt /*stride*/);
	}

	// wait for return
	uint64_t unit_updCnt = 0;
	for (int unit = 0; unit < unitCnt; unit++)
		while (!pAuUnits[unit]->RecvReturn_htmain(unit_updCnt))
			usleep(1);

#ifdef DEBUG
	fprintf(stderr, "pers_scatter_bfs:  all units returned\n");
#endif
}

#define XADJ_SIZE int64_t
extern "C" void pers_bottom_up(int64_t g500_ctl, int64_t nv,
			       int64_t *bfs_tree, int64_t *bfs_packed_cp,
			       XADJ_SIZE *xadj, int64_t *xoff,
			       int64_t **bfs_tree_bit, int64_t **bfs_tree_bit_new,
			       int64_t *k1, int64_t *k2, int64_t *oldk2)
{
#ifdef DEBUG
	fprintf(stderr, "pers_bottom_up:  start \n");
#endif

	if (!pHtHif) pers_attach();

	*oldk2 = *k2;

	pHtHif->SendAllHostMsg(BFS_CTL, (uint64_t)g500_ctl);
	pHtHif->SendAllHostMsg(BMAP_OLD_ADDR, (uint64_t)*bfs_tree_bit);
	pHtHif->SendAllHostMsg(BMAP_NEW_ADDR, (uint64_t)*bfs_tree_bit_new);
	pHtHif->SendAllHostMsg(XOFF_ADDR, (uint64_t)xoff);
	pHtHif->SendAllHostMsg(XADJ_ADDR, (uint64_t)xadj);
	pHtHif->SendAllHostMsg(BFS_ADDR, (uint64_t)bfs_tree);
	pHtHif->SendAllHostMsg(BFS_SIZE, (uint64_t)nv);

	int unitCnt = pHtHif->GetUnitCnt();
	for (int unit = 0; unit < unitCnt; unit++) {
		pAuUnits[unit]->SendCall_htmain(2 /*bfs*/,
						unit /*bmap index*/,
						unitCnt /*bmap stride*/);
	}

	// wait for return
	uint64_t updCnt = 0;
	uint64_t unit_updCnt = 0;
	for (int unit = 0; unit < unitCnt; unit++) {
		while (!pAuUnits[unit]->RecvReturn_htmain(unit_updCnt))
			usleep(1);
		updCnt += unit_updCnt;
	}

#ifdef DEBUG
	fprintf(stderr, "pers_bottom_up: all units returned, updCnt = %lld\n", (long long)updCnt);
#endif

	*k2 += updCnt;

	*k1 = *oldk2;

	// flip addresses for next iteration
	int64_t *temp;
	temp = *bfs_tree_bit;
	*bfs_tree_bit = *bfs_tree_bit_new;
	*bfs_tree_bit_new = temp;
}

extern "C" void pers_detach()
{
	static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

	pthread_mutex_lock(&mutex);

	// did a racing thread alread do this?
	if (!pHtHif)
		return;

#ifdef DEBUG
	fprintf(stderr, "deleting pHtHif\n");
#endif

	delete pHtHif;
	pHtHif = NULL;

	pthread_mutex_unlock(&mutex);
}
