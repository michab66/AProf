/* :ts=3 ********************************************************************
 * $RCSfile: dsp.c,v $
 *
 * General console.device support.
 *
 * $Revision: 1.2 $ $Date: 2008/03/25 20:36:49 $
 *
 * Copyright © 1992-94 Michael G. Binz
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define INTUI_V36_NAMES_ONLY
#include <intuition/intuitionbase.h>
#include <intuition/imageclass.h>
#include <intuition/gadgetclass.h>
#include <intuition/screens.h>
#include <devices/conunit.h>
#include <clib/alib_protos.h>
#include <clib/exec_protos.h>
#include <clib/intuition_protos.h>
#include <clib/alib_protos.h>
#include <clib/gadtools_protos.h>
#include <clib/graphics_protos.h>
#include <pragmas/graphics_lib.h>
#include <pragmas/exec_lib.h>
#include <pragmas/intuition_lib.h>
#include <pragmas/console_lib.h>
#include <pragmas/gadtools_lib.h>

#include "dsp.h"


#define DSP_BUFFER      256         /* Size for internal buffers */

#define STX_MIN_WIDTH   150
#define STX_MIN_HEIGHT   50
#define STX_MAX_WIDTH    ~0
#define STX_MAX_HEIGHT   ~0

static struct
{
   struct Screen   *screen;
   struct Window   *win;
   struct DrawInfo *di;
   struct Gadget   *up, *slider, *dw;
   struct Menu     *menu;
   APTR             vis;
   struct IOStdReq *wreq;
   struct ConUnit  *cun;
   struct MsgPort  *wport;
} dspInfo;

static char       **outarray;                   /* List of output lines */
static int          fst_lin;                    /* Index in outarray */
static int          fst_col;                    /* Index in line */
static int          itemnum;                    /* Number of output lines - y */

#define Lines       (dspInfo.cun->cu_YMax)
#define Columns     (1 + dspInfo.cun->cu_XMax)  /* Number of visible cols */
#define Overlap     1

// Called with unknown messages.
static void (*appIDCMPHandler)( struct IntuiMessage * );

static char buffer[DSP_BUFFER];                 /* All purpose buffer */

// Prototypes
static void idspSynchronize( void );
static void idspResWinDownA( void );
static void idspResWinUpA( void );

// Import from p3funcs.c
char *strins( char *s, int p, char *i );


/* Gadget definitions - The handler function is in GadgetUserData.
 *
 * Handler functions receive a pointer on their message as an argument.
 * An example signature is:
 *  void HandleFunc1( struct IntuiMessage *im );
 * If the message's not needed then this can be used instead:
 *  void HandleFunc2( void );
 */

// Pixel space at the upper and lower border of the SliderGadget.
#define SLIDER_SPACE      2

// DummyGadget - Display area for messages in BottomBorder.
static struct Gadget gDummy = {
   NULL, 0, 0, 1, 1, GFLG_GADGHNONE | GFLG_RELBOTTOM, GACT_BOTTOMBORDER,
   GTYP_BOOLGADGET };

// Pointers to BOOPSI-images.
static APTR iaU, iaD;

// Global definitions.
struct ConsoleDevice   *ConsoleDevice;


/************************************************************/
/********************** BOOPSI handling *********************/

// Calculates the heigth of a BOOPSI sysiclass/imageclass gadget.

static WORD ImageHeight( ULONG type )
{
   WORD hi;

   // Create image.
   struct Image *img = (struct Image *)NewObject( NULL, "sysiclass",
      SYSIA_Which, type,
      SYSIA_DrawInfo, dspInfo.di,
      TAG_END );

   // Read the heigth...
   if ( img )
   {
      hi = img->Height;

      // ...and release the image.
      DisposeObject( img );
   }
   else
      hi = 0;

   // Return the heigth.
   return hi;
}



