/*
 * $RCSfile: p3break.c,v $
 *
 * Behandlung von Breakpoints
 *
 * $Revision: 1.1.1.1 $ $Date: 2002/05/31 23:15:37 $
 * 
 * © 1993,94 Michael G. Binz
 */

#include <stdlib.h>
#include <string.h>

#include <exec/tasks.h>
#include <clib/exec_protos.h>
#include <dos/dos.h>
#include <pragmas/exec_lib.h>

#include "pro.h"
#include "timer.h"


APTR OldTrap;

static LONG      local_trap_tmp = -1;   // Trap #
static LONG      local_trap_fix = -1;   // Trap #

UWORD            trap_fix_cmd;      // Assemblercode für Kommando
UWORD            trap_tmp_cmd;      // Assemblercode für Kommando

#define TRAP_FRAME   0x4e40

static union BreakPoint *breaktable[HASHSIZE];



// Entfernt alle in breaktable eingehängten Daten aus dem Speicher 
static void BreakClear( void )
{
   unsigned i;

   for ( i = 0 ; i < HASHSIZE ; i++ )
   {
      union BreakPoint *b1, *b2;

      for ( b1 = breaktable[i] ; b1 ; b1 = b2 )
      {
         b2 = b1->fix.bp_next;
         free( b1 );
      }

      breaktable[i] = NULL;
   }

#ifdef HASHINFO
   {
      int used = 0, two = 0, three = 0, four = 0, max = 0, i;

      for ( i = 0 ; i < HASHSIZE ; i++ )
      {
         union BreakPoint *bp = breaktable[ i ];
         int len;

         if ( bp )
         {
            used++;

            for ( len = 0 ; bp ; bp = bp->fix.bp_next )
               len++;

            switch ( len )
            {
               case 2:  two++;   break;
               case 3:  three++; break;
               case 4:  four++;  break;
            }

            if ( len > max )
               max = len;
         }
      }

      p3err( "Size: %d  Used: %d  2: %d  3: %d  4: %d  Max: %d"
         HASHSIZE, used, two, three, four, max );
   }      
#endif
}



static void FreeTraps( void )
{
   struct Task *t = FindTask( NULL );

   if ( local_trap_tmp != -1 )
      FreeTrap( local_trap_tmp );

   if ( local_trap_fix != -1 )
      FreeTrap( local_trap_fix );

   Disable();   
   if ( ProfilerTrapHandler == t->tc_TrapCode )
      t->tc_TrapCode = OldTrap;
   Enable();
}



// Init Funktion
static BOOL SetTraps( void )
{
   struct Task *t = FindTask( NULL );
   
   atexit( FreeTraps );

   if ( -1 == (local_trap_tmp = AllocTrap( -1 )) )
      return FALSE;
   else if ( -1 == (local_trap_fix = AllocTrap( -1 )) )
      return FALSE;

   trap_fix_cmd = TRAP_FRAME | local_trap_fix;
   trap_tmp_cmd = TRAP_FRAME | local_trap_tmp;
   
   // Übergabe der allokierten Traps an p3trap.asm
   InitTrapHandler( local_trap_fix, local_trap_tmp );
   
   // Neuen TrapHandler aktivieren
   Disable();
      OldTrap = t->tc_TrapCode;
      t->tc_TrapCode = ProfilerTrapHandler;
   Enable();

   return TRUE;
}



#define HashValue( adr )   (((adr)>>1)%HASHSIZE)



static union BreakPoint *BreakCreate( ULONG adr )
{
   unsigned idx;
   union BreakPoint *bp;

   // reservieren
   if ( NULL == (bp = (union BreakPoint *)malloc( sizeof( union BreakPoint ) )) )
      return NULL;

   // einhaengen
   idx = HashValue( adr );

   bp->fix.bp_next = breaktable[idx];
   breaktable[idx] = bp;

   return bp;
}



/* CheckEntryBreak
 *
 * Sucht nach Breakpoint an Programmeintritt. Wenn keiner vorhanden
 * ist, wird Symbol '*ENTRY*' generiert und in die Symbolliste 
 * eingehängt.
 * Am Programmeintritt muß ein Symbol definiert sein, damit beim
 * Verlassen des Profilee mit exit() oder einem anderen nicht-lokalen
 * Sprung der Stack abgeräumt wird und nicht der Fall auftritt, daß eine
 * Funktion zwar einen Hitcount von 1 hat, aber keine Ausführungszeit
 * bestimmt wurde.
 */
