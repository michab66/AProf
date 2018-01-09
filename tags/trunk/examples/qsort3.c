/* qsort2.c
 *
 * AProf example code
 *
 */


/* Size of sort array */
#define ASIZE  50

/* Definition of sort array */
int array[ ASIZE ];



/* swap
 *
 * swap the values at index positions x1 and x2 in array
 *
 * Optimized XOR swap
 */
void swap( int x1, int x2 )
{
   array[ x1 ] ^= array[ x2 ];
   array[ x2 ] ^= array[ x1 ];
   array[ x1 ] ^= array[ x2 ];
}



/* getPivot
 *
 * Return the index of the pivot element in range x1, x2
 *
 * 3 median
 */
int getPivot( int x1, int x2 )
{
   int mid = (x2 - x1) >> 1;

   if ( array[ x1 ] < array[ mid ] )
   {
      if ( array[ mid ] < array[ x2 ] )
         return mid;
      else
      {
         if ( array[ x1 ] > array[ x2 ] )
            return x1;
         else
            return x2;
      }
   }
   else
   {
      if ( array[ x1 ] < array[ x2 ] )
         return x1;
      else
      {
         if ( array[ mid ] > array[ x2 ] )
            return mid;
         else
            return x2;
      }
   }
}



/* doSplit
 * 
 * Gnah...
 */
int doSplit( int x1, int x2, int pivot )
{
   do {
      while ( array[x1] < array[pivot] )
         x1++;

      while ( array[x2] > array[pivot] )
         x2--;

      if ( x1 < x2 )
      {
         swap( x1, x2 );

         if ( x1 == pivot )
            pivot = x2;
         else if ( x2 == pivot )
            pivot = x1;
      }
   } while ( x1 < pivot || x2 > pivot );

   return pivot;
}



/* theQsort
 *
 * Quicksorts array between indices x1, x2
 */
void theQsort( int x1, int x2 )
{
   if ( x1 < x2 )
   {
      /* pivot element */
      int pivot = getPivot( x1, x2 );

      pivot = doSplit( x1, x2, pivot );

      theQsort( x1, pivot-1 );
      theQsort( pivot+1, x2 );
   }
}



int main( void )
{
   int i;

   /* Initialize sort array */
   for ( i = 0 ; i < ASIZE ; i++ )
      array[i] = rand();

   theQsort( 0, ASIZE-1 );

   exit( 0 );
}   