// Remove the gadgets.
static void idspRemGadgets( struct Window *win )
{
   // Remove the gadgets from the window.
   RemoveGList( win, dspInfo.slider, -1 );

   // Release the gadgets.
   DisposeObject( dspInfo.up );
   DisposeObject( dspInfo.dw );
   DisposeObject( dspInfo.slider );

   // Release the images.
   DisposeObject( iaD );
   DisposeObject( iaU );
}



// Create the gadgets.
static void idspAddGadgets( struct Window *win )
{
   WORD arrhi = ImageHeight( UPIMAGE );
   WORD sizhi = ImageHeight( SIZEIMAGE );

   // Create the arrow images.
   iaU = (struct Image *)NewObject( NULL, "sysiclass",
      SYSIA_Which,   UPIMAGE,
      SYSIA_DrawInfo, dspInfo.di,
      TAG_END );

   iaD = (struct Image *)NewObject( NULL, "sysiclass",
      SYSIA_Which,   DOWNIMAGE,
      SYSIA_DrawInfo, dspInfo.di,
      TAG_END );


   // Get the gadget images from BOOPSI.
   dspInfo.slider = (struct Gadget *)NewObject( NULL, "propgclass",
      PGA_Freedom,   FREEVERT,
      PGA_NewLook,   TRUE,

      // Calculate position and size.
      GA_RelRight,   6-( win->BorderRight ),
      GA_Top,         win->BorderTop + SLIDER_SPACE,
      GA_Width,      9,
      GA_RelHeight,   - sizhi - 2*arrhi - win->BorderTop - 2*SLIDER_SPACE,

      // Knob fills whole container.
      PGA_Visible,   10,
      PGA_Total,      10,

      GA_RightBorder,TRUE,
      GA_RelVerify,   TRUE,

      // Gadget handler.
      GA_UserData,   idspSynchronize,
      TAG_END );

   dspInfo.dw = (struct Gadget *)NewObject( NULL, "buttongclass",
      GA_Previous,   dspInfo.slider,
      GA_RelBottom,   - sizhi - arrhi,
      GA_RelRight,   1-win->BorderRight,
      GA_RightBorder,TRUE,
      GA_Immediate,   TRUE,
      GA_Width,      17,
      GA_Height,      9,
      GA_Image,      iaD,
      GA_UserData,   idspResWinDownA,
      TAG_END );

   dspInfo.up = (struct Gadget *)NewObject( NULL, "buttongclass",
      GA_Previous,   dspInfo.dw,
      GA_RelBottom,   - sizhi - 2*arrhi,
      GA_RelRight,   1-win->BorderRight,
      GA_RightBorder,TRUE,
      GA_Immediate,   TRUE,
      GA_Width,      17,
      GA_Height,      9,
      GA_Image,      iaU,
      GA_UserData,   idspResWinUpA,
      TAG_END );


   // Gadget list is displayed - the first gadget created by BOOPSI is the
   // first in the list.
   AddGList( win, dspInfo.slider, -1, -1, NULL );
   RefreshGList( dspInfo.slider, win, NULL, -1 );
}


#ifdef __MAXON__
static ULONG SetMaxonAttrs( Gadget *g, Window *w, Requester *r, unsigned long s, ... )
{
   return SetGadgetAttrsA( g, w, r, (TagItem *)&s );
}
#else
#define SetMaxonAttrs SetGadgetAttrs
#endif



/* Update slider gadget
 *
 * If fl < 0, then the current first line is used for the computation,
 * else the line number passed in fl is used as the first line.
 */
static void idspUpdateSlider( int fl )
{
/* struct PropInfo *pi = (struct PropInfo *)dspInfo.slider->SpecialInfo; */

   fl = (fl < 0) ? fst_lin : fl;

   SetMaxonAttrs( dspInfo.slider, dspInfo.win, NULL,
      PGA_Visible, Lines,
      PGA_Top,     fl,
      PGA_Total,   itemnum,
      TAG_END );
}



/**************************************************/
/************ Console.device handling *************/

