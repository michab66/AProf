/*
 * $RCSfile: p3symdis.c,v $
 *
 * p3symdis.c - Funktionen zur Anzeige von SymbolListen
 *
 * $Revision: 1.1.1.1 $ $Date: 2002/05/31 23:15:49 $
 *
 * © 1993,94 Michael G. Binz
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <clib/dos_protos.h>
#include <pragmas/dos_lib.h>

#include "dsp.h"
#include "timer.h"
#include "pro.h"

static char buffer[ 256 ];


/*
 * Pattern
 */

static char   *sympatp;            // Zeiger auf Symbolpattern nach ParsePattern()
static char   *sympats;            // Zeiger auf Symbolpattern wie eingegeben



/*
 * Einhängen des neuen Pattern
 */
void SetPattern( char *pat )
{
   char *pp, *ps;

   if ( pat )
   {
      size_t sz = 2 + ( strlen( pat ) << 1 );
      ps = stralloc( pat );
      pp = (char *)malloc( sz );

      if ( ps && pp )
      {
         if ( -1 == ParsePattern( ps, pp, sz ) )
         {
            free( ps );
            free( pp );
            p3err( "Syntax error in pattern '%s'", pat );
            return;
         }
      }
      else
      {
         if ( ps )
            free( ps );
         if ( pp )
            free( pp );
         p3err( "Not enough memory to parse pattern '%s'", pat );
         return;
      }
   }
   else
      pp = ps = NULL;

   /* Wenn wir hier angekommen sind, dann sind pp und ps
    * mit gültigen Werten belegt
    */

   /* Freigabe eines evtl. vorhandenen alten Patterns */
   if ( sympats )         // Eingabepattern
      free( sympats );
   sympats = ps;

   if ( sympatp )         // Parsed Pattern
      free( sympatp );
   sympatp = pp;
}



/* Auslesen des gespeicherten Pattern
 */
char *GetPattern( void )
{
   return sympats;
}



/* Auslesen des ToolType für Speicherung des Pattern
 */
char *GetPatternToolType( void )
{
   return "PATTERN";
}



/* Moduldestruktor
 *
 * Wird nur einmal bei Programmende aufgerufen (atexit)
 */
static void p3symdisExit( void )
{
   if ( sympatp )
      free( sympatp );
   if ( sympats )
      free( sympats );
   if ( THE_BASE.pb_display )
   {
      DspPutText( NULL );
      TxtFree( THE_BASE.pb_display );
      THE_BASE.pb_display = NULL;
   }
}



/*******************
 * Die folgenden Funktionen gehören zum Sortiersystem
 */

// Argumente für p3Cmp, nicht besser möglich
static int srt_idx, srt_len;

static int p3Cmp( char **s1, char **s2 )
{
   return strncmp( &(*s1)[srt_idx], &(*s2)[srt_idx], srt_len );
}

static int p3CmpRev( char **s1, char **s2 )
{
   return -1 * strncmp( &(*s1)[srt_idx], &(*s2)[srt_idx], srt_len );
}


static void p3Sort( int idx, int len, int (* sfunc)( char **, char ** ) )
{
   char **tab = THE_BASE.pb_display;

   if ( tab )
   {
      int i;

      // TextDisplay desaktivieren
      DspPutText( NULL );

      // Zählen der Zeilen
      for ( i = 0 ; tab[i] ;  i++ )
         ;

      // Argumente für p3Sort
      srt_idx = idx, srt_len = len;

      // Sortieren
      qsort( THE_BASE.pb_display, i, sizeof( char * ), 
         (int (*)( const void *, const void * ))sfunc );
#if 0
      p3qsort( (void **)THE_BASE.pb_display, 0, i-1,
         (int (*)(const void *, const void *))sfunc );
#endif
   }
   else
      p3err( "Nothing to sort" );
}



static void SortName( void )
{
   p3Sort( 0, 28, p3Cmp );
}


static void SortCount( void )
{
   p3Sort( 29, 6, p3CmpRev );
}


static void SortAverage( void )
{
   p3Sort( 36, 9, p3CmpRev );
}


static void SortOver( void )
{
   p3Sort( 46, 9, p3CmpRev );
}


/**************************************************************
 * Tabellenwerte & Datendefinitionen für Sortierung
 *
 * Alle Tabellen müssen parallel laufen
 */

static char *sortStr[] = {
   "None",
   "Name",
   "Hit count",
   "Average time",
   "Overall time",
   NULL
};

static void (* SortFuncTable[])( void ) = {
   NULL,
   SortName,
   SortCount,
   SortAverage,
   SortOver
};

// Aktueller Index
static int SortIdx;

/**********
 * Get und Set für Sortierung
 */
void SetSortIdx( UWORD idx )
{
   SortIdx = idx;
}

UWORD GetSortIdx( void )
{
   return SortIdx;
}

char **GetSortStringTable( void )
{
   return sortStr;
}

char *GetSortToolType( void )
{
   return "SORT";
}



/********************
 * Funktionsdefinitionen für Anzeigeeinheit
 */

// Tick2Ms wird von p3timer.c importiert

// prc_time - Umrechnung von TickValue->Prozentualem Anteil an Rechenzeit
static double prc_time( ULONG tv )
{
   if ( tv && THE_BASE.pb_base_time )
      return (double)tv*100.0/(double)THE_BASE.pb_base_time;
   else
      return 0.0;
}




