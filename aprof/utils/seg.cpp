/* seg.cpp - Anzeige einer Segmentliste
 *
 * Projekt: pro3
 *
 * $Date: 2002/05/31 23:15:59 $ 
 * $Id: seg.cpp,v 1.1.1.1 2002/05/31 23:15:59 michab66 Exp $ 
 * $Revision: 1.1.1.1 $
 *
 * © 1993,94 Michael G. Binz
 */


#include <iostream.h>
#include <pragmas/dos_lib.h>



// Kein zurück...

void usage()
{
   cerr << "Usage: seg exe ..." << endl;
   
   exit( 10 );
}



inline ULONG HunkSize( BPTR hunkbase )
{
   ULONG *hb = (ULONG *)BADDR( hunkbase );
   
   return *(hb-1) -8;
}



inline BPTR HunkNext( BPTR hunkbase )
{
   return *(BPTR *)BADDR( hunkbase );
}

   

inline ULONG HunkBase( BPTR hunkbase )
{
   ULONG *hb = (ULONG *)BADDR( hunkbase );

   return (ULONG)(hb+1);
}



int main( int argc, char *argv[] )
{
   if ( argc <= 1 )
      usage();
      
   for ( int i = 1 ; i < argc ; i++ )
   {
      BPTR seglist;
      
      if ( seglist = LoadSeg( argv[i] ) )
      {
         int count = 0;
         BPTR tmplist = seglist;
         
         cout << "Segment list of <" << argv[i] << ">" << endl;
         
         do {
            cout << count++ << " : " << int( HunkSize( tmplist ) ) << endl;   
         } while ( tmplist = HunkNext( tmplist ) );
         
         UnLoadSeg( seglist );
      }
      else
         cerr << "Can't open file <" << argv[i] << ">" << endl;
   }
   
   return 0;
}