// Closes console.
static void idspCloseConsole( void )
{
    if ( dspInfo.wreq )
    {
        CloseDevice( (struct IORequest *)dspInfo.wreq );
        DeleteStdIO( dspInfo.wreq );
        DeleteMsgPort( dspInfo.wport );
    }
}



// Open a console for the passed window.
static BOOL idspOpenConsole( struct Window *win )
{
   if ( dspInfo.wport = CreateMsgPort() )
   {
      if ( dspInfo.wreq = CreateStdIO( dspInfo.wport ) )
      {
         dspInfo.wreq->io_Data = win;
         dspInfo.wreq->io_Length = sizeof( *win );

         if ( !OpenDevice( "console.device", CONU_SNIPMAP, (struct IORequest *)dspInfo.wreq, CONFLAG_DEFAULT ) )
         {
            ConsoleDevice = (struct ConsoleDevice *)dspInfo.wreq->io_Device;
            dspInfo.cun = (struct ConUnit *)dspInfo.wreq->io_Unit;

            return TRUE;
         }
         DeleteStdIO( dspInfo.wreq );
      }
      DeleteMsgPort( dspInfo.wport );
   }

   return FALSE;
}



// Print the passed string.
static void idspConOut( struct IOStdReq *wq, char *str )
{
   int len = strlen( str );

   wq->io_Command = CMD_WRITE;
   wq->io_Data    = (APTR)str;
   wq->io_Length  = (len > Columns) ? Columns: len ;
   DoIO( (struct IORequest *)wq );
}



// Print the passed string up to a maximum of len characters.
static void idspConOutL( struct IOStdReq *wq, char *str, int len )
{
    wq->io_Command = CMD_WRITE;
    wq->io_Data    = (APTR)str;
    wq->io_Length  = len;
    DoIO( (struct IORequest *)wq );
}



// Position cursor to (x,y).
static void idspConPos( struct IOStdReq *wq, int x, int y )
{
   char pbuf[10];
   sprintf( pbuf, "\x9b%d;%d\x48", y, x );
   idspConOutL( wq, pbuf, -1 );
}



// Scroll text down.
#define idspConScDown( wq )      idspConOutL( wq, "\x9b" "1\x54", -1 )

// Scroll text up.
#define idspConScUp( wq )         idspConOutL( wq, "\x9b" "1\x53", -1 )

// Clear console.
#define idspConClr( wq )         idspConOutL( wq, "\x0c", -1 )


// Initialise console.
static void idspInitConsole( void )
{
   idspConOutL( dspInfo.wreq, "\x9b\x30\x20\x70"      // Cursor off
                              "\x9b\x3f\x37\x6c",     // AutoWrap off
                              -1 );
}



/*************************************************/
/********* High level window functions ***********/

// Refresh output window.
void idspRefreshWin( void )
{
   int begin, end;
   SHORT lpos = 0;

   /* After enlarging a window pull text to the bottom of the window if this
    * is possible.
    ***/
   if ( (begin = itemnum - Lines) < fst_lin )
      fst_lin =  (begin >= 0) ? begin : 0;

   end = fst_lin + Lines;

   if ( end > itemnum )
      end = itemnum;

   idspConClr( dspInfo.wreq );

   if ( outarray )
   {
      for ( begin = fst_lin ; begin <= end ; begin++ )
      {
         idspConPos( dspInfo.wreq, 1, ++lpos );
         idspConOut( dspInfo.wreq, outarray[begin] );
      }
   }
}



static void idspResWinDown( void )
{
   if ( (itemnum - fst_lin) > Lines )
   {
      idspConScUp( dspInfo.wreq );
      idspConPos( dspInfo.wreq, 1, Lines + 1 );
      idspConOut( dspInfo.wreq, outarray[ ++fst_lin + Lines ] );
   }
}



// Scroll down display one line and update slider.
static void idspResWinDownA( void )
{
   idspResWinDown();
   idspUpdateSlider( -1 );
}




