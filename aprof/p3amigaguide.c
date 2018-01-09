/*
 * $RCSfile: p3amigaguide.c,v $
 *
 * Interface implementation for AmigaGuide
 *
 * $Revision: 1.2 $ $Date: 2008/04/10 18:41:20 $
 *
 * © 1993,94 Michael G. Binz
 */

#include <stdlib.h>
#include <string.h>

#include <libraries/amigaguide.h>
#include <clib/amigaguide_protos.h>
#include <clib/dos_protos.h>
#include <pragmas/amigaguide_lib.h>
#include <pragmas/dos_lib.h>

#include "dsp.h"
#include "pro.h"

// Name of the help file.
#define GUIDE_NAME         "PROGDIR:AProf.guide"


static LONG ShowAmigaGuideFile (STRPTR name, STRPTR node, LONG line)
{
   struct NewAmigaGuide nag;
   AMIGAGUIDECONTEXT handle;
   LONG retval = 0L;

   // Init memory.
   memset( &nag, 0, sizeof( nag ) );

   /* Fill in the NewAmigaGuide structure */
   nag.nag_Name = name;
   nag.nag_Node = node;
   nag.nag_Line = line;

   /* Open the AmigaGuide client */
   if ( handle = OpenAmigaGuideA(&nag, NULL))
   {
      /* Close the AmigaGuide client */
      CloseAmigaGuide(handle);
   }
   else
   {
      /* Get the reason for failure */
      retval = IoErr();
   }

   return retval;
}



void ProvideAProfHelp( char *node, long line )
{
   BPTR l = Lock( GUIDE_NAME, ACCESS_READ );

   if ( l )
   {
      UnLock( l );
      ShowAmigaGuideFile( GUIDE_NAME, node, line );
   }
   else
      p3err( "Helpfile '%s' not found", GUIDE_NAME );
}



void men_help( void )
{
   ProvideAProfHelp( NULL, 0 );
}
