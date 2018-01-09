/* $RCSFile$
 *
 * Check environment supplied by AProf
 *
 * $Revision: 1.1.1.1 $ $Date: 2002/05/31 23:15:58 $
 */

#include <exec/exec.h>
#include <stdio.h>



extern struct ExecBase *SysBase;



/* ToBin
 *
 * Create string of binary digits from input
 */
char *ToBin( int bitlen, unsigned long x )
{
   static char buf[ 33 ];

   if ( bitlen <= 0 || bitlen > 32 )
      sprintf( buf, "ToBin bitlen <%lu>", bitlen );
   else
   {
      int i;

      for ( i = bitlen-1 ; i >= 0 ; i-- )
      {
         buf[i] = '0' + (x&1);
         x >>= 1;
      }

      buf[bitlen] = 0;
   }

   return buf;
}



/* CheckStack
 *
 * Display stack information 
 */
void CheckStack( void )
{
   ULONG lo, hi, ct;

   Forbid();
   lo = (ULONG)SysBase->ThisTask->tc_SPLower;
   hi = (ULONG)SysBase->ThisTask->tc_SPUpper;
   ct = (ULONG)SysBase->ThisTask->tc_SPReg;
   Permit();

   printf( "Stack information\n-----------------\n" );
   printf( "Stack lower bound: 0x%lx\n", lo );
   printf( "Stack upper bound: 0x%lx\n", hi );
   printf( "Stack pointer    : 0x%lx\n", ct );
   printf( "Stack size       : %ld\n", hi-lo );
   printf( "Stack free       : %ld\n\n", ct-lo );
}



/* CheckTraps
 *
 * Display trap settings
 */
void CheckTraps( void )
{
   UWORD ta, te;

   Forbid();
   ta = SysBase->ThisTask->tc_TrapAlloc;
   te = SysBase->ThisTask->tc_TrapAble;
   Permit();

   printf( "Trap flags settings\n-------------------\n" );
   printf( "Allocated: <16:%s> = %u\n", ToBin( 16, ta ), ta );
   printf( "Enabled  : <16:%s> = %u\n", ToBin( 16, te ), te );
}



int main( void )
{
   CheckStack();
   CheckTraps();

   return 0;
}
