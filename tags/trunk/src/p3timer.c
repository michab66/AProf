/*
 * $RCSfile: p3timer.c,v $
 *
 * Handle-Modul für CIA-Timer
 *
 * Koppeln von Timer A & B zu einem 32-Bit-Zähler
 * Zählfrequenz auf E-Takt einstellen (716 kHz = 1/10 ProTakt)
 * CIA(B)
 *
 * $Revision: 1.1.1.1 $ $Date: 2002/05/31 23:15:51 $
 *
 * © 1991-94 Michael G. Binz
 */



#include <stdio.h>

#include <exec/execbase.h>
#include <exec/types.h>
#include <libraries/dos.h>
#include <libraries/dosextens.h>
#include <resources/cia.h>
#include <hardware/cia.h>
#include <clib/cia_protos.h>

#include <pragmas/exec_lib.h>

#include "timer.h"


/* Basisstrukturen */
extern struct CIA ciab;             // Bezug wird vom Linker eingesetzt
extern struct ExecBase *SysBase;


/* Anzahl der TaskSwitches zwischen 1. StartTimeMarkCIA
 * und letztem StopTimeMarkCIA.
 */
static ULONG task_switches;

/* Fehlmessung für Aufruf von StartTimeMark und StopTimeMark
 */
static ULONG StartStopMeaError;      

/* Erkennen eines Overflow */
static struct Library *CIAResource;
static struct Interrupt CIATimerInt;


/* Flags */
static BOOL running = FALSE;        // TRUE = Timer läuft
static BOOL init = FALSE;           // TRUE = Timer initialisiert
BOOL timer_err = FALSE;             // TRUE, falls Fehler aufgetreten


/* Typ für Umwandlung CIA-Register -> 32 Bit */
union Reg2Long
{
   ULONG longval;
   UBYTE reg[ sizeof( ULONG ) ];
};


/* Prototypen */
static void TimeOver( void );
static void execStartTimerCIA(void);
static void execStopTimerCIA(void);



/* TimeOver
 *
 * Routine wird bei Timeroverflow als Interrupt ausgeführt
 */
static void TimeOver()
{
   timer_err = TRUE;
}



/* execStartTimerCIA
 *
 * Starten des Timers (extern)
 * Funktion wird im Supervisor Mode direkt vom Task-Scheduler abgearbeitet
 */
static void execStartTimerCIA()
{
   if ( running )
      ciab.ciacra |= CIACRAF_START;
}



/* execStopTimerCIA
 *
 * Anhalten des Timers (extern)
 * Funktion wird im Supervisor Mode direkt vom Task-Scheduler abgearbeitet
 */
static void execStopTimerCIA()
{
   if ( running )
   {
      ciab.ciacra &= ~CIACRAF_START;
      task_switches++;
   }
}



/* RawTimeCIA
 *
 * Auslesen des Timers in CIA-B
 *
 * Rückgabe: Aktueller Stand des Timers
 *
 */

ULONG RawTimeCIA( void )
{
   union Reg2Long change;

   change.reg[0] = ciab.ciatbhi;
   change.reg[1] = ciab.ciatblo;
   change.reg[2] = ciab.ciatahi;
   change.reg[3] = ciab.ciatalo;

   return change.longval;
}



/* SetTimerCIA
 *
 * Rücksetzen des Timers auf Startwert
 *
 * Eingabe: Startwert
 */
static void SetTimerCIA( ULONG val )
{
   union Reg2Long change;

   change.longval = val;

   ciab.ciatbhi = change.reg[0];
   ciab.ciatblo = change.reg[1];
   ciab.ciatahi = change.reg[2];
   ciab.ciatalo = change.reg[3];
}



/* StartTimerCIA
 *
 * Starten des Timers
 *
 * Rückgabe: keine
 *
 */
void StartTimerCIA( void )
{
   running = TRUE;
   ciab.ciacra |= CIACRAF_START;
}



/* StopTimerCIA
 *
 * Anhalten des Timers
 *
 * Rückgabe: 0 = standard
 *           1 = dto.
 */
void StopTimerCIA( void )
{
   ciab.ciacra &= ~CIACRAF_START;
   running = FALSE;
}




/* StartTimeMarkCIA
 *
 * Zeitnahme starten
 *
 * Ausgabe: Aktueller Stand der Uhr
 */
