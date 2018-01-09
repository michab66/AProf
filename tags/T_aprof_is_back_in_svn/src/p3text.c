/*
 * $RCSfile: p3text.c,v $
 *
 * Textklasse für AProf
 *
 * $Revision: 1.1.1.1 $ $Date: 2002/05/31 23:15:49 $
 *
 * © 1993,94 Michael G. Binz
 */

#include <stdio.h>
#include <stddef.h> /* offsetof */
#include <stdlib.h>
#include <string.h>

#include "p3text.h"

const int TabGrow = 50;
const int BufGrow = 4096;


/*
 * Definition der Textstruktur, wird nur intern verwendet
 */
typedef struct {
   short  error;       // Wenn != 0 => Fehler aufgetreten
   size_t sizTab;      // Allokierte Tabellengröße
   int    posTab;      // Nächste zu füllende Position in Tabelle
   size_t sizBuf;      // Allokierte Puffergröße
   size_t posBuf;      // Ziel für nächste Kopieraktion
   char  *Buffer;      // Allokierter Puffer
   char  *Table[1];    // Beginn der Tabelle
} Text;

/*
 * Umwandlung des Memberpointer c in Text Pointer
 */
#define Carr2Text( c )   ((Text *)((size_t)c - offsetof( Text, Table )))



/* short resizeBuffer( Text * )
 *
 * Interne Funktion. Vergrössert den zur Tabelle gehörenden Textpuffer
 * und gleicht Pointeroffsets an.
 */
static short resizeBuffer( Text *txt )
{
   char *newb = (char *)realloc( txt->Buffer, txt->sizBuf + BufGrow );

   if ( newb )
   {
      int i;
      long delta = (size_t)txt->Buffer - (size_t)newb;

      // Eintrag für Puffer aktualisieren
      txt->Buffer = newb;
      txt->sizBuf += BufGrow;

      // Pointeroffsets angleichen
      for ( i = 0 ; i < txt->posTab ; i++ )
         txt->Table[i] -= delta;
   }
   else
      txt->error  = 1;

   return txt->error;
}



/* short resizeTable( Text * )
 *
 * Interne Funktion. Vergrössert die Anzahl der Tabelleneinträge
 */
static Text *resizeTable( Text *txt )
{
   Text *newText = (Text *)realloc( txt,
      sizeof( Text ) + (txt->sizTab + TabGrow) * sizeof( char * ) );

   if ( newText )
   {
      newText->sizTab += TabGrow;
      txt = newText;
   }
   else
      txt->error = 2;

   return txt;
}



/* TextObj TxtAlloc( void )
 *
 * Anlegen und Initialisieren einer Texttabelle
 */
TextObj TxtAlloc( void )
{
   // Textstruktur anlegen
   Text *txt = (Text *)malloc( sizeof( Text ) + TabGrow * sizeof( char * ) );

   if ( txt )
   {
      txt->error  = 0;
      txt->sizTab = TabGrow;
      txt->posTab = 0;
      txt->sizBuf = 0;
      txt->posBuf = 0;
      txt->Buffer = NULL;

      return txt->Table;
   }

   return NULL;
}



/* void TxtFree( TextObj  )
 *
 * Gibt durch Text belegten Speicher wieder frei
 */
void TxtFree( TextObj carr )
{
   if ( carr )
   {
      Text *txt = Carr2Text( carr );

      // Puffer freigeben
      if ( txt->Buffer )
         free( txt->Buffer );

      // Textstruktur freigeben
      free( txt );
   }
}



/* TextObj TxtClear( TextObj  )
 *
 * Leeren einer Tabelle
 */
TextObj TxtClear( TextObj carr )
{
   if ( carr )
   {
      Text *txt = Carr2Text( carr );

      txt->error  = 0;
      txt->posTab = 0;
      txt->posBuf = 0;
      return txt->Table;
   }
   else
      return NULL;
}



/* TextObj TxtPutLine( TextObj , char * )
 *
 * Fügt eine Zeile an Tabelle an
 */
TextObj TxtPutLine( TextObj carr, char *line )
{
   int len = line ? strlen( line ) + 1 : 0;

   if ( carr )
   {
      Text *txt = Carr2Text( carr );

      // Sofort zurück, wenn bereits Fehler passiert
      if ( txt->error )
         return carr;

      // Wenn Text länger als freier Puffer - vergrößern
      if ( ( txt->sizBuf - txt->posBuf ) < len && resizeBuffer( txt ) )
         // Kein Speicher
         return carr;

      // Wenn Tabelle voll - vergrößern
      if ( txt->sizTab == txt->posTab )
      {
         Text *newText = resizeTable( txt );

         if ( newText->error )
            // Kein Speicher
            return carr;
         else
            txt = newText;
      }

      // Zeiger in Tabelle eintragen
      txt->Table[ (txt->posTab)++ ] =
         line ? &(txt->Buffer[ txt->posBuf ]) : NULL;

      // Neue Zeile in Speicher schreiben
      if ( len )
      {
         strcpy( &(txt->Buffer[ txt->posBuf ]), line );
         // Zeiger auf neues Pufferende stellen
         txt->posBuf += len;
      }

      carr = txt->Table;
   }

   return carr;
}



/* short TxtError( TextObj  )
 *
 * Prädikat, zeigt an ob bei Textbehandlung ein Fehler passiert ist
 */
short TxtError( TextObj carr )
{
   if ( carr )
   {
      Text *txt = Carr2Text( carr );
      return txt->error;
   }
   else
      return 1;
}



/* int TxtGetLineNum( TextObj  )
 *
 * Anzahl der Zeilen im Text
 */
int TxtGetLineNum( TextObj carr )
{
   if ( carr )
   {
      Text *txt = Carr2Text( carr );
      return txt->posTab;
   }
   else
      return -1;
}



#ifdef P3_TEXT_TEST

int main( int argc, char *argv[] )
{
   int i, j;
   TextObj t;
   char buffer[200];
   int reload;
   FILE *in;

   t = TxtAlloc();

   if ( argc < 2 || argc > 3 )
   {
      printf( "Benötige Arg\n" );
      exit ( 1 );
   }

   if ( argc == 3 )
      reload = atoi( argv[2] );
   else
      reload = 1;

   for ( j = 0 ; j < reload ; j++ )
   {
      if ( in = fopen( argv[1], "r" ) )
      {
         for (;;)
         {
            fgets( buffer, 200, in );
            if ( feof( in ) )
               break;
            else
               t = TxtPutLine( t, buffer );
         }
         fclose( in );
      }
      else
      {
         printf( "Kann Datei nicht öffnen\n" );
         exit( 1 );
      }

      if ( TxtError( t ) )
      {
         printf( "TxtError motzt\n" );
         exit( 1 );
      }

      for ( i = 0 ; i < TxtGetLineNum( t ) ; i++ )
         printf( t[i] );

      t = TxtClear( t );
   }

   TxtFree( t );

   return 0;
}

#endif


