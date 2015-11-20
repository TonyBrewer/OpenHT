#include "Ht.h"
#include <sys/time.h>
#include <sys/resource.h>
#include "utilities.h"

using namespace Ht;
#include "vgups.h"

#define CPUSEC() (HPL_timer_cputime())
#define RTSEC() (CNY_timer_walltime())

#define TblSizeBytes (TblSize << 3)
extern "C" {
    int vgups(u64Int TblSize, u64Int* hostTable, u64Int* rndTblPtr, u64Int nUpdate, double *cputime, double *realtime, void *extdata)
{
  bool useCoprocMem = true;
    // define/construct the personality class
    CHtHifParams hifParams;
    hifParams.m_iBlkTimerUSec = 100;
    hifParams.m_oBlkTimerUSec = 100;
    
    Ht::CHtHif *pHtHif = new CHtHif(&hifParams);
    int unitCnt = pHtHif->GetUnitCnt() > AE_LIM * 16 ? AE_LIM * 16 : pHtHif->GetUnitCnt();
    if (unitCnt != CNY_HT_AEUCNT) {
      printf("Warning: Hardware unit count of %d does not match compiled unit count of %d\n",
	     unitCnt, (int)CNY_HT_AEUCNT );
      printf("         Results will not verify!\n");
      //      unitCnt = 1;
    }
    CHtAuUnit ** pAuUnits = new CHtAuUnit * [unitCnt];
    
    for (int unitId = 0; unitId < unitCnt; unitId += 1)
        pAuUnits[unitId] = new CHtAuUnit(pHtHif);
    
    // allocate coprocessor memory
    void* cpTable;
    if (useCoprocMem) {
      // need largest possible pages because this code's random
      // access patterns is not TLB friendly - use 1 GB alignment
      // large allocations will get large pages by defualt
      if (!(cpTable = pHtHif->MemAlloc((size_t)TblSizeBytes))) // TblSize is units of QW
	{
	  printf("Failed to allocate memory for the update table %lld.\n",
	    (long long)TblSize);
	  return 1;
	}
      // initialize coproc memory
      printf("Copying Table from 0x%012lx to 0x%012lx (%lld bytes)\n",
	     (unsigned long) hostTable, (unsigned long) cpTable,
	     (long long) TblSizeBytes);
      pHtHif->MemCpy(cpTable, hostTable, TblSizeBytes);
    } else {
      printf("Using host table at 0x%012lx\n", (unsigned long) hostTable);
      cpTable = hostTable;
    }
    
    // initialize the coprocessor (ie. load operating parameters and thread seeds)
    pHtHif->SendAllHostMsg(OP_BASE, (u64Int)cpTable);
    pHtHif->SendAllHostMsg(OP_SIZE, (u64Int)TblSize);
    pHtHif->SendAllHostMsg(OP_UPDCNT, nUpdate/(unitCnt * ThreadCnt));
    for (int unit = 0; unit < unitCnt; unit++)
        for (u64Int thread = 0; thread < ThreadCnt; thread++)
	{
	    // send 64b seeds as 2x32b with prepended threadId
            pAuUnits[unit]->SendHostMsg(SET_INITSEED0, thread << 32 | (rndTblPtr[unit * ThreadCnt + thread] >>  0) & ((1LL << 32) - 1));
            pAuUnits[unit]->SendHostMsg(SET_INITSEED1, thread << 32 | (rndTblPtr[unit * ThreadCnt + thread] >> 32) & ((1LL << 32) - 1));
	}

    printf("Calling %d units\n", unitCnt);

    // get start times
    /* Begin timing here */
    *cputime = -CPUSEC();
    *realtime = -RTSEC();

    // Add extra loop around coproc call to artificially increase power/coproc time
    //
    uint64_t time_sec=0; // Default to 1 sec so we do just 1 copcall by default
    if (extdata != NULL) {
        time_sec = *(uint64_t*) extdata;
    }
    uint64_t timeout_us = time_sec*1000*1000;
    if (timeout_us != 0) {
      printf("Timeout_us is %lld \n", (long long)timeout_us);
    }
    struct timeval tv,first_read_time;
    uint64_t elapsed_time=0;

    gettimeofday(&first_read_time,NULL);

    do {
      // run the coprocessor
      for (int unit = 0; unit < unitCnt; unit++) {
          pAuUnits[unit]->SendCall_htmain();
      }
    
      for (int unit = 0; unit < unitCnt; unit++)
          while (!pAuUnits[unit]->RecvReturn_htmain())
              usleep(1000);

      gettimeofday(&tv,NULL);
      elapsed_time = (uint64_t)((uint64_t)tv.tv_sec -(uint64_t)first_read_time.tv_sec)*1000000LL
                     + ((uint64_t)tv.tv_usec-(uint64_t)first_read_time.tv_usec);
   }
   while ((elapsed_time<timeout_us));

    
    // get end times TBD
    /* End timed section */
    *cputime += CPUSEC();
    *realtime += RTSEC();
    
    printf("All units returned\n");

    if (useCoprocMem) {
      // move contents of coproc memory back to host for verification
      printf("Copying Table Back from 0x%012lx to 0x%012lx\n",(unsigned long) cpTable, (unsigned long) hostTable);
      pHtHif->MemCpy(hostTable, cpTable, TblSizeBytes);
    
      // free coproc memory
      pHtHif->MemFree(cpTable);
    }

    delete pHtHif;
    return 0;
}
}
