#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bmp_interface.h"

extern int __htc_get_unit_count();
extern int global_radius;

int app_main(int argc, char **argv) {
    global_radius = 1;
    char *filename;
    char *newfilename;
    char *system_command;

    if (argc != 2) {
        filename = "lena512.bmp";
    } else {
        filename = argv[1];
    }

    newfilename = (char *)malloc(strlen("new_")+strlen(filename)+1);
    strcpy(newfilename, "new_");
    strcpy(newfilename + strlen(newfilename), filename);

    // Load image
    bmap_t bmap = load_bmp(filename);
    uint32_t rows = get_bmp_num_rows(bmap);
    uint32_t cols = get_bmp_num_cols(bmap);
    uint32_t arows = rows+2*global_radius;
    uint32_t acols = cols+2*global_radius;
    uint32_t bufsize = arows*acols;

    printf("dimensions are %d by %d\n", rows, cols);

    // Get image buffer
    uint8_t *image = get_bmp_buffer(bmap);

    // Process  

    uint8_t kernel3x3[] = {
      // Identity (sanity test)
//      0, 0, 0,
//      0, 1, 0,
//      0, 0, 0
//  
      // Sobel 3x3
//      -1, 0, 1,
//      -2, 0, 2,
//      -1, 0, 1

      // Laplace (common discrete approx 2)
     -1, -1, -1,
     -1,  8, -1,
     -1, -1, -1
    };

    // Allocate target temp buffer.
    extern void *stencil_cp_alloc(size_t);
    uint8_t *unew = (uint8_t *)stencil_cp_alloc(bufsize);
    uint8_t *uhost = (uint8_t *)malloc(bufsize);
    memset(unew, 0xff, bufsize);     // Make cg bugs obvious in output image.
    memset(uhost, 0xff, bufsize);

    // Generate a host version of the stencil for expected results testing.
    rhomp_stencil_conv2ds(uhost, image, acols, arows, 3, kernel3x3, 8192);

#pragma omp target teams num_teams(1)
    {
        rhomp_stencil_conv2ds(unew, image, acols, arows, 3, kernel3x3, 2);
    } /* end omp_target */

    if (memcmp(uhost, unew, bufsize) == 0) {
      printf("Coproc matches host.\nPASSED\n");
    } else {
      printf("FAILED: coproc does not match host.\n");
    }

    // Update and Save
    set_bmp_from_buffer(bmap, unew);
    save_bmp(bmap, newfilename);
             
    // Display
    char *viewer = "firefox ";
    //char *viewer = "shotwell ";
    system_command = (char *)malloc(strlen(viewer) + strlen(newfilename) + 1);
    strcpy(system_command, viewer);
    strcpy(system_command+(strlen(system_command)), newfilename);
    system(system_command);

    return 0;
}
