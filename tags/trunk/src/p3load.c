/*
 * $RCSfile: p3load.c,v $
 *
 * Laderoutinen für Profiler - Compilerunabhängiges Lesen von Symbolhunks
 *
 * $Revision: 1.1.1.1 $ $Date: 2002/05/31 23:15:42 $
 *
 * ©1993,94 Michael G. Binz
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dos/dos.h>
#include <clib/dos_protos.h>

#include <pragmas/dos_lib.h>

#include "dsp.h"
#include "version.h"
#include "pro.h"



static char title[256];
static BOOL CleanUp = FALSE;



/* Freigabe der in ProfileBase eingetragenen Resourcen (atexit)
 */
static void ProFree( void )
{
   if ( THE_BASE.pb_seglist )
   {
      UnLoadSeg( THE_BASE.pb_seglist );
      THE_BASE.pb_seglist = 0;
   }

   if ( THE_BASE.pb_lock )
   {
      UnLock( THE_BASE.pb_lock );
      THE_BASE.pb_lock = 0;
   }

   /* Gibt Symbolliste frei.
    * Nach Ablauf dieser Schleife steht THE_BASE.pb_symlist auf NULL
    */
   if ( THE_BASE.pb_symlist )
   {
      struct Symbol *temp;

      while ( THE_BASE.pb_symlist )
      {
         temp = THE_BASE.pb_symlist->s_next;
         free( THE_BASE.pb_symlist );
         THE_BASE.pb_symlist = temp;
      }
   }

   if ( THE_BASE.pb_comname )
   {
      free( THE_BASE.pb_comname );
      THE_BASE.pb_comname = NULL;
   }

   if ( THE_BASE.pb_comline )
   {
      free( THE_BASE.pb_comline );
      THE_BASE.pb_comline = NULL;
   }
}



// Makros für Hunkzugriffe
#define HunkSize( hunkbase )      (*(((ULONG *)BADDR( hunkbase ))-1)-8)
#define HunkNext( hunkbase )      (*(BPTR *)BADDR( hunkbase ))
#define HunkBase( hunkbase )      ((ULONG)(((ULONG *)BADDR( hunkbase ))+1))



// Berechnet die Adresse des ersten Datenbyte eines Hunk
static ULONG HunkBaseAdr( USHORT hunk )
{
   BPTR chunk = THE_BASE.pb_seglist;

   while ( hunk-- )
      chunk = HunkNext( chunk );

   return HunkBase( chunk );
}



static struct Symbol *MakeSymbol( char *sname )
{
   struct Symbol *s;
   size_t sz;

   sz = sizeof( *s ) + strlen( sname );

   if ( s = (struct Symbol *)malloc( sz ) )
   {
      memset( s, '\0', sz );
      strcpy( s->s_name, sname );
   }

   return s;
}



static void SetExecutableName( char *name )
{
   if ( THE_BASE.pb_comname )
      free( THE_BASE.pb_comname );

   THE_BASE.pb_comname = stralloc( name );

   SetProgramName( name );
}



static BPTR p3LoadSegList( char *exe )
{
   if ( THE_BASE.pb_seglist )
      UnLoadSeg( THE_BASE.pb_seglist );

   return LoadSeg( exe );
}



int SymAdrCmp( const void *va, const void *vb )
{
   Symbol *a = *(Symbol **)va;
   Symbol *b = *(Symbol **)vb;

   return b->s_radr - a->s_radr;
}



static void AdrSortSymList( void )
{
   struct Symbol *Source = THE_BASE.pb_symlist;
   struct Symbol *Snoop, *FSnoop;
   struct Symbol *Hunk;
   struct Symbol *Dest = NULL;

   while ( Source )
   {
      struct Symbol **Sort;
      USHORT hunknum = Source->s_hunk;
      int    symnum  = 0;

      // Ende des aktuellen Hunk in Liste suchen
      for ( Snoop = Source ; Snoop && (Snoop->s_hunk == hunknum) ; Snoop = Snoop->s_next )
         FSnoop = Snoop, ++symnum;

      // Listenteil abhängen
      Hunk = Source;
      Source = Snoop;
      FSnoop->s_next = NULL;

      // Sortierarray allokieren
      if ( Sort = (struct Symbol **)calloc( symnum, sizeof( Sort ) ) )
      {
         int i;

         // Symbole in Array eintragen
         for ( i = 0, Snoop = Hunk ; Snoop ; Snoop = Snoop->s_next )
            Sort[i++] = Snoop;

         // Sortieren
         qsort( Sort, symnum, sizeof( *Sort ), SymAdrCmp );

         // Array wieder in Liste verwandeln
         for ( i = 0 ; i < symnum ; i++ )
         {
            Sort[i]->s_next = Dest;
            Dest = Sort[i];
         }

         // Sortierarray freigeben
         free( Sort );
      }
      else
         p3err( "Can't allocate %d bytes for symbol sort...", symnum * sizeof( Sort ) );
   }

   THE_BASE.pb_symlist = Dest;
}



