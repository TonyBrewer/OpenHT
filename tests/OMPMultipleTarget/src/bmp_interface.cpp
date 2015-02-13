#include "bmp_interface.h"
#include "bmp.h"

int global_radius;

EXTERN_C
bmap_t load_bmp(char *filename) {
    CBitmap *bitmap = new CBitmap(filename);
    return (bmap_t) bitmap;
}

EXTERN_C 
uint32_t get_bmp_num_rows(bmap_t bmap) {
    CBitmap *bitmap = (CBitmap *) bmap;
    return bitmap->GetHeight();
}

EXTERN_C 
uint32_t get_bmp_num_cols(bmap_t bmap) {
    CBitmap *bitmap = (CBitmap *) bmap;
    return bitmap->GetWidth();
}

EXTERN_C
uint8_t *get_bmp_buffer(bmap_t bmap) {
    CBitmap *bitmap = (CBitmap *) bmap;
    return bitmap->GetRedBitsWithBorder();
}

EXTERN_C
void set_bmp_from_buffer(bmap_t bmap, uint8_t *buffer) {
    CBitmap *bitmap = (CBitmap *) bmap;
    bitmap->SetRedBitsFromBuffer(buffer);
}

EXTERN_C
void save_bmp(bmap_t bmap, char *filename) {
    CBitmap *bitmap = (CBitmap *) bmap;
    bitmap->Save(filename, 8);
}
