/*
 * $RCSfile: p3traphandler.c,v $
 *
 * Enth�lt C Traphandler zur Zeitmessung und Hilfsfkt.
 *
 * $Revision: 1.1.1.1 $ $Date: 2002/05/31 23:15:53 $
 *
 * � 1993,94 Michael G. Binz
 */

#include <exec/types.h>
#include "timer.h"
#include "pro.h"


#define BREAK_STACK_SIZE      2000

extern UWORD trap_tmp_cmd;                  // TODO

// Hier wird der Stack f�r die R�cksprungpunkte definiert
static union BreakPoint breakstack[BREAK_STACK_SIZE];

// Index des obersten freien Slot in 'breakstack'
static int bpidx;

/* Enth�lt den Wert des Stackpointers des Profilee
 * Wird zum Erkennen von Fktsanspr�ngen via jmp verwendet
 */
static ULONG lstack;

// Ist Safe Profiling gew�nscht?
static BOOL IsSafe;



static void SetSymbolITime( struct Symbol *s, ULONG timeval )
{
   if ( s->s_xtim_min == 0 && s->s_xtim_max == 0 )
      s->s_xtim_min = s->s_xtim_max = timeval;
   else if ( timeval < s->s_xtim_min )
      s->s_xtim_min = timeval;
   else if ( timeval > s->s_xtim_max )
      s->s_xtim_max = timeval;

   s->s_xtim_inc += timeval;
}



/***************************************************************************
 * Traphandler f�r Zeitmessungen inklusive Unterroutinen
 *
 *   Parameter f�r alle Traphandler:
 *
 *      adr      = Adresse des ausl�senden Breakpoints;
 *      tadr      = Adresse des R�cksprungs
 *      stackval = Adresse des usp
 */

BOOL TH_HandleFixTrapI( ULONG adr, ULONG tadr, ULONG stackval )
{
   struct Symbol *s= Break2Symbol( adr );

   // Bearbeitung bei aktiviertem 'Safe Profiling'
   if ( IsSafe && !IsCodeHit( tadr ) )
   {
      // HitCount des Symbols auf -1
      s->s_xcount = -1;

      // Ausl�senden Breakpoint l�schen
      *(UWORD *)adr = s->s_break->fix.bp_save;

      // Nicht auf Trace schalten!
      return FALSE;
   }

   s->s_xcount++;                                  // Ansprungsz�hler f�r aktuelles Symbol erh�hen
   *(UWORD *)adr =  s->s_break->fix.bp_save;       // Breakpoint l�schen

   // Wurde Funktion mit jmp aufgerufen?
   if ( lstack == stackval )
   {
      bpidx--;

      SetSymbolITime( breakstack[bpidx].temp.bp_symbol,
         StopTimeMarkCIA( breakstack[bpidx].temp.bp_time ) );

      breakstack[bpidx].temp.bp_symbol = s;        // Symbol speichern
   }
   else
   {
      breakstack[bpidx].temp.bp_symbol = s;        // Symbol speichern
      breakstack[bpidx].temp.bp_cadr = tadr;       // TempBreak setzen
      breakstack[bpidx].temp.bp_save = *(UWORD *)tadr;
      *(UWORD *)tadr = trap_tmp_cmd;
   }

   breakstack[bpidx].temp.bp_time = StartTimeMarkCIA();

   lstack = stackval;                              // Wert des SlaveStack speichern
   bpidx++;                                        // Stackindex aktualisieren

   return TRUE;
}



void TH_HandleTmpTrapI( ULONG adr )
{
   struct Symbol *s;

   lstack = 0;                                     // Jumpkennung l�schen

   do   {
      bpidx--;                                     // Stackindex aktualisieren

      SetSymbolITime( breakstack[bpidx].temp.bp_symbol,
         StopTimeMarkCIA( breakstack[bpidx].temp.bp_time ) );

      // Breakpoint l�schen
      *(UWORD *)breakstack[bpidx].temp.bp_cadr = breakstack[bpidx].temp.bp_save;

   } while ( breakstack[bpidx].temp.bp_cadr != adr );
   // Armer Compiler...
}


/***************************************************************************
 * Traphandler f�r Zeitmessungen exklusive Unterroutinen
 */

enum ExecDir { UP, DOWN };

