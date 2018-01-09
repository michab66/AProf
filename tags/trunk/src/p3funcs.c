/*
 * $RCSfile: p3funcs.c,v $
 *
 * Hilfsfunktionen für Profiler
 *
 * $Revision: 1.1.1.1 $ $Date: 2002/05/31 23:15:40 $
 * 
 * © 1992-94 Michael G. Binz
 */


#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <ctype.h>

#include <libraries/asl.h>
#include <dos/dostags.h>
#include <exec/io.h>
#include <devices/inputevent.h>
#include <clib/intuition_protos.h>
#include <clib/asl_protos.h>
#include <clib/exec_protos.h>
#include <clib/gadtools_protos.h>
#include <clib/dos_protos.h>

#include <pragmas/intuition_lib.h>
#include <pragmas/asl_lib.h>
#include <pragmas/exec_lib.h>
#include <pragmas/gadtools_lib.h>
#include <pragmas/dos_lib.h>

#include "version.h"
#include "dsp.h"
#include "pro.h"

#define STX_BUFFER      256     /* Größe für statische Pufferbereiche */

static char buffer[ STX_BUFFER ];
static char fname[ STX_BUFFER ];   /* Puffer für Dateinamen */


#ifdef __MAXON__
/* strnicmp
 *
 * Vergleich zweier Strings bis zur Länge n unabhängig von Gross- und
 * Kleinschreibung
 */
int strnicmp( char *s1, char *s2, int n )
{
   // Zweimal der gleiche String??
   if ( s1 == s2 )
      return 0;

   // Kein Zeiger NULL und n positiv
   if ( s1 && s2 && ( n>0 ) )
   {
      while ( n && *s1 && *s2 && ( toupper( *s1 ) == toupper( *s2 ) ) )
         s1++, s2++, n--;

      if ( !n )
         return 0;

      if ( toupper( *s1 ) > toupper( *s2 ) )
         return 1;
      else if ( toupper ( *s1 ) < toupper( *s2 ) )
         return -1;
      else
         return 0;
   }
   else
      // OK, -2 ist bescheuert, aber was sonst?
      return -2;
}

char *strstri( char *s1, char *s2 )
{
    char *si1, *si2;    // Interne Stringzeiger
    char *buf;          // malloced buffer

    buf = (char *)malloc( strlen( s1 ) + strlen( s2 ) + 2 );    // 2 = 2 * EOS

    si1 = buf;
    si2 = buf + strlen( s1 ) +1;    // +1 = EOS

    strcpy( si1, s1 );  strcpy( si2, s2 );
    strupr( si1 );      strupr( si2 );

    si1 = strstr( si1, si2 );

    if ( si1 )
        si1 = ( s1 + ( si1 - buf ) );

    free( buf );

    return( si1 );
}



char *strins( char *s, int p, char *i )
{
    size_t taill, movel;

    if ( !s || !i || p > strlen( s ) )      // Wenn p oder s NULL oder ...
        return( NULL );                     // Position außerhalb

    taill = strlen( &s[p] ) +1;             // Inkl. EOS
    movel = strlen ( i );                   // Ohne EOS

    memmove( &s[p+movel], &s[p], taill );   // Bereiche überlappen
    memcpy( &s[p], i, movel );

    return( s );
}

#endif


#if 0
// Diese Funktionen wurden wg. einem Bug im Maxon C++ qsort() eingeführt
static void p3swap( void *v[], int i, int j )
{
   void *tmp = v[i];
   v[i] = v[j];
   v[j] = tmp;
}



// QSort der Dicke...

