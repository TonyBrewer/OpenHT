/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*
 * This file copyright(C) Convey Computer Corporation, 2014.
 * All rights reserved.
 */
#include <semaphore.h>

extern short		g_powersaveActiveCnt;
extern short		g_powersaveResponsible[];
extern bool		g_powersaveWorkerActive[];
extern uint64_t		g_powersaveWake;
extern uint64_t		g_powersaveNoWork;
extern pthread_mutex_t	g_powersaveLock;	// Protects access to above items

extern sem_t		g_powersaveDoorBell[];

extern void PowersaveInit(void);
extern short Workload(short unitId, short activeCnt);
extern void AdjustWorkload(CWorker *me, short workload);
extern void PowersaveSchedule(CWorker *me);