static void SetSymbolETime( union BreakPoint *bp, ULONG rawtime, enum ExecDir ed )
{
   static ULONG ltime;
   register ULONG timeval;

   if ( 0 == ltime )
      ltime = rawtime;
   else
   {
      // Aktuelle Ausf�hrungszeit
      bp->temp.bp_time += ltime - rawtime;

      if ( DOWN == ed )
      {
         register struct Symbol *s = bp->temp.bp_symbol;

         timeval = bp->temp.bp_time;

         // Zeitfelder aktualisieren ***************************
         s->s_xtim_inc += timeval;                          // *
                                                            // *
         if ( s->s_xtim_min == 0 && s->s_xtim_max == 0 )    // *
            s->s_xtim_min = s->s_xtim_max = timeval;        // *
         else if ( timeval < s->s_xtim_min )                // *
            s->s_xtim_min = timeval;                        // *
         else if ( timeval > s->s_xtim_max )                // *
            s->s_xtim_max = timeval;                        // *
         // ****************************************************
      }

      // Zeitpunkt der Messung sichern
      ltime = rawtime;
   }
}



BOOL TH_HandleFixTrapE( ULONG adr, ULONG tadr, ULONG stackval )
{
   register struct Symbol *s = Break2Symbol( adr );

   // Bearbaitung bei aktiviertem 'Safe Profiling'
   if ( IsSafe && !IsCodeHit( tadr ) )
   {
      // HitCount des Symbols auf -1
      s->s_xcount = -1;
      // Ausl�senden Breakpoint l�schen
      *(UWORD *)adr = s->s_break->fix.bp_save;
      // Nicht in Trace Modus gehen
      return FALSE;
   }

   s->s_xcount++;                                  // Ansprungsz�hler f�r aktuelles Symbol erh�hen
   *(UWORD *)adr = s->s_break->fix.bp_save;        // Breakpoint l�schen

   // Zeitwerte f�r Funktion auf Stack aktualisieren
   SetSymbolETime( &breakstack[bpidx-1], RawTimeCIA(), UP );

   // Wurde Funktion mit jmp aufgerufen ?
   if ( lstack == stackval )                       // TOS-Symbol wechseln
   {
      SetSymbolETime( &breakstack[--bpidx], RawTimeCIA(), DOWN );
      breakstack[bpidx].temp.bp_time = 0;          // Zwischenzeit init
      breakstack[bpidx].temp.bp_symbol = s;        // Symbol speichern
   }
   else                                            // Neues Symbol TOS
   {
      breakstack[bpidx].temp.bp_symbol = s;        // Symbol speichern
      breakstack[bpidx].temp.bp_time = 0;          // Zwischenzeit init
      breakstack[bpidx].temp.bp_cadr = tadr;       // TempBreak setzen
      breakstack[bpidx].temp.bp_save = *(UWORD *)tadr;
      *(UWORD *)tadr = trap_tmp_cmd;
   }

   lstack = stackval;                              // Wert des Stack speichern
   bpidx++;                                        // Stackindex aktualisieren

   return TRUE;
}



void TH_HandleTmpTrapE( ULONG adr )
{
   struct Symbol *s;

   lstack = 0;                                     // Jumpkennung l�schen

   do   {
      bpidx--;                                     // Stackindex aktualisieren
                                                   // Breakpoint l�schen
      *(UWORD *)breakstack[bpidx].temp.bp_cadr = breakstack[bpidx].temp.bp_save;

      SetSymbolETime( &breakstack[bpidx], RawTimeCIA(), DOWN );

   } while ( breakstack[bpidx].temp.bp_cadr != adr );
}



/* TH_ClearStack - initialisiert den breakstack
 *
 * R�ckgabe: (BOOL) TRUE, wenn Stack nicht leer war, sonst FALSE
 */

BOOL TH_ClearStack( void )
{
   BOOL ret = bpidx ? TRUE : FALSE;
   bpidx = 0;
   return ret;
}



/* Funktionen zum Setzen und Abfragen des IsSafe-Flags
 */

BOOL SetSaveProf( BOOL neu )
{
   BOOL ret = IsSafe;
   IsSafe = neu;
   return ret;
}




BOOL GetSaveProf( void )
{
   return IsSafe;
}



char *GetSafeProfToolType( void )
{
   return "SAFE";
}