void p3qsort( void *k[], int l, int r,
   int (* cmp)(const void *, const void *) )
{
   int i, j, x, y;
   void *p;

#ifdef DEBUG
   rec++;
   maxrec = MAX( rec, maxrec );

   cout << endl << "Index : " << l << " : " << r << endl;
#endif

   if ( l < r )
   {
      x = i = l;
      y = j = r;

      // Pivot nach 3-Median-Strategie
      if ( (cmp( &k[r], &k[l] ) < 0) && (cmp( &k[r], &k[(l+r)/2] ) > 0) )
         p = k[r];
      else if ( (cmp( &k[l], &k[r] ) < 0) && (cmp( &k[l], &k[(l+r)/2] ) > 0) )
         p = k[l];
      else
         p = k[(l+r)/2];

#ifdef DEBUG
      cout << "Pivot :" << int( p ) << endl;
#endif

      for (;;)
      {
         // Solange von links die Elemente kleiner als Pivot sind, hochzählen
         while ( cmp( &k[i], &p ) < 0 )
            i++;

         // Solange von rechts Elemente größer als Pivot sind, runterzählen
         while ( cmp( &k[j], &p ) > 0 )
            j--;

         // Beide Zeiger auf Pivot?
         if ( i >= j )
            break;

         // i>p && j<p
         if ( cmp( &k[i], &p ) > 0 && cmp( &k[j], &p ) < 0 )
            p3swap( k, i, j );

         // i>p && j==p
         if ( cmp( &k[i], &p ) > 0 && cmp( &k[j], &p ) == 0 )
         {
            p3swap( k, i, j );
            p3swap( k, i, x );
            x++;
         }

         // i==p && j<p
         if ( cmp( &k[i], &p ) == 0 && cmp( &k[j], &p ) < 0 )
         {
            p3swap( k, j, i );
            p3swap( k, j, y );
            y--;
         }

         // i==p==j
         if ( cmp( &k[i], &p ) == 0 && cmp( &k[j], &p ) == 0 )
         {
            p3swap( k, i, x );
            p3swap( k, j, y );
            x++, y--;
         }

         // x,y dürfen nicht kleiner respektive größer als i,j werden
         if ( y < j )
            j = y;
         if ( x > i )
            i = x;

      }

      // Einkopieren der Pivotelemente

      // Falls Elemantanzahl ungerade war, mittleres Element speziell behandeln
      if ( i == j )
      {
         if ( cmp( &k[i], &p ) == 0 )
            j = i + 1;
         else if ( cmp( &k[i], &p ) < 0 )
            j++, i++;
      }
      else
         j = i;

      if ( y < r )
      {
         while ( y < r )
            p3swap( k, ++y, j++ );
      }

      if ( x > l )
      {
         while ( x > l )
            p3swap( k, --x, --i );
      }

#ifdef DEBUG
      cout << "x: " << x << "\ty: " << y << endl;
      cout << "p3qsort( " << l << ", " << i-1 << " )" << endl;
      cout << "p3qsort( " << j << ", " << r << " )" << endl;
#else
      p3qsort( k, l, i-1, cmp );
      p3qsort( k, j, r, cmp );
#endif
   }

#ifdef DEBUG
   rec--;
#endif
}
#endif



/****************************************************************************
 * Table2Idx - Sucht String in einer Tabelle und gibt den Index zurück
 */

UWORD Table2Idx( char **table, char *str )
{
   int i;

   for ( i = 0 ; table[i] ; i++ )
   {
      if ( !strnicmp( str, table[i], strlen( str ) ) )
         return i;
   }

   return 0;
}



/* Str2ToolType - Baut aus einem String einen ToolType (keine Spaces)
 */

char *Str2ToolType( char *str )
{
   int idx;

   buffer[0] = '\0';

   if ( str )
   {
      strcpy( buffer, str );

      if ( idx = strcspn( buffer, " \t\n" ) )
         buffer[idx] = '\0';

      strupr( buffer );
   }

   return buffer;
}



/* RequestFilenameR
 *
 * Dateiname von User anfordern, sollte lesbar sein
 *
 * Eingabe: Keine (In Zukunft vielleicht FilePattern, zus. Tags, Dateiname)
 * Ausgabe: Zeiger auf String (Dateiname) oder NULL
 *
 */
// Fehlt in asl.h
#define FILF_STANDARD   0

char *RequestFilenameR( void )
{
   extern struct FileRequester *AslReq;   // FileRequester Struktur

   if ( AslRequestTags( (APTR)AslReq,
                  // Normaler Filerequest
                  ASL_FuncFlags,   FILF_STANDARD,
                  // Position
                  ASL_LeftEdge,   DspGetWindow()->LeftEdge+DspGetWindow()->BorderLeft,
                  ASL_TopEdge,   DspGetWindow()->TopEdge+DspGetWindow()->BorderTop,

                  TAG_DONE ) )
   {
      if ( *AslReq->rf_Dir )
         strcpy( fname, (char *)AslReq->rf_Dir );
      else
         fname[0] = '\0';

      AddPart( fname, AslReq->rf_File, STX_BUFFER );
   }
   else
      *fname = '\0';

   return (fname && *fname) ? fname : NULL;
}



/**************************************************************************
 * RequestFilenameW - holt Dateiname von User, Datei wird überschrieben
 *
 * Sonst wie RequestFilenameR
 */

char *RequestFilenameW( void )
{
   extern struct FileRequester *AslReq;   // FileRequester Struktur

   if ( AslRequestTags( (APTR)AslReq,
                  // Saverequest!
                  ASL_FuncFlags,   FILF_SAVE,
                  // Position
                  ASL_LeftEdge,   DspGetWindow()->LeftEdge+DspGetWindow()->BorderLeft,
                  ASL_TopEdge,   DspGetWindow()->TopEdge+DspGetWindow()->BorderTop,

                  TAG_DONE ) )
   {
      if ( *AslReq->rf_Dir )
         strcpy( fname, (char *)AslReq->rf_Dir );
      else
         fname[0] = '\0';

      AddPart( fname, AslReq->rf_File, STX_BUFFER );
   }
   else
      *fname = '\0';

   return (fname && *fname) ? fname : NULL;
}