static void idspResWinUp( void )
{
    if ( fst_lin > 0 )
    {
        idspConScDown( dspInfo.wreq );
        idspConPos( dspInfo.wreq, 1, 1 );
        idspConOut( dspInfo.wreq, outarray[--fst_lin] );
    }
}



static void idspResWinUpA( void )
{
   idspResWinUp();
   idspUpdateSlider( -1 );
}



// Synchronise text with slider position.
static void idspSynchronize( void )
{
   ULONG i;               // Current line number.

   if ( itemnum > Lines )
      GetAttr( PGA_Top, dspInfo.slider, &i );
   else
      i = 0;

   if ( i != fst_lin )
   {
      if ( Lines <= abs( fst_lin-i ) +1 )   // Current line number is more than
      {                                     // a single page away.
         fst_lin = i;
         idspRefreshWin();
      }
      else if ( i < fst_lin )
      {
         while ( i < fst_lin )
            idspResWinUp();
      }
      else
      {
         while ( i > fst_lin )
            idspResWinDown();
      }
   }
}



/*************************************
 ********* Message handler ***********/

static void idspHandleRefresh( void )
{
   static int lheight;
   static int lwidth;

   if ( !itemnum )
      return;

   // Update only if the new window size is larger than the current.
   if ( lheight != Lines || lwidth != Columns )
   {
      idspUpdateSlider( -1 );
      idspRefreshWin();
   }

   lheight = Lines, lwidth = Columns;
}



static void idspHandleTicks( void )
{
   if ( itemnum && itemnum > Lines)
   {
      void (* gadmethod)( BOOL ) = NULL;


      // No BOOPSI-get-function available.
      if ( dspInfo.slider->Flags & GFLG_SELECTED )
         gadmethod = (void (*)(BOOL))dspInfo.slider->UserData;

      else if ( dspInfo.up->Flags & GFLG_SELECTED )
         gadmethod = (void (*)(BOOL))dspInfo.up->UserData;

      else if ( dspInfo.dw->Flags & GFLG_SELECTED )
         gadmethod = (void (*)(BOOL))dspInfo.dw->UserData;

      if ( gadmethod )
         gadmethod( TRUE );
   }
}



static BOOL idspHandleRawKey( struct IntuiMessage *message )
{
   char *poi; // Pointer to transformed message.

   switch ( message->Code )
   {
      case CURSORUP:
         idspResWinUpA();
         return TRUE;

      case CURSORDOWN:
         idspResWinDownA();
         return TRUE;
   }

   return FALSE;
}



static BOOL idspHandleVanillaKey( struct IntuiMessage *message )
{
   // Currently we do not use vanilla keys.
   return FALSE;
}



/***********************************
 ********** Message loop ***********/

static void idspMessageDispatch( void )
{
   ULONG sigmsk, sigs;
   SHORT brk = 1;

   sigmsk = 1L << dspInfo.win->UserPort->mp_SigBit;
   while ( brk )
   {
      sigs = Wait( sigmsk );
      if ( sigs & sigmsk )
      {
         struct IntuiMessage *message;

         while ( message = GT_GetIMsg( dspInfo.win->UserPort ) )
         {
            switch ( message->Class )
            {
               case IDCMP_CLOSEWINDOW:
                  brk = 0;
                  break;

               case IDCMP_REFRESHWINDOW:
                  GT_BeginRefresh( dspInfo.win );
                  GT_EndRefresh( dspInfo.win, TRUE );
                  idspHandleRefresh();
                  break;

               case IDCMP_INTUITICKS:
                  idspHandleTicks();
                  break;

               case IDCMP_GADGETUP:
               case IDCMP_GADGETDOWN:
               {
                  void (* gadmethod)( struct IntuiMessage * );
                  struct Gadget *gad = (struct Gadget *)message->IAddress;

                  if ( gadmethod = (void (*)(struct IntuiMessage *))gad->UserData )
                     gadmethod( message );
                  else if ( appIDCMPHandler )
                     appIDCMPHandler( message );
                  break;
               }

               case IDCMP_VANILLAKEY:
                  if ( (FALSE == idspHandleVanillaKey( message )) && appIDCMPHandler )
                        appIDCMPHandler( message );
                  break;

               case IDCMP_RAWKEY:
                  if ( (FALSE == idspHandleRawKey( message )) && appIDCMPHandler )
                        appIDCMPHandler( message );
                  break;

               case IDCMP_MENUPICK:
               {
                  ULONG mnum;

                  if ( MENUNULL != (mnum = message->Code) )
                  {
                     void (* menfunc)( SHORT * );

                     menfunc = (void (*)( SHORT *))GTMENUITEM_USERDATA( ItemAddress( dspInfo.menu, mnum ) );
                     (*menfunc)( &brk );
                  }

                  break;
               }

               default:
                  if ( appIDCMPHandler )
                     appIDCMPHandler( message );
            }

            GT_ReplyIMsg( message );
         }
      }
   }
}



