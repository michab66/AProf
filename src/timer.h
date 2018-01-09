/*
 * $RCSfile: timer.h,v $
 *
 * Handle-Modul für CIA-Timer
 *
 * $Revision: 1.1.1.1 $ $Date: 2002/05/31 23:15:55 $
 *
 * © 1991-94 Michael G. Binz
 */

#ifndef PROFILER_TIMER_H
#define PROFILER_TIMER_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

extern BOOL timer_err;

/* Prototypen */

#ifdef __cplusplus
extern "C" {
#endif

ULONG StartTimeMarkCIA( void );
ULONG StopTimeMarkCIA( ULONG );
void StartTimerCIA( void );
void StopTimerCIA( void );
BOOL InitTimerCIA( void );
ULONG ExitTimerCIA( void );		// Returns number of task switches
double Tick2Ms( ULONG );
ULONG RawTimeCIA( void );

#ifdef __cplusplus
}
#endif

#endif