/**************************************************************
 * Tabellen & Datendefinitionen für Anzeigeeinheit
 *
 * Diese beiden Tabellen muessen parallel laufen!
 */

static char *prof_time_units[] = {
   "Percentual",
   "Millisecond",
   NULL
};


// Tabelle der Umwandlungsfunktionen
static double (* TransformTable[])( ULONG ) = {
   prc_time,
   Tick2Ms                     /* p3timer.c */
};


// Aktueller Index
static int TfmIdx;




/*
 * Get und Set der Einheit der Anzeigewerte - Index
 */
void SetEinheitIdx( UWORD idx )
{
   TfmIdx = idx;
}

UWORD GetEinheitIdx( void )
{
   return TfmIdx;
}


// Für Anzeige der SliderGadgets
char **GetEinheitStringTable( void )
{
   return prof_time_units;
}



/* GetEinheitToolType
 *
 * Auslesen des ToolType für die eingestellte Einheit
 */
char *GetEinheitToolType( void )
{
   return "TUNITS";
}



/*************************************************************************
 * ClearSymList - Falls angezeigte Symbolliste vorhanden, dann löschen
 */
static void ClearSymbolList( void )
{
   // Wenn Text in Anzeige...
   if ( THE_BASE.pb_display && TxtGetLineNum( THE_BASE.pb_display ) )
   {
      // Anzeige abschalten
      DspPutText( NULL );
      // Inhalt der Textstruktur freigeben
      THE_BASE.pb_display = TxtClear( THE_BASE.pb_display );
   }
}




/*************************************************************************
 * Default Unmangler - entfernt Unterstrich, wenn vorhanden
 */
static char *StdUnmangle( char *symbol )
{
   if ( *symbol == '_' )
      return symbol+1;
   else
      return symbol;
}




/*************************************************************************
 * ShowSymbolList - Übergibt eine neue Symbolliste an dsp.o
 *
 * Diese Funktion setzt das Flag S_HIDE in den Symbolflags
 */
void ShowSymbolList( void )
{
   static BOOL NeedCleanup = TRUE;
   struct Symbol *sl;
   char **outa = NULL, **temp = NULL;

   // Destruktor definieren
   if ( NeedCleanup )
   {
      // Dieser Zweig wird nur einmal beim Programmstart durchlaufen
      if ( NULL == ( THE_BASE.pb_display = TxtAlloc() ) )
      {
         p3err( "Can't allocate text frame" );
         return;
      }

      atexit( p3symdisExit );
      NeedCleanup = FALSE;
   }
   else
      ClearSymbolList();

   // Neuen Text aus Symbolliste erzeugen
   for ( sl = THE_BASE.pb_symlist ; sl ; sl = sl->s_next )
   {
      char *name;

      // Flag löschen
      sl->s_flags &= ~S_HIDE;

      // Unmangler verwenden, wenn definiert
      if ( (name = sl->s_name) && GetRexxFunc() )
         name = RexxUnmangle( name );
      else
         name = StdUnmangle( name );

      // Pattern kontrollieren, wenn definiert
      if ( !(sl->s_flags & S_NHIDE) && sympatp && MatchPattern( sympatp, name ) )
      {
         sl->s_flags |= S_HIDE;
         continue;
      }

      // Zeile erzeugen
      if ( sl->s_xcount )
      {
         /* Optimierung:
          * - nur noch einfacher Zugriff auf Elemente
          * - Nur noch eine Dereferenzierung von TransformTable
          */
         register ULONG tmp_xcount = sl->s_xcount;
         register ULONG tmp_xtim_inc = sl->s_xtim_inc;
         register double (* tmp_trans)( ULONG ) = TransformTable[TfmIdx];

         // Alle Werte berechnen
         sprintf( buffer, "%-28.28s %6d %9.3lf %9.3lf %9.3lf %9.3lf",
            name,
            tmp_xcount,
            tmp_xcount
               ? tmp_trans( tmp_xtim_inc ) / (double)tmp_xcount
               : (double)0.0,
            tmp_trans( tmp_xtim_inc ),
            tmp_trans( sl->s_xtim_min ),
            tmp_trans( sl->s_xtim_max ) );
      }
      else
      {
         // Hitcount war 0 => nur mit Konstanten erzeugen
         sprintf( buffer, "%-28.28s %6d %9.3lf %9.3lf %9.3lf %9.3lf",
            name,
            0, 0.0, 0.0, 0.0, 0.0 );
      }

      // Zeile abspeichern
      THE_BASE.pb_display = TxtPutLine( THE_BASE.pb_display, buffer );
   }

   // Abschliessende NULL in Liste
   THE_BASE.pb_display = TxtPutLine( THE_BASE.pb_display, NULL );


   /* Text in ProfileBase einhängen und darstellen
    * wenn kein Fehler aufgetreten ist und die Zahl
    * der Zeilen nicht 0 ist
    */
   if ( !TxtError( THE_BASE.pb_display ) && TxtGetLineNum( THE_BASE.pb_display ) -1 )
   {
      // Display sortieren...
      if ( SortIdx )
         (SortFuncTable[SortIdx])();

      // ... und anzeigen
      DspPutText( THE_BASE.pb_display );

      // Anzahl der Symbole anzeigen (-1 wg. NULL am Ende)
      p3err( "%d symbols active", TxtGetLineNum( THE_BASE.pb_display ) -1 );
   }
   else
   {
      DspPutText( NULL );
      p3err( "No Symbols to display" );
   }
}




