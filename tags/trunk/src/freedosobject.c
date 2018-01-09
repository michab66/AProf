/* $RCSfile: freedosobject.c,v $
 *
 * Wrapper for dos.library function FreeDosObject.
 * Works around an memory leak for object type CLI in dos.lib v<37
 *
 * $Revision: 1.2 $ $Date: 2008/03/25 20:40:24 $
 *
 * Copyright © 1994 Michael G. Binz
 */

#include <exec/libraries.h>
#include <dos/dos.h>
#include <clib/exec_protos.h>
#include <clib/dos_protos.h>
#include <pragmas/exec_lib.h>
#include <pragmas/dos_lib.h>




extern struct Library *DOSBase;



#define FreeBSTR( bstr ) \
   if ( bstr ) \
   { \
      FreeVec( BADDR( bstr ) ); \
      bstr = (BSTR)0L; \
   }




void NewFreeDosObject( ULONG type, void *ptr )
{
   if ( DOSBase->lib_Version < 39 && DOS_CLI == type )
   {
      struct CommandLineInterface *cli =
         (struct CommandLineInterface *)ptr;

      FreeBSTR( cli->cli_SetName );
      FreeBSTR( cli->cli_CommandName );
      FreeBSTR( cli->cli_Prompt );
      FreeBSTR( cli->cli_CommandFile );
   }

   FreeDosObject( type, ptr );
}




#if 0
int main( void )
{
   struct CommandLineInterface *cli;

   if ( cli = (struct CommandLineInterface *)AllocDosObject( DOS_CLI, NULL ) )
      NewFreeDosObject( DOS_CLI, (void *)cli );

   return 0;
}
#endif