static BOOL save_data( FILE *file )
{
   int wo;
   time_t tim;
   int idx;
   char *str;

   // Zeit initialisieren
   time( &tim );

   // Kopf schreiben

   // Datum
   wo = fprintf( file, "Test date:             %s\n", ctime( &tim ) );
   if ( wo < 0 )
      return FALSE;

   // Dateiname
   wo = fprintf( file, "Execution profile for: %s\n", THE_BASE.pb_comname );
   if ( wo < 0 )
      return FALSE;

   /*
    * Die folgenden Stringkonstanten dürfen keine Tabs enthalten
    */

   // Anzeigeeinheit
   str = (GetEinheitStringTable())[ GetEinheitIdx() ];
   wo = fprintf( file, "Time units:            %s\n", str );
   if ( wo < 0 )
      return FALSE;

   // Sortierreihenfolge
   str = (GetSortStringTable())[ GetSortIdx() ];
   wo = fprintf( file, "Sort order:            %s\n", str );
   if ( wo < 0 )
      return FALSE;

   // Profiler Modus
   str = (GetModeStringTable())[ GetModeIdx() ];
   wo = fprintf( file, "Profiling mode:        %s\n", str );
   if ( wo < 0 )
      return FALSE;

   // Kommandozeile
   wo = fprintf( file, "Used commandline:      %s\n",
   THE_BASE.pb_comline ? THE_BASE.pb_comline : "None" );
   if ( wo < 0 )
      return FALSE;

        // Ausgeschlossene Symbole
   if ( str = GetPattern() )
      wo = fprintf( file, "Symbols not shown:     %s\n", str );
   else
      wo = fprintf( file, "All symbols shown\n" );
   if ( wo < 0 )
      return FALSE;

   // Leerzeile
   wo = fprintf( file, "\n" );
   if ( wo < 0 )
      return FALSE;

   // Symbolliste wegschreiben
   for ( idx = 0 ; THE_BASE.pb_display[idx] ; idx++ )
   {
      wo = fprintf( file, "%s\n", THE_BASE.pb_display[idx] );
      if ( wo < 0 )
         return FALSE;
   }

   // TODO: Dummy-Return
   return TRUE;
}



/* men_save - Sichert Ergebnisse eines Profile-Laufs [Menü] File/Save
 *
 * Eingabe,Ausgabe: keine
 */

void men_save( void )
{
   if ( THE_BASE.pb_comname && THE_BASE.pb_display )
   {
      FILE *outfile;

      // Dateinamen erstellen
      sprintf( buffer, "%s.pro", THE_BASE.pb_comname );

      // Datei öffnen
      if ( outfile = fopen( buffer, "w" ) )
      {
         p3err( "Writing to file '%s'", buffer );

                        if ( FALSE == save_data( outfile ) )
                           p3err( "Write error on file '%s'", buffer );

         fclose( outfile );
      }
      else
         p3err( "Can't open output file `%s'", buffer );
   }
   else
      p3err( "Nothing to save" );
}



void men_save_as( void )
{
   if ( THE_BASE.pb_comname && THE_BASE.pb_display )
   {
      char *filename = RequestFilenameW();

      if ( filename && *filename )
      {
         FILE *outfile = fopen( filename, "w" );

         if ( outfile )
         {
            p3err( "Writing to file '%s'", filename );
            if ( FALSE == save_data( outfile ) )
                                    p3err( "Write error on file '%s'", filename );
            fclose( outfile );
         }
         else
            p3err( "Can't open output file `%s'", filename );
      }
   }
   else
      p3err( "Nothing to save" );
}



void men_print( void )
{
   if ( THE_BASE.pb_comname && THE_BASE.pb_display )
   {
      FILE *the_printer;

      if ( the_printer = fopen( "prt:", "w" ) )
      {
         if ( FALSE == save_data( the_printer ) )
            p3err( "Printer not available" );
         fclose( the_printer );
      }
      else
         p3err( "Printer not available" );
   }
   else
      p3err( "Nothing to save" );
}




/* pro_about - Gibt Versionsmeldung aus ([Menü] File/About)
 *
 * Eingabe: keine
 * Ausgabe; keine
 */

void pro_about( void )
{
   char *version = "$VER: " APP_NAME " " APP_VERSION " ("
                    /* $AmigaDATE: */ "11.08.94" /*  $ */
                    ")";
   struct EasyStruct es;
   char *poi;

   es.es_StructSize = sizeof( es );
   es.es_Flags      = 0;
   es.es_Title      = APP_NAME " Information";
   es.es_TextFormat = "%s\n\n"
                      "© 1993,94 Michael G. Binz\n\n"
                      "EMail: michab@informatik.fh-augsburg.de";
   es.es_GadgetFormat = "Cancel";

   poi = strchr( version, ' ' ) +1;    /* Zeigt auf erste Pos nach $VER ^ */

   EasyRequest( DspGetWindow(), &es, NULL, poi );
}



// [MENU] File/Exit
void app_exit( BOOL *brk )
{
   DspPutText( NULL );            // Textanzeige informieren

   *brk = 0;                     // IDCMP verlassen
}



// [MENU] Misc/Refresh
void app_refresh( void )
{
    DspRefreshWin();
}



char *stralloc( char *str )
{
   char *poi = NULL;

   /*
    * Debug code!!!!
    */
   if ( !str )
      p3err( "Internal error: Called stralloc() with NULL pointer!" );

   if ( poi = (char *)malloc( strlen( str ) + 1 ) )
      strcpy( poi, str );

   return poi;
}





