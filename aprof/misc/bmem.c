/* Amiga Universal Profiler
 *
 * Modul: bmem.c
 *
 * Einfache Speicherverwaltung für statische Strings
 *
 * (C) 1991 Binz Software Systems
 */




#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Nur temporär wg. Debugging
#include "pro.h"


/****************************************************************************
 *  Definitionen für Speicherverwaltung
 *
 *  (Sollte in eigenes Modul gelegt werden!)
 */


#define BUFSIZE              256            /* Größe des Lesepuffers */

/* Bei jedem Aufruf von grow() wird outa um GROWSIZE Zeilen vergrößert */
#define GROWSIZE             4
/* Speicherbedarf für grow()-Operation */
#define GROWMEMSIZE          ( GROWSIZE * sizeof( char * ) )
/* Wachstum für allocstr (muß größer als BUFSIZE sein) */
#define STRMEM_SIZE          (20 * BUFSIZE)     /* 5k */

struct memchunk
{
    struct memchunk *next;
    char text[ STRMEM_SIZE ];
};

static char           **outa;            /* Zeilenarray */
static short            outa_grow;       /* Anzahl der Erweiterungen */
static struct memchunk *strmem_root;     /* Erster Speicherbereich */
static struct memchunk *strmem     ;     /* Aktueller Speicherbereich */
static char            *strmem_poi ;     /* Zeiger in aktuellen Speicher */
static int              count;           /* Gesamtzeilen */


/* Protos */
void free_line( void );
void close_all( void );




/*
 *  char *strmemgrow( void )
 *
 *  Belegt neue Speicherblöcke für die Stringverwaltung und verkettet sie mit
 *  bereits bestehenden.
 */

static char *strmemgrow( void )
{
    if ( strmem == NULL )        /* Erster Aufruf */
    {
        if ( (strmem = (struct memchunk *)malloc( sizeof( struct memchunk ) )) == NULL )
		  {
				p3err( "strmemgrow() failed" );
            return( NULL );
		}
        else
            strmem_root = strmem;
    }
    else
    {
        if ( (strmem->next = (struct memchunk *)malloc( sizeof( struct memchunk ) )) == NULL )
		  {
			  p3err( "strmemgrow() failed" );
            return( NULL );
		}
        strmem = strmem->next;
    }

    strmem->next = NULL;                /* Nächstes Speichersegment */
    strmem_poi = strmem->text;

    return( strmem_poi );
}




/****************************************************************************
 *  char *allocstr( char *str )
 *
 *  Trägt neuen String in Speicherblock ein und holt sich gegebenenfalls
 *  einen neuen Speicherblock
 */

static char *allocstr( char *str )
{
    int templen;
    char *ret;

    if ( strmem == NULL )        /* Erster Aufruf */
    {
        if ( strmemgrow() == NULL )
            return( NULL );
    }

    if ( (templen = strlen( str )+1) >= (STRMEM_SIZE - ((unsigned int)strmem_poi-(unsigned int)strmem)) )
    {
        if ( strmemgrow() == NULL )
            return( NULL );
    }

    ret = strmem_poi;
    strcpy( strmem_poi, str );
    strmem_poi += templen;

    return( ret );
}




/****************************************************************************
 *  int grow( void )
 *
 *  Erweitert das auf die einzelnen Strings zeigende Pointerarray
 */

static int grow( void )
{
    char **dpoi;

	p3err( "grow() called" );
	
    if ( outa == NULL )
    {
        dpoi = (char **)malloc( GROWMEMSIZE );
        ++outa_grow;
    }
    else
		dpoi = (char **)realloc( outa, GROWMEMSIZE * ++outa_grow );

	p3err( "leaving grow()" );
	
    if ( dpoi == NULL )
        return( 0 );

    outa = dpoi;

    return( 1 );
}




/****************************************************************************
 * void free_line( void )
 *
 * Gibt gesamten, von grow() und strmemgrow() belegten Speicher wieder frei
 *
 */

void free_line( void )
{
    struct memchunk *temp1, *temp2;
	 
	 p3err( "free_line() called" );

    if ( count )
    {
        temp1 = strmem_root;
        while( temp1 )
        {
            temp2 = temp1->next;
            free( temp1 );
            temp1 = temp2;
        }

        free( outa );

        outa = NULL;
		  strmem_poi = NULL;
		  strmem_root = strmem = NULL;
		  
        count = outa_grow = 0;
    }

    return;
}




/****************************************************************************
 *  Legt String in reservierten Speicherbereichen ab
 *
 */

char **alloc_line( char *line )
{
	if ( count >= (outa_grow * GROWSIZE) )
	{
		if ( grow() == 0 )
			return( NULL );								/* Kein Arrayspeicher mehr */
	}

    outa[count] = allocstr( line );
    if ( outa[count] == NULL )
        return( NULL );                           /* Kein Zeilenspeicher mehr */

    outa[++count] = NULL;                         /* Letzter Zeiger immer NULL */

    return( outa );
}