/* OptSymList
 *
 * Entfernung von Adresskollisionen innerhalb eines Hunk.
 * Symboltabelle muß aufsteigend geordnet sein
 */
static void OptSymList( void )
{
   struct Symbol *src, *good, *bad, *tmp;

   good = bad = NULL;
   src = THE_BASE.pb_symlist;

   /* Rückkehr, falls keine Symbolliste existiert
    */
   if ( !src )
      return;

   /* Aufteilen der Symbolliste (src) in zwei neue
    * Listen (good, bad), wobei good die weiterhin
    * gültigen Symbole und bad Doppeleinträge
    * enthält.
    */
   while ( src )
   {
      if ( src->s_next
           /* Gleiche Adresse und gleiche Hunknummer */
           && src->s_radr == src->s_next->s_radr 
           && src->s_hunk == src->s_next->s_hunk )
      {
         tmp = src->s_next;
         src->s_next = bad;
         bad = src;
         src = tmp;
      }
      else
      {
         tmp = src->s_next;
         src->s_next = good;
         good = src;
         src = tmp;
      }
   }

   /* Good wird die neue Symbolliste, Bad wird freigegeben,
    * wenn Einträge existieren.
    */
   THE_BASE.pb_symlist = good;

   while ( bad )
   {
      tmp = bad->s_next;
      free( bad );
      bad = tmp;
   }
}



/* Reverse auf Symbolliste -> Sortierung nach Hunks/Adressen
 */
static struct Symbol *FlipSymList( struct Symbol *curr )
{
   if ( curr )
   {
      struct Symbol *prev = NULL, *tmp;

      do
      {
         tmp = curr->s_next;
         curr->s_next = prev;
         prev = curr;

      } while ( curr = tmp );

      return prev;
   }
   else
      return NULL;
}



static BOOL CreatSymlist( char *exe )
{
   FILE     *exef;               // executable

   if ( exef = fopen( exe, "rb" ) )
   {
      struct SymInfo *si;

      while ( si = GetSymbol( exef ) )
      {
         struct Symbol  *sy;

         // Außerhalb des Hunks?
         if ( si->si_radr >= si->si_hunksize )
            continue;

         // Symboleintrag erzeugen
         if ( NULL == (sy = MakeSymbol( si->si_name )) )
         {
            p3err( "Not enough memory to create symbol table" );
            ProFree();
            return FALSE;
         }

         // Symboleintrag initialisieren
         sy->s_next = THE_BASE.pb_symlist;
         sy->s_hunk = si->si_hunk;
         THE_BASE.pb_symlist = sy;
         sy->s_radr = si->si_radr;
      }

      fclose( exef );

      if ( THE_BASE.pb_symlist == NULL )
      {
         p3err( "Executable '%s' contains no SymbolHunk", exe );
         return FALSE;
      }

      // Kollisionen entfernen
      AdrSortSymList();
      OptSymList();

      // Liste wenden
      THE_BASE.pb_symlist = FlipSymList( THE_BASE.pb_symlist );
#if 0
      //   DEBUG
      {
         struct Symbol *s = NULL;

         while (THE_BASE.pb_symlist)
         {
            struct Symbol *temp = THE_BASE.pb_symlist;
            THE_BASE.pb_symlist = THE_BASE.pb_symlist->s_next;
            temp->s_next = s;
            s = temp;
         }
         THE_BASE.pb_symlist = s;

         for ( s = THE_BASE.pb_symlist ; s ; s = s->s_next )
            printf( "%d %d %s\n", s->s_hunk, s->s_radr, s->s_name );
      }
#endif

      return TRUE;
   }
   else
   {
      p3err( "File '%s' not found", exe );
      return FALSE;
   }
}



// Gleicht Symbol- und Segmentliste ab - Aufruf von p3exec()
BOOL UpdateSymlist( void )
{
   if ( THE_BASE.pb_seglist = p3LoadSegList( THE_BASE.pb_comname ) )
   {
      struct Symbol *s;

      for ( s = THE_BASE.pb_symlist ; s ; s = s->s_next )
         s->s_cadr = HunkBaseAdr( s->s_hunk ) + s->s_radr;

      return TRUE;
   }
   else
   {
      p3err( "Can't reload '%s'?!", THE_BASE.pb_comname );

      return FALSE;
   }
}