static struct Symbol *CheckEntryBreak( struct Symbol *s, BPTR seg )
{
   const char *entnm = "*ENTRY*";

   int count;
   struct Symbol *cs;

   /* Berechnung des Programmeinsprungs */
   ULONG entry_adr = (ULONG)BADDR( seg + 1 );

   /* Suchen eines Breakpoints für Programmeinsprung */
   for ( cs = s, count = 0 ; cs ; cs = cs->s_next )
   {
      if ( cs->s_cadr == entry_adr && !(cs->s_flags & S_HIDE) )
         count++;
   }


   /* count muß im Bereich 0 <= count <= 2 liegen.
    *
    * Bei count == 0 muß *ENTRY* eingefügt werden,
    * bei count == 1 ist nichts zu tun,
    * bei count == 2 wird *ENTRY* entfernt
    */
   if ( count == 0 )
   {
      cs = (struct Symbol *)malloc( sizeof( struct Symbol ) + strlen( entnm ) );

      if ( cs )
      {
         /* Initialisieren */
         memset( cs, '\0', sizeof( *cs ) );
         strcpy( cs->s_name, entnm );
         cs->s_cadr = entry_adr;
         cs->s_flags |= S_NHIDE;
         /* Verketten */
         cs->s_next = s;
         THE_BASE.pb_symlist = cs;

         p3err( "Symbol *ENTRY* generated" );
         count = 1;
      }
   }
   else if ( count == 2 )
   {
      if ( !strcmp( (char *)s->s_name, entnm ) )
      {
         THE_BASE.pb_symlist = s->s_next;
         free( s );

         p3err( "Symbol *ENTRY* removed" );
         count = 1;
      }
   }

   if ( count < 1 )
      p3err( "Warning: no entry symbol generated" );
   else if ( count > 1 )
      p3err( "Warning: several program entrys" );

   return THE_BASE.pb_symlist;      
}



BOOL BreakSet( void )
{
   static BOOL cleanup = FALSE;
   struct Symbol *sy;

   UpdateSymlist();

   if ( !cleanup )
   {
      cleanup = TRUE;
      atexit( BreakClear );
      if ( FALSE == SetTraps() )
      {
         p3err( "Can't allocate traps" );
         return FALSE;
      }
   }
   else
      BreakClear();

   /* Suche nach Breakpoint an Programmeintritt */
   (void)CheckEntryBreak( THE_BASE.pb_symlist, THE_BASE.pb_seglist );

   /* Alle FixTraps setzen */
   for ( sy = THE_BASE.pb_symlist ; sy ; sy = sy->s_next )
   {
      // Symbol nicht bearbeiten?
      if ( sy->s_flags & S_HIDE )
         continue;

      // Eintrag in breaktable erstellen
      if ( NULL == (sy->s_break = BreakCreate( sy->s_cadr )) )
         return FALSE;
      
      // Rückbezug von BreakPoint auf Symbol erstellen
      sy->s_break->fix.bp_symbol = sy;

      // Ursprünglichen Befehl sichern
      sy->s_break->fix.bp_save   = *(USHORT *)sy->s_cadr;
      
      // Breakpoint setzen
      *(USHORT *)sy->s_cadr = trap_fix_cmd;
   }

   // 68040 Unterstützung (I'm only dreaming...) Cache zurückschreiben
   CacheClearU();

   return TRUE;
}



/* Macht aus der Adresse eines BreakPoints das dazugehörige Symbol
 * ACHTUNG: Fehler werden nicht abgefangen
 */
struct Symbol *Break2Symbol( ULONG adr )
{
   union BreakPoint *bp = breaktable[ HashValue( adr ) ];

   // Kollisionsliste durchlaufen
   while ( bp && bp->fix.bp_symbol->s_cadr != adr )
      bp = bp->fix.bp_next;

   return bp ? bp->fix.bp_symbol : NULL;
}



UWORD uwGetTrapCommand( LONG trap )
{
   return TRAP_FRAME | trap;
}
