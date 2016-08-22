#include "Ht.h"
using namespace Ht;

#define DFLT_ALIGN 0
#define DFLT_START 64
#ifdef HT_VSIM
#define DFLT_END   (long)4*1024L
#else
#ifdef HT_SYSC
#define DFLT_END   (long)4*1024*1024L
#else
#define DFLT_END   (long)4*1024*1024*1024L
#endif
#endif

void usage(char *);
double atime();
double rate(uint64_t bytes, double time);

int main(int argc, char **argv)
{
	double t1, t2;
	//uint64_t i;
	uint64_t align = DFLT_ALIGN;
	uint64_t end   = DFLT_END;
	uint64_t start = DFLT_START;
	uint64_t *a1;
	double time;

	// check command line args
	if (argc == 1) {
		// use defaults
	} else if (argc == 4) {
		 align = atoi(argv[1]);
		if (align < 0) {
			usage(argv[0]);
			return 0;
		}
		start = atoi(argv[2]);
		if(start <= 0){
			usage(argv[0]);
			return 0;
		}
		end = atoi(argv[3]);
		if(end < 0){
			usage(argv[0]);
			return 0;
		}
	} else {
		usage(argv[0]);
		return 0;
	}
	
	uint64_t size = end+2*align;
	printf("Dispatch Microbenchmark\n");
	printf("  Running with start = %ld, end = %ld, align = %ld, size = %ld\n", start, end, align, size);
	fflush(stdout);

	//malloc host
	uint64_t *temp_a1;
	t1 = atime();
	temp_a1 = (uint64_t *)malloc(size);
	t2 = atime();
	a1 = temp_a1;
	time = t2-t1;
	printf("MALLOC: malloc %ld bytes %lf sec, %lf MB/sec\n",size,time,rate(size,time));
	
	//declare host interface
	CHtHif *pHtHif = new CHtHif();
	//prepare units
	int unitCnt = pHtHif->GetUnitCnt();
	//printf("#AUs = %d\n", unitCnt);

	CHtAuUnit ** pAuUnits = new CHtAuUnit * [unitCnt];
	for (int unit = 0; unit < unitCnt; unit++)
		pAuUnits[unit] = new CHtAuUnit(pHtHif);

	// Coprocessor malloc
	uint64_t *temp_cp_a1;
	uint64_t *cp_a1;
	t1 = atime();
	temp_cp_a1 = (uint64_t*)pHtHif->MemAlloc(size);
	t2 = atime();		
	cp_a1 = temp_cp_a1;
	time  = t2-t1;
	printf("CPMALLOC: cny_cp_malloc %ld bytes %lf sec, %lf MB/sec\n",size,time,rate(size,time));
	
	//memset host to 0
	t1 = atime();
	memset(a1, 0, size);
	t2 = atime();
	time = t2-t1;
	printf("HOST INIT: init %ld bytes %10.2lf sec, %10.2lf MB/sec\n",size,time,rate(size,time));

	//host align
	if(align) 
		a1 = (uint64_t *) ((((uint64_t)a1 + (uint64_t)align)) & ~(((uint64_t)align-1)));
		      
	//coproc memset
	t1 = atime();
	pHtHif->MemSet(cp_a1, 0, size);
	t2 = atime();
        time = t2-t1;
        printf("CPINIT: init %ld bytes %10.2lf sec, %10.2lf MB/sec\n",size,time,rate(size,time));

	//coproc align
	if(align)
		cp_a1 = (uint64_t *) ((((uint64_t)cp_a1 + (uint64_t)align)) & ~(((uint64_t)align-1)));

	// avoid bank aliasing for performance
	if (unitCnt > 16 && !(unitCnt & 1)) unitCnt -= 1;
	printf("stride = %d\n", unitCnt);

	fflush(stdout);

	printf(" COPY rate using cny_cp_memcpy\n");
#ifdef TWOADDS
	printf("                   host->cp           cp add             cp add            cp->host\n");
	printf("     bytes        sec   mb/s        sec    mb/s        sec    mb/s        sec   mb/s\n");
#else
	printf("                   host->cp           cp add            cp->host\n");
	printf("     bytes        sec   mb/s        sec    mb/s        sec   mb/s\n");
#endif

	//loop from end to start, cut bytes in half each time 
	uint64_t bytes; 
	for(bytes = end; bytes >= start; bytes/=2) {

		printf("%10ld",bytes);
		// Send arguments to all units using messages
		pHtHif->SendAllHostMsg(OP1_ADDR, (uint64_t)cp_a1);
		pHtHif->SendAllHostMsg(SIZE, (uint64_t)(bytes/(sizeof(double)*8)));

		//copy host array to coproc
		t1 = atime();
		pHtHif->MemCpy(cp_a1, a1, bytes);
		t2 = atime();
		time = t2-t1;
		printf("   %8.6lf %6.1lf",time,rate(bytes,time));

		// Send calls to units
		t1 = atime();
		for (int unit = 0; unit < unitCnt; unit++)
			pAuUnits[unit]->SendCall_htmain(unit /*offset*/, unitCnt /*stride*/);
	
		// Wait for returns
		for (int unit = 0; unit < unitCnt; unit++) {
			while (!pAuUnits[unit]->RecvReturn_htmain())
				usleep(1000); 
		}

		t2 = atime();
		time = t2-t1;
		printf("   %8.6lf %7.1lf",time,rate(bytes,time));

#ifdef TWOADDS
	       	// Send calls to units
		t1 = atime();
		for (int unit = 0; unit < unitCnt; unit++)
			pAuUnits[unit]->SendCall_htmain(unit /*offset*/, unitCnt /*stride*/);
	
		// Wait for returns
		for (int unit = 0; unit < unitCnt; unit++) {
			while (!pAuUnits[unit]->RecvReturn_htmain())
				usleep(1000); 
		}

		t2 = atime();
		time = t2-t1;
		printf("   %8.6lf %7.1lf",time,rate(bytes,time));
#endif

		//move coproc array to host array
		t1 = atime();
		pHtHif->MemCpy(a1, cp_a1, bytes);
		t2 = atime();
		time = t2-t1;
		printf("   %8.6lf %7.1lf",time,rate(bytes,time));

		printf("\n");

	}

	printf("Completed...\n");
	printf("\n");
	printf("PASSED\n");
	printf("\n");

	// free memory
	free(temp_a1);
	pHtHif->MemFree(temp_cp_a1);
	delete pHtHif;

	return 0;
}

// Print usage message and exit with error.
void usage(char *p)
{
	printf("usage: %s [align] [start] [end] \n", p);
	exit(1);
}

//returns time in seconds
double atime()
{
	struct timeval tp;
	struct timezone tzp;
	gettimeofday(&tp, &tzp);
	return ((double)tp.tv_sec + (double)tp.tv_usec * 1.e-6);
}
//returns rate in mb/s
double rate(uint64_t bytes, double time)
{
  return ((double)bytes)*1.e-6/time;
}

