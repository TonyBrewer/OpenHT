#include "Ht.h"
using namespace Ht;
#include <cmath>

#ifdef HT_VSIM
#define DEFAULT_MAX_SIZE 18000//96*1048576
#else
#ifdef HT_SYSC
#define DEFAULT_MAX_SIZE 20000//96*1048576
#else
#define DEFAULT_MAX_SIZE 4*1048576//96*1048576
#endif
#endif

#define DEFAULT_MIN_SIZE 16*1024
#define DEFAULT_STRIDE 192
#define LINE_SIZE 64
#define PAGE_SIZE 4096
#define MIN_ACCESSES 1048576

#define BLOCK 100 

uint64_t delta_usec(uint64_t *usr_stamp);
uint64_t **init_strided(uint64_t **dataBuf, uint64_t size, uint64_t stride);
uint64_t **init_random(uint64_t **dataBuf, uint64_t size);
uint64_t walk_scalar(uint64_t **dataPtr, uint64_t naccesses);

uint64_t **output;

int main(int argc, char **argv)
{
	int aidx = 1;
	uint64_t maxsize = DEFAULT_MAX_SIZE;
	uint64_t minsize = DEFAULT_MIN_SIZE;
	uint64_t randomp = 0;
	uint64_t naccesses = 0;
	uint64_t use_cp = 1;
	uint64_t use_cpmem = 1;
	uint64_t ndone;
	uint64_t ntimes = 3;
	uint64_t stride = DEFAULT_STRIDE;
	uint64_t tend;
	double tlat,tsamp;

	uint64_t *dataBuf;
	uint64_t **dataPtr;


	// Check/Get Command-Line Args
	while (aidx < argc) {
		if(strcmp(argv[aidx],"-minsize")==0) {
			aidx++;
			minsize=atoi(argv[aidx]);
		}
		else if(strcmp(argv[aidx],"-maxsize")==0) {
			aidx++;
			maxsize=atoi(argv[aidx]);
		}
		else if(strcmp(argv[aidx],"-stride")==0) {
			aidx++;
			stride=atoi(argv[aidx]);
		} 
		else if(strcmp(argv[aidx],"-random")==0) {
			randomp = 1;
		} 
		else if(strcmp(argv[aidx],"-n")==0) {
			aidx++;
			naccesses=atoi(argv[aidx]);
		} 
		else if(strcmp(argv[aidx],"-cp")==0)
			use_cp=1;
		else if(strcmp(argv[aidx],"-hp")==0) 
			use_cp=0;
		else if(strcmp(argv[aidx],"-cpmem")==0) 
			use_cpmem=1;
		else if(strcmp(argv[aidx],"-hpmem")==0) 
			use_cpmem=0;
		else  {
			fprintf(stderr, "argument %s not recognized\n",argv[aidx]);
			fprintf(stderr, "usage: %s [-hp|-cp] [-hpmem|-cpmem] [-minsize <bytes>] [-maxsize <bytes>] [-stride <bytes]> [-n naccesses]\n", argv[0]);
			exit(1);
		}
		aidx++;
	}


	if(naccesses <= 0)
#ifdef HT_VSIM
		naccesses = maxsize/sizeof(*dataPtr)/8;
#else
		naccesses = maxsize/sizeof(*dataPtr);
#endif

	printf("Memory Latency Microbenchmark\n");
	printf("  Running with:\n");
	printf("    Min Size = %ld\n", minsize);
	printf("    Max Size = %ld\n", maxsize);
	printf("    Stride   = %ld\n", stride);
	printf("    %s\n", randomp ? "Random Init" : "Strided Init");
	printf("    Accesses = %ld\n", naccesses);
	printf("    %s accessing %s memory\n", use_cp ? "Coprocessor" : "Host", use_cpmem ? "Coprocessor" : "Host");
	printf("\n");
	fflush(stdout);


	// Get Coprocessor / Unit handles
	CHtHif *pHtHif = new CHtHif();
	CHtAuUnit * pAuUnit = new CHtAuUnit(pHtHif);

	if (use_cpmem) {
		dataBuf = (uint64_t*)pHtHif->MemAlloc(maxsize+2*PAGE_SIZE);
	} else {
		dataBuf = (uint64_t*)malloc(maxsize+2*PAGE_SIZE);
	}
	if (!dataBuf) {
		fprintf(stderr, "malloc() failed.\n");
		exit(-1);
	}

	// Align Buffer Pointer
	printf("  Buffer address is:    0x%lx\n", (uint64_t)dataBuf);
	printf("  Aligning to boundary: 0x%lx\n", (uint64_t)PAGE_SIZE);
	dataPtr = (uint64_t**)(((uint64_t)dataBuf + PAGE_SIZE ) & ~(PAGE_SIZE-1));
	printf("  Starting address is:  0x%lx\n", (uint64_t)dataPtr);
	printf("\n");
	printf("\n");
	fflush(stdout);

	// Do work
	uint64_t size = minsize;
	while (size < maxsize) {
    
		if(randomp == 1) {
			dataPtr = init_random(dataPtr,size);
		} else {
			dataPtr = init_strided(dataPtr,size,stride);
		}

		tlat = 9999999999.0;
		for(uint64_t i = 0; i < ntimes; i++) {
			delta_usec(NULL);
			
			// Work Happens
			if (use_cp) {
				pAuUnit->SendCall_htmain((uint64_t)dataPtr, naccesses);
				while (!pAuUnit->RecvReturn_htmain())
					usleep(1000);
				ndone = naccesses;
			} else {
				ndone = walk_scalar(dataPtr, naccesses);
			}

			tend = delta_usec(NULL);
			tsamp = (double)(tend*1000.0)/(double)ndone;
			if(tsamp < tlat)
				tlat = tsamp;
			//printf("  NDONE = %ld, tend = %ld, tsamp = %lf, tlat = %lf\n",ndone, tend, tsamp, tlat);
		}

		printf("  Size: %10ld bytes  |  Load-to-Use Latency: %8.4lf nsec\n",size,tlat);
		size = size + size/10;
	}

	printf("\n");
	printf("\n");
	printf("PASSED\n");
	printf("\n");
	fflush(stdout);


	// free memory
	if (use_cpmem) {
		pHtHif->MemFree(dataBuf);
	} else {
		free(dataBuf);
	}
	delete pAuUnit;
	delete pHtHif;

	return 0;
}