/***********************************
 ******* Basic functions   *********/

BOOL DspInit( int x, int y, int w, int h, struct NewMenu *nm, void (* idcmpfunc)( struct IntuiMessage * ) )
{
   appIDCMPHandler = idcmpfunc ? idcmpfunc : NULL;

   // Get a pointer on the default screen.
   if ( NULL == (dspInfo.screen = LockPubScreen( NULL )) )
      return FALSE;

   // Compute the position of the dummy gadget.
   gDummy.LeftEdge = dspInfo.screen->WBorLeft;
   gDummy.TopEdge  = -(dspInfo.screen->WBorTop + dspInfo.screen->Font->ta_YSize);

   /*
    * For readability we define the IDCMP flags for the window here.
    *
    * Note: RAW- and VANILLAKEY are both set.  Thus no need to use
    * the function RawKeyConvert().
    */
#define DSP_IDCMP   (IDCMP_CLOSEWINDOW | IDCMP_REFRESHWINDOW | \
                  IDCMP_GADGETUP | IDCMP_GADGETDOWN | \
                  IDCMP_INTUITICKS | \
                  IDCMP_RAWKEY | IDCMP_VANILLAKEY | \
                  IDCMP_MENUPICK)

   dspInfo.win = OpenWindowTags( NULL,
      WA_Width, 640,  WA_Height, h,
      WA_Top,   y,    WA_Left,   x,
      WA_MinWidth,    640,
      WA_MinHeight,   STX_MIN_HEIGHT,
      WA_MaxWidth,    ~0,                  // TODO: 640 fixed width only temporary.
      WA_MaxHeight,   ~0,
      WA_CloseGadget, TRUE,
      WA_SizeGadget,  TRUE,
      WA_DragBar,     TRUE,
      WA_DepthGadget, TRUE,
      WA_SmartRefresh,TRUE,
      WA_Activate,    TRUE,
      WA_AutoAdjust,  TRUE,
      WA_IDCMP,       DSP_IDCMP,
      WA_Gadgets,     &gDummy,
      WA_PubScreen,   dspInfo.screen,
      TAG_DONE );

   if ( dspInfo.win )
   {
      // If window opened remove screen lock.
      UnlockPubScreen( NULL, dspInfo.screen );

      dspInfo.di = GetScreenDrawInfo( dspInfo.screen );

      if ( idspOpenConsole( dspInfo.win ) )
      {
         idspInitConsole();
         idspAddGadgets( dspInfo.win );

         if ( dspInfo.vis = GetVisualInfo( dspInfo.screen, TAG_END ) )
         {
            if ( dspInfo.menu = CreateMenus( nm, TAG_END ) )
            {
               if ( LayoutMenus( dspInfo.menu, dspInfo.vis, TAG_END ) )
               {
                  if ( SetMenuStrip( dspInfo.win, dspInfo.menu ) )
                     return TRUE;
               }
               FreeMenus( dspInfo.menu );
            }
            FreeVisualInfo( dspInfo.vis );
         }
         idspCloseConsole();
      }
      CloseWindow( dspInfo.win );
   }

   return FALSE;
}



