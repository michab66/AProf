/*************************************************************************
 * sym.c - Utility zum Anzeigen von Symbolhunks
 *
 * Projekt: AProf
 *
 * $Date: 2002/05/31 23:15:59 $
 * $Id: sym.c,v 1.1.1.1 2002/05/31 23:15:59 michab66 Exp $
 * $Revision: 1.1.1.1 $
 *
 * © 1993,94 Michael G. Binz
 */
 
#include <stdio.h>
#include <stdarg.h>
#include "pro.h"

// Versionskennung für AmigaDOS
static char *ver = "$VER: sym 1.2 (" __DATE__ ")";

#if 0
/*
 * Definitionen für Version 2.0
 */

enum SymType {
   SYM_DATA, SYM_CODE, SYM_BSS 
};

struct Symbol {
   char sym_hunk;
   unsigned long sym_adr;
   SymType sym_type;
   char sym_name[256];
};
#endif
   
// Abbruch durch ^C
void _abort( void )
{
   fflush( NULL );
   fprintf( stderr, "***BREAK\n" );
   exit( 10 );
}



// Neue Fehlerfunktion - proerror sollte nicht mehr verwendet werden
void p3err( const char *fmt, ... )
{
   va_list args;

   va_start( args, fmt );
   vprintf( fmt, args );
   va_end( args );
}



int main( int argc, char *argv[] )
{
   struct SymInfo *si;

   if ( argc <= 1 )
      printf( "Usage: sym exe ...\n" );
   else
   {
      for ( argc = 1 ; argv[argc] ; argc++ )
      {
         FILE *f = fopen( argv[argc], "rb" );

         if ( NULL == f )
            printf( "Can't open (%s)\n", argv[argc] );
         else
         {
            ULONG count = 0;
            BOOL Symbol_found = FALSE;

#ifdef __MAXON__
            setvbuf( f, NULL, _IOFBF, 4096 );
            setvbuf( stdout, NULL, _IOLBF, 1024 );
#endif

            printf( "Symbols contained in (%s):\n", argv[argc] );
            while( si = GetSymbol( f ) )
            {
               Symbol_found = TRUE;

               printf( "%5d\t(%3d / %8x)\t%8x  %s\n", ++count, si->si_hunk, si->si_hunksize, si->si_radr, si->si_name );

               if ( si->si_radr >= si->si_hunksize )
                  printf( "\tSymbol out of hunk bounds\n" );
            }

            if ( !Symbol_found )
               printf( "\tNo symbols found\n" );
         }
      }
   }

   return 0;
}

