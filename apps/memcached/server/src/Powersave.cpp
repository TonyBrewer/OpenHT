/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*
 * This file Copyright (C) Convey Computer Corporation, 2014.
 * All rights reserved.
 */
#include "MemCached.h"

#include "Timer.h"
#include <semaphore.h>
#include <math.h>

#define MCD_UNITS  16

extern double   g_clocks_per_sec;

short		    g_powersaveActiveCnt = -1;
short		    g_powersaveResponsible[MCD_UNITS];
bool		    g_powersaveWorkerActive[MCD_UNITS];
uint64_t	    g_powersaveWake = 0;
uint64_t	    g_powersaveNoWork = 0;
pthread_mutex_t	g_powersaveLock;	// Protects access to above items

sem_t		    g_powersaveDoorBell[MCD_UNITS];

// Tunables
static uint64_t mainTimerDelay;
static uint64_t activeChangeDelay;
static float    highIdleMark;
static float    lowIdleMark;


void PowersaveInit() {
    short i;
    
    g_powersaveActiveCnt = g_settings.num_threads;
    pthread_mutex_init(&g_powersaveLock, 0);
    for (i=0; i<g_settings.num_threads; i++) {
        sem_init(&g_powersaveDoorBell[i], 0, 0);
        g_powersaveWorkerActive[i] = true;
        g_powersaveResponsible[i] = i;
    }

    // Set tunables
    mainTimerDelay    = 0.25 * g_clocks_per_sec;
    activeChangeDelay = 3.00 * g_clocks_per_sec;
    
    if (g_settings.powersave_preset == 1) {
        // preset 1 (save less power; better performance)
        highIdleMark = 97.0;
        lowIdleMark  = 88.0;
    } else if (g_settings.powersave_preset == 3) {
        // preset 3 (save more power)
        highIdleMark = 90.0;
        lowIdleMark  = 50.0;
    } else {
        // preset 2 (default)
        highIdleMark = 95.0;
        lowIdleMark  = 85.0;
    }
}

// Tells any Worker how big of a workload it should have (possibly, but not
// necessarily, including itself) given the total number of active Workers.
short Workload(short unitId, short activeCnt) {

    assert(unitId < g_settings.num_threads);
    assert(activeCnt <= g_settings.num_threads);

#if 0
    // Special case - 16 workers and power-of-two increments:
    assert(activeCnt == 16 || activeCnt == 8 || activeCnt == 4 ||
           activeCnt == 2 || activeCnt == 1);
    
    if (g_settings.num_threads == 16) {
        if (activeCnt == 1) {
            if (unitId == 0) return 16;
            else return 0;
        } else if (activeCnt == 2) {
            if (unitId % 8 == 0) return 8;
            else return 0;
        } else if (activeCnt == 4) {
            if (unitId % 4 == 0) return 4;
            else return 0;
        } else if (activeCnt == 8) {
            if (unitId % 2 == 0) return 2;
            else return 0;
        } else {
            return 1;
        }

    }
#endif

    // General case:

    if (unitId >= activeCnt) return 0;
    
    short groupSize = floor(g_settings.num_threads / activeCnt);
    short firstBigGroup = activeCnt - (g_settings.num_threads % activeCnt);

    if (unitId >= firstBigGroup) return (groupSize+1);

    return groupSize;
}

static void ReleaseFdEvents(CWorker *me, CWorker *other) {
    short i;

    assert(me);
    assert(other);
    
    for (i=0; i < other->m_connAllocCnt; i++) {
        CConn *c = other->m_connList[i];
        if (c) {
            if (!c->m_bFree && c->m_bEventActive) {
                event_base_set(me->m_base, &c->m_event);
                event_del(&c->m_event);
            }
        }
    }

    return;
}

static void ClaimFdEvents(CWorker *me, CWorker *other) {
    short i;

    assert(me);
    assert(other);
    
    for (i=0; i < other->m_connAllocCnt; i++) {
        CConn *c = other->m_connList[i];
        if (c) {
            if (!c->m_bFree && c->m_bEventActive) {
                event_base_set(me->m_base, &c->m_event);
                event_add(&c->m_event, 0);
            }
        }
    }
}

void AdjustWorkload(CWorker *me, short workload) {
    short i;

    if (me->m_workload > workload) {

        // Reduce workload
        for (i=g_settings.num_threads-1; i>0; i--) {
            if (me->m_workload > workload) {
                if (me->m_responsibleFor[i]) {
                    CWorker *imp = g_pWorkers[i];

                    ReleaseFdEvents(me, imp);
                    me->m_responsibleFor[i] = false;
                    me->m_workload--;

                    pthread_mutex_lock(&g_powersaveLock);
                    assert(g_powersaveResponsible[i] == me->m_unitId);
                    g_powersaveResponsible[i] = -1;
                    pthread_mutex_unlock(&g_powersaveLock);
                }	     
            }
        }

        // Nothing left to do? Go to sleep.
        if (me->m_workload == 0) {
            assert(me->m_unitId != 0);

            pthread_mutex_lock(&g_powersaveLock);
            g_powersaveWorkerActive[me->m_unitId] = false;
            pthread_mutex_unlock(&g_powersaveLock);

            sem_wait(&g_powersaveDoorBell[me->m_unitId]);

            // ...time passes...

            // We have slept and re-awoken. Try to claim one work queue.
            // If needed, we'll pick up more work on the next scheduling loop.

            short claimId = -1;
            pthread_mutex_lock(&g_powersaveLock);
            if (g_powersaveResponsible[me->m_unitId] == -1) {
                claimId = me->m_unitId;
                g_powersaveResponsible[me->m_unitId] = me->m_unitId;
            } else {
                bool done=false;
                for (i=g_settings.num_threads-1; !done && i>0; i--) {
                    if (g_powersaveResponsible[i] == -1) {
                        claimId = i;
                        g_powersaveResponsible[i] = me->m_unitId;
                        done = true;
                    }
                }
            }
            pthread_mutex_unlock(&g_powersaveLock);

            if (claimId != -1) {
                ClaimFdEvents(me, g_pWorkers[claimId]);
                me->m_responsibleFor[claimId] = true;
                me->m_workload++;
            }
        }
    }
	

    else if (me->m_workload < workload) {
        short claimId;
        // Need to add more work.

        for (i=1; i<g_settings.num_threads; i++) {
            if (me->m_workload < workload) {
                claimId = -1;
                pthread_mutex_lock(&g_powersaveLock);
                if (g_powersaveResponsible[i] == -1) {
                    claimId = i;
                    g_powersaveResponsible[i] = me->m_unitId;
                }	
                pthread_mutex_unlock(&g_powersaveLock);

                if (claimId != -1) {
                    ClaimFdEvents(me, g_pWorkers[claimId]);
                    me->m_responsibleFor[claimId] = true;
                    me->m_workload++;
                }
            }
        }

    }
    return;
}

