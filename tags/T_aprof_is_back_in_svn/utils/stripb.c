/****************************************************************************
 * StripB - Remove Symbol Hunk
 *
 * Projekt: AProf
 *
 * $Date: 2002/05/31 23:15:59 $
 * $RCSfile: stripb.c,v $
 * $Revision: 1.1.1.1 $
 *
 *  Standalone Programm - entfernt SymbolHunk aus Executable oder Objectfile
 *  - muß Amiga Objectfile sein, nicht Manx
 *
 *  © 1991-94 Binz Software Systems
 *
 ************************************
 *
 *  Versionsdokumentation:
 *
 *  V2.01   : 06.04.94  :   Schreiblesepuffer angelegt fuer Maxon C++
 *
 *  V2.00   : 21.05.93  :   Umbenannt nach StripB, Fehlermeldungen in Englisch,
 *                          Veröffentlicht als Utility zu Pro3
 *
 *  V 1.21  : 13.01.92  :   Bei HUNK_EXT wird jetzt auch SubHunk EXT_SYMB
 *                          verarbeitet. Brauche Informationen für EXT_DEXT..
 *                          SubHunk-Headers!!
 *
 *  V 1.20  : 13.01.92  :   Fehler in Bearbeitung von HUNK_EXT behoben.
 *                          Erforderte größere Aktion - Hält mich von
 *                          Mathe ab!!!.
 *
 *  V 1.10  : 06.01.92  :   Fehler in Bearbeitung von HUNK_BSS behoben
 *
 *  V 1.00  : 27.12.91  :   Erstellt aus Teilen des Moduls getsym.c
 *
 */

#include <exec/types.h>
#include <dos/dos.h>
#include <dos/doshunks.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>


#define APP_NAME        "StripB"
#define APP_VERSION     "2.01"

#define LONG_COPY       putlong( getlong() )


volatile char *version = "$VER: StripB " APP_VERSION " (" 
                         /* $AmigaDATE: */ "11.08.94" /* $ */
                         ")";


char buffer[256];

char *destname;

FILE *source;
FILE *dest;



// No return...
void error( char *fmt, ... )
{
   va_list args;

   va_start( args, fmt );
   vprintf( fmt, args );
   va_end( args );

   if ( dest )
   {
      fclose( dest );
      if ( destname )
         remove( destname );
   }
   if ( source )
      fclose( source );

   exit( RETURN_ERROR );
}



/* ctrl-c Behandlung Aztec C
 *
#ifdef MCH_AMIGA
void _abort( void )
{
    error( "***BREAK" );
}
#endif




/****************************************************************************
 *  doc
 *
 *  Ausgabe einer Informationszeile
 */

void doc( void )
{
   fprintf( stderr, " " APP_NAME " " APP_VERSION "  © 1993,94 Michael Binz\n" );

   error( "Usage: " APP_NAME " <source> <destination>\n"
          "  Removes HUNK_SYMBOL and HUNK_DEBUG from Amiga objectfiles\n" );
}



/****************************************************************************
 *  Lesen des nächsten Langworts des Files
 *
 *  Eingabe: ANSI-FileHandle
 *
 *  Ausgabe: gelesener Wert - Bei Lesefehler oder EOF wird sofort error()
 *           angesprungen
 */

ULONG getlong( void )
{
   ULONG out;

   if ( fread( &out, sizeof( out ), 1, source ) )
      return out;
   else if ( feof( source ) )
      return( EOF );
   else
      error( "Read error" );
}




/****************************************************************************
 *  putlong
 */

ULONG putlong( ULONG val )
{
   if ( fwrite( &val, sizeof( val ), 1, dest ) == 0 )
      error( "Write error" );

   return val;
}




/****************************************************************************
 *  Einlesen der Symboltabelle aus Executable (HUNK_SYMBOL)
 *
 *  Eingabe: Name des Executable
 *
 *  Rückgabe: Fehlercode
 *
 */