ULONG StartTimeMarkCIA( void )
{
   return RawTimeCIA();
}




/* StopTimeMarkCIA
 *
 * Zeitnahme beenden
 *
 * Rückgabe: Seit letztem StartTimeMarkCIA vergangene Ticks
 */
ULONG StopTimeMarkCIA( ULONG time )
{
   return time - RawTimeCIA();
}



/* ExitTimerCIA
 *
 * Abschalten der Timer in CIA-B
 *
 * Rückgabe: Anzahl der Taskswitches zwischen InitTimer und ExitTimer
 */
ULONG ExitTimerCIA( void )
{
   struct Process *pro;

   if ( init )
   {
      if ( running )
         StopTimerCIA();

      Disable();
         pro = (struct Process *)FindTask( NULL );
         pro->pr_Task.tc_Flags &= ~( TF_SWITCH | TF_LAUNCH );
         pro->pr_Task.tc_Switch = NULL;
         pro->pr_Task.tc_Launch = NULL;
      Enable();

      RemICRVector( CIAResource, CIAICRB_TB, &CIATimerInt );

      ciab.ciacra &= ~CIACRAF_START;
      ciab.ciacrb &= ~CIACRBF_START;

      init = FALSE;
   }

   return task_switches;
}



/* InitTimerCIA
 *
 * Komplette Initialisierung der Timer in CIA-B.
 * Koppeln zu 32 Bit-Zähler, Zählfrequenz E-Takt 716 Hz einstellen
 *
 * Rückgabe: 0 = Timer nicht verfügbar
 *           1 = Timer initialisiert
 */
BOOL InitTimerCIA( void )
{
   struct Process *pro;

   /* Kontrolle der Startbits in CRA & CRB */
   if ( (ciab.ciacra & CIACRAF_START) || (ciab.ciacrb & CIACRBF_START) )
      return FALSE;                    // => Timer nicht verfügbar

   /* Öffnen der Ressource */
   if ( (CIAResource = (struct Library *)OpenResource( CIABNAME )) == NULL )
      return FALSE;

   /*  Overflow-Interrupt installieren */
   CIATimerInt.is_Node.ln_Type = NT_INTERRUPT;
   CIATimerInt.is_Node.ln_Pri  = 127;
   CIATimerInt.is_Node.ln_Name = "profiler_cia_timer";
   CIATimerInt.is_Code         = TimeOver;

   if ( AddICRVector( CIAResource, CIAICRB_TB, &CIATimerInt ) )
      return FALSE;

   /* Switch und Launch Funktionen in Prozeßstruktur installieren */
   Disable();
      pro = (struct Process *)FindTask( NULL );
      pro->pr_Task.tc_Switch = execStopTimerCIA;
      pro->pr_Task.tc_Launch = execStartTimerCIA;
      pro->pr_Task.tc_Flags |= ( TF_SWITCH | TF_LAUNCH );
   Enable();

   /* Zählfrequenz Timer A auf E-Takt stellen (ca. 716 kHz)          */
   ciab.ciacra &= ~CIACRAF_INMODE;       /* %1101 1111 > löschen     */

   /* Timer B an Unterlauf Timer A koppeln */
   ciab.ciacrb &= ~(CIACRBF_INMODE0|CIACRBF_INMODE1);
   ciab.ciacrb |= CIACRBF_IN_TA;

   /* RUNMODE = continous */
   ciab.ciacra &= ~CIACRAF_RUNMODE;      /* löschen  */
   ciab.ciacrb |=  CIACRBF_RUNMODE;      /* setzen   */

   /* Startwerte der Zähler initialisieren (0xffffffff) */
   SetTimerCIA( (ULONG)-1 );

   /* Interne Init-Markierung setzen */
   init = TRUE;
   task_switches = 0;

   // Fehlerflag zurücksetzen
   timer_err = FALSE;

   return TRUE;
}



/* Tick2ms - Wandelt Tickwert in Millisekunden
 *
 * Eingabe: int    - Tickwert
 * Ausgabe: double - Millisekunden
 */

double Tick2Ms( ULONG tick )
{
   return (double)tick * (double)1000 / (double)SysBase->ex_EClockFrequency;
}


