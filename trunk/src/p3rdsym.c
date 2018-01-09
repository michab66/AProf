/*
 * $RCSfile: p3rdsym.c,v $
 *
 * Lesen von Amiga Symboltabellen
 *
 * $Revision: 1.1.1.1 $ $Date: 2002/05/31 23:15:46 $
 *
 * © 1993 Michael G. Binz
 */


#include <stdio.h>
#include <stdlib.h>
#include <dos/doshunks.h>
#include "pro.h"


enum ReaderStates { NEED_SYNC, AT_SYMBOL, ERROR_STOP };


static int exestatus;                     // State of current input file

static ULONG *HunkTable;                  // HunkTable read from HUNK_HEADER

static char symbuf[256];                  // name buffer used in si
static struct SymInfo si;                 // returned structure



/* readlong - reads one ULONG from file fil and returns it */

static BOOL getlong( FILE *fil, ULONG *i )
{
   const size_t count = 1;

   if (  feof( fil ) || (count > fread( i, sizeof( *i ), count, fil )) )
      return FALSE;
   else
      return TRUE;
}



static BOOL tosymh( ULONG *hnk, FILE *fil )
{
   ULONG i;
   BOOL bIsCode = FALSE;

   while ( getlong( fil, &i ) )
   {
      switch( i )
      {
         case HUNK_HEADER:
         {
            ULONG j, tabsize;

            // Alle Namen überspringen ( i<<2 == i * sizeof( ULONG )
            for ( getlong( fil, &i ) ; i ; getlong( fil, &i ) )
               fseek( fil, i<<2, SEEK_CUR );

            getlong( fil, &tabsize );               // Größe der Tabelle
            getlong( fil, &i );                     // F
            getlong( fil, &j );                     // L

            if ( tabsize != j - i + 1 )            // i = L-F+1
            {
               p3err( "Overlays are not supported" );
               return FALSE;
            }
            else if ( NULL == (HunkTable = (ULONG *)malloc( tabsize * sizeof( ULONG ) )) )
            {
               // TODO: Fehlermeldung sollte zusätzliche Infos ausgeben
               p3err( "Memory allocation failed" );
               return FALSE;
            }
            else
               fread( HunkTable, sizeof( ULONG ), tabsize, fil );

            break;
         }

         case HUNK_UNIT:
         case HUNK_NAME:
         case HUNK_DEBUG:
            getlong( fil, &i );
            fseek( fil, i<<2, SEEK_CUR );
            break;

         case HUNK_CODE:
         case HUNK_CODE | HUNKF_CHIP:
         case HUNK_CODE | HUNKF_FAST:
         case HUNK_CODE | HUNKF_FAST | HUNKF_CHIP:
            getlong( fil, &i );
            fseek( fil, i<<2, SEEK_CUR );
            bIsCode = TRUE;
            break;

         case HUNK_DATA:
         case HUNK_DATA | HUNKF_CHIP:
         case HUNK_DATA | HUNKF_FAST:
         case HUNK_DATA | HUNKF_FAST | HUNKF_CHIP:
            getlong( fil, &i );
            fseek( fil, i<<2, SEEK_CUR );
            bIsCode = FALSE;
            break;

         case HUNK_BSS:
         case HUNK_BSS | HUNKF_CHIP:
         case HUNK_BSS | HUNKF_FAST:
         case HUNK_BSS | HUNKF_FAST | HUNKF_CHIP:
            getlong( fil, &i );
            bIsCode = FALSE;
            break;

         case HUNK_RELOC32:
         case HUNK_RELOC16:
         case HUNK_RELOC8:
         case HUNK_DREL32:
         case HUNK_DREL16:
         case HUNK_DREL8:
         {
            ULONG dummy;

            for ( getlong( fil, &i ) ; i ; getlong( fil, &i ) )
            {
               getlong( fil, &dummy );                     // Modulnummer
               fseek( fil, i<<2, SEEK_CUR );
            }
            break;
         }

         case HUNK_END:
            (*hnk)++;                        // Ende des Hunks
            break;

         case HUNK_SYMBOL:
            if ( bIsCode )
               return TRUE;
            else
            {
               for ( getlong( fil, &i ) ; i ; getlong( fil, &i ) )
                  fseek( fil, ++i<<2, SEEK_CUR );
            }
            break;

         default:
            p3err( "Unknown hunk number: <%x>", i );
            return FALSE;
            break;
      }
   }

   exestatus = NEED_SYNC;

   return FALSE;  /* EOF */
}



static struct SymInfo *ReadSymbol( FILE *exe, ULONG size )
{
   symbuf[ size * sizeof( long ) ] = '\0';               // EOS setzen
   fread( symbuf, sizeof( long ), size, exe );            // Symbolname
   fread( &si.si_radr, sizeof( si.si_radr ), 1, exe );   // Symboladresse

   si.si_name = (UBYTE *)symbuf;

   // Hunkgröße eintragen - size wird neu verwendet
   if ( size = HunkTable[si.si_hunk] )
      si.si_hunksize = sizeof( ULONG ) * size;
   else
      si.si_hunksize = 0;

   return &si;
}



/* Interface des Symbolreaders
 *
 * Eingabe:
 *      FILE *exe          Executable dessen Symbolhunks gelesen werden sollen
 *
 * Rückgabe:
 *      struct SymInfo *   Zeiger auf SymbolInformationen
 *                        oder NULL, wenn keine mehr vorhanden oder Fehler
 */
struct SymInfo *GetSymbol( FILE *exe )
{
   ULONG size;

   if ( exestatus == AT_SYMBOL )                           // read next
   {
      getlong( exe, &size );

      if ( size )
         return ReadSymbol( exe, size );
      else if ( tosymh( &si.si_hunk, exe ) )
      {
         getlong( exe, &size );
         return ReadSymbol( exe, size );
      }
   }

   else if ( exestatus == NEED_SYNC )                     // read first
   {
      si.si_hunk = 0;

      if ( tosymh( &si.si_hunk, exe ) )
      {
         exestatus = AT_SYMBOL;
         getlong( exe, &size );
         return ReadSymbol( exe, size );
      }
   }

   exestatus = NEED_SYNC;

   free( HunkTable );
   HunkTable = NULL;

   return NULL;
}



