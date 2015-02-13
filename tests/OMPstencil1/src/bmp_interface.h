#ifndef BMP_INTERFACE_H
#define BMP_INTERFACE_H

#ifndef EXTERN_C
#if defined(__cplusplus)
#define EXTERN_C extern "C"
#else
#define EXTERN_C
#endif
#endif

#include <stdint.h>

typedef long bmap_t;

#ifndef _HTV

EXTERN_C bmap_t load_bmp(char *filename);
EXTERN_C uint32_t get_bmp_num_rows(bmap_t bmap);
EXTERN_C uint32_t get_bmp_num_cols(bmap_t bmap);
EXTERN_C uint8_t *get_bmp_buffer(bmap_t bmap);
EXTERN_C void set_bmp_from_buffer(bmap_t bmap, uint8_t *buffer);
EXTERN_C void save_bmp(bmap_t bmap, char *filename);

#endif

#endif /* BMP_INTERFACE_H */