void DspStart( void )
{
    idspMessageDispatch();
}



void DspExit( void )
{
   idspRemGadgets( dspInfo.win );
   ClearMenuStrip( dspInfo.win  );
   FreeMenus( dspInfo.menu );
   FreeVisualInfo( dspInfo.vis );
   idspCloseConsole();
   FreeScreenDrawInfo( dspInfo.screen, dspInfo.di );
   CloseWindow( dspInfo.win );
}



/**************************************
 ******** Helper functions  ***********/

void DspPutText( char *text[] )
{
   fst_lin = 0;

   // If text was passed compute number of lines.
   if ( text )
   {
      for ( itemnum = 0; text[itemnum] ; itemnum++ )
         ;
      itemnum--;
   }
   else
      itemnum = 0;

   // Assign pointer to text array.
   outarray = text;

   // Display window contents.
   idspRefreshWin();
   idspUpdateSlider( -1 );
}



APTR DspGetVisInfo( void )
{
    return dspInfo.vis;
}



int DspGetCurrentLine( void )
{
    return fst_lin + 1;
}



void DspMarkText( int lin, int col, int len )
{
    static int lastl;

    // If lin is not in the visible area.
    if ( ( lin-1 < fst_lin ) || ( lin-1 > (fst_lin+Lines) ) )
    {
        int i, z;

        if ( (z = lin - (Lines >> 1)) < 0 )       // z is topmost line.
            z = 0;

        if ( (i = itemnum - Lines) < (z-1) )      // i is largest allowed value for fst_lin
            i =  (i >= 0) ? i : 0;                // if itemnum > Lines => i<0  => i:=0 else ok
        else
            i = (z>0) ? z-1 : 0;

        idspUpdateSlider( i );
        idspSynchronize();
    }

    if ( ( lastl-1 >= fst_lin ) && ( lastl-1 <= (fst_lin+Lines) ) && ( lastl != lin ) )
    {
        idspConPos( dspInfo.wreq, 1, lastl-fst_lin );
        idspConOut( dspInfo.wreq, outarray[lastl-1] );
    }
    lastl = lin;

    strcpy( buffer, outarray[lin-1] );

    idspConPos( dspInfo.wreq, 1, lin-fst_lin );
    strins( buffer, col+len-1, "\x9b" "0" "\x6d" ); // Inv. off; 1
    strins( buffer, col-1,     "\x9b" "7" "\x6d" ); // Inv. on ; 2
    idspConOut( dspInfo.wreq, buffer );
}



/*
 * Prints message (used for error messages).
 *
 * TODO: Use variable argument lists
 *       Support queuing of messages.
 *       If no window is open a requester should be shown.
 */
