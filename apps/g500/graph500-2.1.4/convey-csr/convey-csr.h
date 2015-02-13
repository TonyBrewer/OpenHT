#include <stdlib.h>
#include <stdint.h>

#define XADJ_32
#undef TIMER_ON

#if defined (XADJ_32)
#define XADJ_SIZE int32_t
#else
#define XADJ_SIZE int64_t
#endif

extern void pers_attach();
extern void pers_detach();

extern void pers_cp_free(void *ptr);
extern void *pers_cp_malloc(size_t size);
extern void pers_cp_memcpy(void *dst, void *src, size_t len);

extern void pers_init_bfs_tree (int64_t nv, int64_t *bfs_tree);
extern void pers_scatter_bfs (int64_t *k2, int64_t *bfs_tree,
			      int64_t *bfs_packed);
extern void pers_bottom_up (int64_t g500_ctl, int64_t nv,
			    int64_t *bfs_tree, int64_t *bfs_packed_cp,
			    XADJ_SIZE *xadj, int64_t *xoff,
			    int64_t **bfs_tree_bit, int64_t **bfs_tree_bit_new,
			    int64_t *k1, int64_t *k2, int64_t *oldk2);
