/*
 * $RCSfile: p3text.h,v $
 *
 * Prototypen für Textfunktionen
 *
 * $Revision: 1.1.1.1 $ $Date: 2002/05/31 23:15:49 $
 *
 * © 1993,94 Michael G. Binz
 */

#ifndef P3TEXT_H
#define P3TEXT_H

// Definition des neuen Typs
typedef char **TextObj;

// Definition der Fehler (TxtError())
#define TXTERR_TABLE_MEM	2
#define TXTERR_BUFFER_MEM	1

TextObj TxtAlloc( void );
void TxtFree( TextObj carr );
TextObj TxtClear( TextObj carr );
TextObj TxtPutLine( TextObj carr, char *line );
short TxtError( TextObj carr );
int TxtGetLineNum( TextObj carr );

#endif
