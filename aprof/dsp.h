/* $RCSfile: dsp.h,v $
 * 
 * Interface description for the display module.
 *
 * $Revision: 1.2 $ $Date: 2008/04/10 18:41:21 $
 *
 * Copyright © 1993,94 Michael G. Binz
 */

BOOL DspInit( int x, int y, int w, int h, struct NewMenu *nm, void (* idcmpfunc)( struct IntuiMessage * ) );
void DspStart( void );
void DspExit( void );

void DspPutText( char *text[] );
APTR DspGetVisInfo( void );
int DspGetCurrentLine( void );
void DspMarkText( int lin, int col, int len );
void DspMessage( char *msg );
void DspSetWindowTitle( char *strg );
void DspWindow2Front( void );
void DspWindow2Back( void );
void DspBlockWin( BOOL w );
struct Window *DspGetWindow( void );
void DspPgUp( void );
void DspPgDown( void );
void DspLineUp( void );
void DspLineDown( void );
void DspRefreshWin( void );
void DspTop( void );
void DspBottom( void );
void DspSetTopOffset( int );
struct TextAttr *DspGetFont( void );
BOOL DspIsLineVisible( int );
char *GetDspToolType( void );
char *GetDspToolSize( void );
