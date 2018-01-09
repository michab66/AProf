/* $RCSfile: cplus.cpp,v $
 *
 * Example program for demonstration of C++ name unmangling
 * in Rexx.
 *
 * Works with Maxon C++
 *
 * $Revision: 1.1 $ $Date: 2002/06/03 20:09:01 $
 */

#include <iostream.h>



void print_int( int x )
{
   for ( int i = 0 ; i < x ; i++ )
      cout << i << endl;
}

void print_float( float x )
{
   for ( float i = 0 ; i < x ; i++ )
      cout << i << endl;
}

void print_short( short x )
{
   for ( short i = 0 ; i < x ; i++ )
      cout << i << endl;
}

int main( int argc, char **argv )
{
   cout << "This is an example C++ program demonstrating" << endl
        << "ARexx unmangling of C++ symbols." << endl
        << "Set 'Rexx unmangler' in Misc/Prefs to 'maxoncpp.aprof'" << endl;

   print_int( 5 );
   print_float( 5 );
   print_short( 5 );

   return 0;
}