void LoadExecutable( char *exen )
{
   struct Symbol *t;

   // Beim ersten Aufruf automatische Destruktion aktivieren
   if ( !CleanUp )
   {
      CleanUp = TRUE;
      atexit( ProFree );
   }
   // Bei weiteren Aufrufen vorherige Daten freigeben
   else
      ProFree();

   // User soll mitbekommen was abgeht
   p3err( "Loading '%s'...", exen );

   // Symbolliste erzeugen
   if ( !CreatSymlist( exen ) )
      return;

   // Lock auf executable erzeugen
   THE_BASE.pb_lock = Lock( exen, ACCESS_READ );

   // Programmname speichern
   SetExecutableName( exen );
   THE_BASE.pb_base_time = 0;

   // Anzeige über dsp
   ShowSymbolList();
   sprintf( title, APP_NAME ": %s", exen );
   DspSetWindowTitle( title );
   DspWindow2Front();
}




/****************************************************************************
 *  Funktion app_open   [MENU] File/Open
 *
 *  Wird direkt aus der IDCMP Hauptschleife von stx angesprungen. Fordert
 *  Dateinamen von Benutzer an, liest die Datei ein und stellt sie dar.
 *
 *  Eingabe: keine
 *  Ausgabe: keine
 *
 */

void men_open( void  )
{
   struct Symbol *sl;
   char *file;

   if ( file = RequestFilenameR() )
      LoadExecutable( file );

   // Gesamtausführungszeit löschen
   UpdateXTime( NULL );
}



// Aufgelaufene Zeitwerte auf 0 setzen (File/reset im Menü)
void men_reset( void )
{
   struct Symbol *s;

   if ( THE_BASE.pb_symlist )
   {
      // Liste rücksetzen
      for ( s = THE_BASE.pb_symlist ; s ; s = s->s_next )
         s->s_xcount = s->s_xtim_inc = s->s_xtim_min = s->s_xtim_max = 0;

      // Gesamtlaufzeit rücksetzen
      THE_BASE.pb_base_time = 0;

      // Anzeige der Gesamtausführungszeit löschen
      UpdateXTime( NULL );

      // Liste mit neuen Werten darstellen
      ShowSymbolList();
   }
   else
      p3err( "No program loaded!" );
}




// TODO: Es sollte binäre Suche verwendet werden!
//       Schliesslich laufen wir im Supervisor Mode

static ULONG SaveTable[256][2];


// Hier wird die Tabelle erzeugt, die das Save Profiling ermöglicht.
ULONG **CreateSaveTable( void )
{
   USHORT MaxHunk = 0, i;
   struct Symbol *sp;

   // Tabelle initialisieren
   memset( SaveTable, 0, sizeof( SaveTable ) );

   // Maximale Hunknummer feststellen
   for ( sp = THE_BASE.pb_symlist ; sp ; sp = sp->s_next )
   {
      if ( sp->s_hunk > MaxHunk )
         MaxHunk = sp->s_hunk;
   }

   // Platz für Endemarkierung
   MaxHunk++;

   // Alle vorkommenden Hunks markieren
   for ( sp = THE_BASE.pb_symlist ; sp ; sp = sp->s_next )
      SaveTable[sp->s_hunk][0] = 1;

   // Adressen aktualisieren
   for ( i = 0 ; i < MaxHunk ; i++ )
   {
      if ( SaveTable[i][0] )
      {
         USHORT ii;
         BPTR seg = THE_BASE.pb_seglist;

         for ( ii = 0 ; ii < i ; ii++ )
            seg = HunkNext( seg );

         SaveTable[i][0] = HunkBase( seg );
         SaveTable[i][1] = HunkSize( seg ) + HunkBase( seg );
      }
   }

   // Optimieren - d.h. Hunks mit Grösse 0 werden entfernt
   for ( i = 0 ; i < MaxHunk ; i++ )
   {
      if ( 0 == SaveTable[i][1] )
      {
         SaveTable[i][0] = SaveTable[MaxHunk-1][0];
         SaveTable[i][1] = SaveTable[MaxHunk-1][1];
         MaxHunk--;
         i--;
      }
   }

   // Endekennung setzen
   SaveTable[MaxHunk][0] = SaveTable[MaxHunk][1] = 0;

   // TODO: Dummy return - sollte entfernt werden
   return NULL;
}



#if 0
void FreeSaveTable( void )
{
   return;
}
#endif



/* Funktion läuft im Supervisor Mode
 */
BOOL IsCodeHit( ULONG ret )
{
   USHORT i;

   // Ungerade?
   if ( ret & 1 )
      return FALSE;

   // Innerhalb eines Codehunks?
   for ( i = 0 ; SaveTable[i][0] ; i++ )
   {
      if ( ret >= SaveTable[i][0] && ret < SaveTable[i][1] )
         return TRUE;
   }

   // Sollte es nötig sein, kann hier noch an der Position ~adr-2
   // nach einem jsr oder jmp gesucht werden.
#if 0

#endif

   return FALSE;
}