void PowersaveSchedule(CWorker *me) {
    // Only Worker 0 should access static vars!
    static uint64_t changeTimer = 0;

	uint64_t now = (uint64_t)CTimer::Rdtsc();	
	me->m_wakeCount++;

	if (now > me->m_powersaveTimer) {
	    short activeCnt = -1;

	    me->m_powersaveTimer = now + (0.25 * g_clocks_per_sec);

	    pthread_mutex_lock(&g_powersaveLock);

	    g_powersaveWake += me->m_wakeCount;
	    me->m_wakeCount = 0;

	    g_powersaveNoWork += me->m_noWorkCount;
	    me->m_noWorkCount = 0;

	    if (me->m_unitId == 0) {
            short awake = g_powersaveActiveCnt;
            float idlePct = 100 * ((float)g_powersaveNoWork / g_powersaveWake);
            g_powersaveWake   = 0;
            g_powersaveNoWork = 0;

#if 0
            int i;
            fprintf(stderr, "Idle: %2.2f%%; ", idlePct);
            fprintf(stderr, "Resp: ");
            for (i=0; i<g_settings.num_threads; i++) {
                fprintf(stderr, "%d ", g_powersaveResponsible[i]);
            }
            fprintf(stderr, "\n");
#endif

            if (idlePct > highIdleMark) {
                if (now > changeTimer) {
                    if (g_powersaveActiveCnt == 1) {
                        me->m_slowSpin = true;
                    } else if (g_powersaveActiveCnt > 1) {
                        g_powersaveActiveCnt--;
                    }
                    changeTimer = now + activeChangeDelay;
                }
            } else if (idlePct < lowIdleMark) {
                if (me->m_slowSpin == true) {
                    me->m_slowSpin = false;
                } else if (g_powersaveActiveCnt < g_settings.num_threads) {
                    g_powersaveActiveCnt *= 2;
                    if (g_powersaveActiveCnt > g_settings.num_threads) {
                        g_powersaveActiveCnt = g_settings.num_threads;
                    }
                }
                changeTimer = now + activeChangeDelay;
            }

#if 0
            if (g_powersaveActiveCnt != awake) {
                fprintf(stderr, "Powersave: activeCnt is now %d\n", g_powersaveActiveCnt);
            }
#endif

            // Wake up any sleepers.
            while (g_powersaveActiveCnt > awake) {
                if (g_powersaveWorkerActive[awake] == false) {
                    sem_post(&g_powersaveDoorBell[awake]);
                    g_powersaveWorkerActive[awake] = true;
                    awake++;
                }
            }
		
	    }
	    activeCnt = g_powersaveActiveCnt;
	    pthread_mutex_unlock(&g_powersaveLock);
	    short workload = Workload(me->m_unitId, activeCnt);

	    if (workload != me->m_workload) {
            AdjustWorkload(me, workload);
	    }

	}

	
	// Now, get to work...
	short i;
	int rc = 0;

	for (i=0; i<g_settings.num_threads; i++) {
	    if (me->m_responsibleFor[i]) {
            CWorker *imp = g_pWorkers[i];
            if (imp) {
                g_pTimers[5]->Start(imp->m_unitId);
                g_pSamples[2]->Sample(imp->m_freeCurr, imp->m_unitId);

                rc |= imp->AddCloseMsgToRecvBuffer();
                rc |= imp->FlushRecvBuf();
                rc |= imp->ProcessRespBuf();
                rc |= imp->TransmitFromList();
                rc |= imp->HandleDispatchCmds();
	    
                g_pTimers[5]->Stop(imp->m_unitId);

                // Add any pending connections to my event_base.
                pthread_mutex_lock(&imp->m_psPendLock);
                while (imp->m_psPendHead) {
                    CConn *c = imp->m_psPendHead;
                    imp->m_psPendHead = c->m_psPendNext;
                    c->m_psPendNext = 0;
		
                    event_base_set(me->m_base, &c->m_event);
                    event_add(&c->m_event, 0);
                    c->m_bEventActive = true;
                    rc++;
                }
                pthread_mutex_unlock(&imp->m_psPendLock);
		    
            }
	    }	    
	}

	// Adjust no-work count for computing idlePct
	if (rc == 0) me->m_noWorkCount++;
}