uint64_t **init_strided(uint64_t **buf,uint64_t size,uint64_t stride)
{
	  uint64_t i,nptrs;
	  uint64_t **p;
	  uint64_t **start,**last;
	  uint64_t **next;

	  start = buf;
	  p = start;
	  last = (uint64_t **) (((uint64_t) start) + size)-1;
	  nptrs = size / sizeof(*p);

	  for(i=0;i < nptrs-1;i++) {
	  	  next = (uint64_t **) (((uint64_t)p) + stride);
		  if(next > last) 
	  	  	  next = (uint64_t **) ((uint64_t)start+sizeof(*p));
		  *p = (uint64_t *) next;
		  p = next;
	  }

	  *p = (uint64_t *)start;

	  return (start);
}


uint64_t **init_random(uint64_t **buf,uint64_t size)
{
	  uint64_t i,nptrs;
	  nptrs = size / sizeof(*buf);
	  uint64_t *idx = (uint64_t *) calloc(nptrs,sizeof(uint64_t));
	  for(i=0;i<nptrs-1;i++) idx[i] = i+1;
	  for(i=0;i<nptrs-1;i++) {
	  	  uint64_t j = rand() % (nptrs-1);
		  uint64_t t = idx[j];
		  idx[j] = idx[i];
		  idx[i] = t;
	  }
	  int ths = 0;
	  for(i=0;i<nptrs-1;i++) {
#ifdef DEBUG
		  printf("buf[%d] -> %d\n",ths,idx[i]);
#endif
		  buf[ths] = (uint64_t *)&buf[idx[i]];
		  ths = idx[i];
	  }
#ifdef DEBUG
	  printf("buf[%d] -> %d\n",ths,0);
#endif
	  buf[ths] = (uint64_t *)&buf[0];
	  //  buf[nptrs-1] = &buf[0];
#ifdef DEBUG
	  for(i=0;i<nptrs;i++)
		  printf("idx[%d] = %ld; (%x) -> %x\n",i,idx[i],&buf[i],buf[i]);
#endif
	  free(idx);
	  return (&buf[0]);
}


uint64_t walk_scalar(uint64_t **dataPtr, uint64_t naccesses) {
	uint64_t i;

	for(i = 0; i < naccesses;i += BLOCK) {
		/* ten accesses */
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		/* ten accesses */
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		/* ten accesses */
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		/* ten accesses */
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		/* ten accesses */
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		/* ten accesses */
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		/* ten accesses */
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		/* ten accesses */
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		/* ten accesses */
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		/* ten accesses */
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
		dataPtr = (uint64_t **) *dataPtr;
	}
	i=i-BLOCK;
	output=dataPtr;

	return(i);
}