void DspMessage( char *msg )
{
   BYTE oldAPen, newAPen;
   LONG MaxWi;
   struct IntuiText imsg;

   // Get string in buffer.
   if ( msg )
      strncpy( buffer, msg, DSP_BUFFER );
   else
      return;

   // Compute text colour.
   if ( dspInfo.win->Flags & WFLG_WINDOWACTIVE )
      newAPen = dspInfo.di->dri_Pens[FILLPEN];
   else
      newAPen = dspInfo.di->dri_Pens[BACKGROUNDPEN];

   // Initialise imsg.
   memset( &imsg, 0, sizeof( imsg ) );

   // Font.
   imsg.ITextFont= dspInfo.screen->Font;

   // Text colour.
   imsg.FrontPen = dspInfo.di->dri_Pens[FILLTEXTPEN];

   // Background colour depends on window active or not.
   imsg.BackPen = newAPen;

   // Draw Mode.
   imsg.DrawMode = JAM2;

   // Text.
   imsg.IText = buffer;

   // Maximum display width.
   MaxWi = dspInfo.win->Width-dspInfo.win->BorderLeft-dspInfo.win->BorderRight;

   // Delete output area.
   oldAPen = dspInfo.win->RPort->FgPen;
   SetAPen( dspInfo.win->RPort, newAPen );
   RectFill( dspInfo.win->RPort,
      dspInfo.win->BorderLeft,
      dspInfo.win->Height - dspInfo.win->BorderBottom +1,
      dspInfo.win->Width - dspInfo.win->BorderRight -1,
      dspInfo.win->Height - dspInfo.win->BorderBottom +1 + dspInfo.screen->RastPort.TxHeight );
   SetAPen( dspInfo.win->RPort, oldAPen );

   // Fits text in window?
   while ( IntuiTextLength( &imsg ) > MaxWi )
      buffer[ strlen( buffer ) -1 ] = '\0';

   // Display new message.
   PrintIText( dspInfo.win->RPort, &imsg, dspInfo.win->BorderLeft +2,
      dspInfo.win->Height - dspInfo.win->BorderBottom +1 );
}



/* DspSetWindowTitle */

void DspSetWindowTitle( char *strg )
{
    SetWindowTitles( dspInfo.win, (UBYTE *)strg, (UBYTE *)-1 );
}



/* DspWindowToFront */

void DspWindow2Front( void )
{
    WindowToFront( dspInfo.win );
}



void DspWindow2Back( void )
{
   WindowToBack( dspInfo.win );
}



struct Window *DspGetWindow( void )
{
   return dspInfo.win;
}



void DspPgUp( void )
{
    int i;

    if ( fst_lin > ( i = Lines-Overlap ) )
        i = fst_lin - i;
    else
        i = 0;

    idspUpdateSlider( i );
    idspSynchronize();
}




void DspPgDown( void )
{
    int i;

    if ( itemnum-Lines > (fst_lin + (i = Lines-Overlap)) )
        i = fst_lin + i;
    else
        i = itemnum - Lines;

    idspUpdateSlider( i );
    idspSynchronize();
}




void DspTop( void )
{
   idspUpdateSlider( 0 );
   idspSynchronize();
}




void DspBottom( void )
{
   idspUpdateSlider( itemnum - Lines );
   idspSynchronize();
}




void DspLineUp( void )
{
    idspResWinUpA();
}




void DspLineDown( void )
{
    idspResWinDownA();
}



void DspRefreshWin( void )
{
   idspRefreshWin();
}


/* DspSetTopOffset
 *
 * Defines an empty area to the top of the text display not
 * used by the console.device.  This is used to display
 * gadgets.
 * Offset is the height of the free area in pixels.
 */
void DspSetTopOffset( int offset )
{
   sprintf( buffer, "\x9b%d\x79", offset );
   idspConOutL( dspInfo.wreq, buffer, -1 );
}



/* DspGetFont
 *
 * Returns the font used by the console.device.
 */
struct TextAttr *DspGetFont( void )
{
   static struct TextAttr ta;

   if ( !ta.ta_Name )
      AskFont( dspInfo.win->RPort, &ta );

   return &ta;
}



// Checks whether the passed line number is visible.
BOOL DspIsLineVisible( int lin )
{
   if ( ( lin-1 < fst_lin ) || ( lin-1 > (fst_lin+Lines) ) )
      return FALSE;
   else
      return TRUE;
}



// Returns prototype for window size.
char *GetDspToolType( void )
{
   return "WINDIM";
}



// Returns current window size in format x/y/w/h.
char *GetDspToolSize( void )
{
   sprintf( buffer, "%hd/%hd/%hd/%hd",
      dspInfo.win->LeftEdge,
      dspInfo.win->TopEdge,
      dspInfo.win->Width,
      dspInfo.win->Height );
   return buffer;
}