void remove_symbols( void )
{
   long found = 0;
   union
   {
      ULONG i;
      unsigned char j[ sizeof( ULONG ) ];
   } b;
   long count;

   for (;;)
   {

// ctrl-c Behandlung Maxon: einschalten
#ifdef __MAXON__
#pragma +
#endif

      switch( b.i = getlong() )
      {
         case HUNK_HEADER:
            putlong( b.i );
            /* Namen überspringen */
            while ( ( b.i = LONG_COPY ) != 0L )
            {
               for ( count = b.i ; count ; count-- )
                  LONG_COPY;
            }

            LONG_COPY;                      /* Größe der Tabelle */
            b.i= LONG_COPY;                 /* F */
            b.i= LONG_COPY - b.i +1;        /*b.i= L-F+1 */

            for ( count = b.i; count ; count-- )
               LONG_COPY;
            break;

         case HUNK_UNIT:
         case HUNK_NAME:
         case HUNK_CODE:
         case HUNK_DATA:
            putlong( b.i );

            b.i= LONG_COPY;
            for ( count = b.i ; count ; count-- )
               LONG_COPY;
            break;

         case HUNK_BSS:
            putlong( b.i );
            LONG_COPY;
            break;

         case HUNK_RELOC32:
         case HUNK_RELOC16:
         case HUNK_RELOC8:
            putlong(b.i);

            while( ( b.i = LONG_COPY) != 0L )
            {
               LONG_COPY;                       // Modulnummer
               for ( count = b.i; count ; count-- )
                  LONG_COPY;
            }
                break;

         case HUNK_OVERLAY:
         case HUNK_BREAK:
            error( "Can't handle Overlays\n" );
            break;

         case HUNK_EXT:
            putlong(b.i);

            while( b.i = LONG_COPY )
            {
               switch( b.j[0] )
               {
                  case EXT_SYMB:
                     b.j[0] = 0;
                     while ( b.i = LONG_COPY )
                     {
                        for ( count = b.i; count ; count-- )
                           LONG_COPY;
                        LONG_COPY;
                     }
                     break;

                        case EXT_RES:
                            fprintf( stderr, "Warning: HUNK_EXT/EXT_RES found\n" );
                        case EXT_DEF:
                        case EXT_ABS:
                            b.j[0] = 0;
                            for ( count = b.i ; count ; count-- )
                                LONG_COPY;
                            LONG_COPY;
                            break;

                        case EXT_REF32:
                        case EXT_REF16:
                        case EXT_REF8:
                            b.j[0] = 0;
                            for ( count = b.i ; count ; count-- )
                                LONG_COPY;
                            for ( count = LONG_COPY ; count ; count-- )
                                LONG_COPY;
                            break;

                        case EXT_COMMON:
                            b.j[0] = 0;
                            for ( count = b.i ; count ; count-- )
                                LONG_COPY;
                            LONG_COPY;              // Größe des COMMON-Blocks
                            for ( count = LONG_COPY ; count ; count-- )
                                LONG_COPY;
                            break;

                        case EXT_DEXT32:
                        case EXT_DEXT16:
                        case EXT_DEXT8:
                        default:
                            error( "Unknown subhunk %#lx in HUNK_EXT", b.i );
                    }
                }
                break;

            case HUNK_END:
                putlong(b.i);
                break;

            case HUNK_SYMBOL:               // Löschen
                found = 1;
                while ( b.i = getlong() )
                {
                    for ( count = b.i; count ; count-- )
                        getlong();
                    getlong();
                }
                break;

            case HUNK_DEBUG:                // Löschen
                for ( count = getlong() ; count ; count-- )
                    getlong();
                break;

            default:
                error( "Unknown hunk type %#lx", b.i );
                break;

            case EOF:
                if ( found == 0 )
                    error( "No symbol hunk found" );

                return;  // <====== Ausgang!!
        }

// ctrl-c Behandlung Maxon: ausschalten
#ifdef __MAXON__
#pragma -
#endif

    }
}



int main( int argc, char *argv[] )
{
   if ( argc != 3  )                        // Falsche Argumentenzahl
      doc();

   // Ist destination vorhanden?
   if ( dest = fopen( argv[2], "r" ) )
      error( "Destination file '%s' exists\n", argv[2] );

   destname = argv[2];

   if ( (source = fopen( argv[1], "rb" )) == NULL )
      error( "Can't open source file '%s'\n", argv[1] );

   if ( (dest = fopen( argv[2], "wb" )) == NULL )
      error( "Can't open destination file '%s'\n", argv[2] );

#ifdef __MAXON__
   setvbuf( source, NULL, _IOFBF, 4096 );
   setvbuf( dest, NULL, _IOFBF, 4096 );
#endif

   remove_symbols();

   fclose( dest );
   fclose( source );

   return RETURN_OK;
}


