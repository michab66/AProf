/*
 * $RCSfile: p3exec.c,v $
 *
 * Executes a program under profiler control.
 *
 * $Revision: 1.2 $ $Date: 2008/04/10 18:33:24 $
 *
 * Copyright © 1992-94 Michael G. Binz
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <intuition/gadgetclass.h>
#include <clib/exec_protos.h>
#include <clib/dos_protos.h>
#include <clib/gadtools_protos.h>
#include <clib/intuition_protos.h>

#include <pragmas/exec_lib.h>
#include <pragmas/dos_lib.h>
#include <pragmas/gadtools_lib.h>
#include <pragmas/intuition_lib.h>

#include "version.h"
#include "dsp.h"
#include "pro.h"
#include "timer.h"

// Transform APTR->BOOL
#define AP2BOOL( ap )   (ap ? TRUE : FALSE)

// Window dimensions.
#define SR_WIN_WI       535

// Command line length.
#define MAX_COMLINE      256

static char xbuffer[256];

/*
 * Forward definition.
 */
static void ExecHelp( struct IntuiMessage * );


/*
 * Gadget ids, these are indexes into the table below.
 */
enum
{
   ID_COMLINE,
   ID_STACK,
   ID_USE,
   ID_CANCEL,
   ID_HELP
} GadgetID;



// Definition of the Gadgets in the Exec (Data) dialog.

/* Vergiss es! Ab Rev. 1.2 nochmal geändert! In Zukunft muss das so laufen
 * wie in p3prefs.c!
 * Achtung: Wieder neue Logik. Wenn ng_UserData ungleich Null ist, dann ist dieses
 * Gadget ein Endgadget. Der (Boolesche) Returnwert ergibt sich aus ng_UserData>>1
 * d.h. bei Positivem Abbruch (Use,OK) muss ng_UserData == 0x02 sein!!.
 *
 * TODO: Endlich ein allgemeines System implementieren!!
 */ 
static struct NewGadget DlgExec[] = {
   { 130, 0, 390, 0, "Command line: ", NULL, 1, 0, NULL, NULL },
   { 130, 0,  80, 0, "Stack size: ",   NULL, 2, 0, NULL, NULL },
   { 220, 0,  90, 0, "Use",            NULL, 2, 0, NULL, (APTR)0x02 },
   { 320, 0,  90, 0, "Cancel",         NULL, 2, 0, NULL, (APTR)0x01 },
   { 420, 0,  90, 0, "Help",           NULL, 2, 0, NULL, ExecHelp },
   {   0, 0,   0, 0, NULL,             NULL, 0, 0, NULL, NULL }
};



static void SetComline( char *cl )
{
   if ( THE_BASE.pb_comline )
      free( THE_BASE.pb_comline );
      
   THE_BASE.pb_comline = stralloc( cl );
}



static ULONG DlgExecLayout( struct NewGadget *gl )
{
   WORD TopFrameHi = DspGetWindow()->BorderTop;
   WORD prLineN;   // Previous line (line number)
   WORD crLineN;   // Current line (line number)
   WORD crLineP;   // Current line (pixels)
   WORD crColmP;   // Current column position (Pixels)
   ULONG GadHi;

   // Compute Gadgetheight depending on the default font (1.5 * line height)
   GadHi = DspGetFont()->ta_YSize + (DspGetFont()->ta_YSize>>1);

   for ( prLineN = 0, crLineN = gl->ng_GadgetID ; crLineN ; crLineN = (++gl)->ng_GadgetID )
   {
      if ( prLineN != crLineN )
      {
         // Start column.
         crColmP = INTERWIDTH / 2;

         // Compute current line in pixels.
         crLineP = TopFrameHi
                  + (crLineN * INTERHEIGHT)
                  + ((crLineN-1) * GadHi);

         // Set prLineN to current value.
         prLineN = crLineN;
      }

      gl->ng_TopEdge    = crLineP;
      gl->ng_Height     = GadHi;
      gl->ng_TextAttr   = DspGetFont();
      gl->ng_VisualInfo = DspGetVisInfo();
   }
   
   return crLineP + GadHi + INTERWIDTH/2 + DspGetWindow()->WScreen->WBorBottom;
}



/* ExecHelp
 *
 * Call the help system.
 */
static void ExecHelp( struct IntuiMessage *dummy )
{
   ProvideAProfHelp( "dialog-exec", 0 );
}



static BOOL dispatch_execopt( struct Window *w, struct Gadget *sgad )
{
   BOOL brk = FALSE, ret;
   struct IntuiMessage *im;

   while( !brk )
   {
      Wait( 1 << w->UserPort->mp_SigBit );

      while( !brk && (im = GT_GetIMsg( w->UserPort )) )
      {
         switch( im->Class )
         {
            case IDCMP_GADGETUP:
            {
               struct Gadget *gad = (struct Gadget *)im->IAddress;

               if ( gad->UserData )
               {
                  if ( (long)gad->UserData > 0 && (long)gad->UserData < 3 )
                  {
                     brk = TRUE;
                     ret = AP2BOOL( ((ULONG)gad->UserData)>>1 );
                  }
                  else
                     ((void(*)(struct IntuiMessage *))(gad->UserData))( im );
               }
               break;
            }

            case IDCMP_ACTIVEWINDOW:
               ActivateGadget( sgad, w, NULL );
               break;

            case IDCMP_REFRESHWINDOW:
               GT_BeginRefresh( w );
               GT_EndRefresh( w, TRUE );
               break;
         }

         GT_ReplyIMsg( im );
      }
   }

   return ret;
}



void men_execopt( void )
{
   ULONG WinHi;
   struct Gadget *glist = NULL, *gad, *clgad, *stgad;

   // If no program is loaded then leave.
   if ( NULL == THE_BASE.pb_symlist )
   {
      p3err( "No program loaded" );
      return;
   }
   
   // Stack size must not be less than 4000 bytes.
   if ( THE_BASE.pb_stacksize < 4000 )
      THE_BASE.pb_stacksize = 4000;

   // Initialise Gadgets.
   WinHi = DlgExecLayout( DlgExec );

   gad = CreateContext( &glist );

   gad = clgad = CreateGadget( STRING_KIND, gad, &DlgExec[ID_COMLINE], 
      GTST_String,   THE_BASE.pb_comline,
      GTST_MaxChars,   MAX_COMLINE,
      TAG_END );
   gad = stgad = CreateGadget( INTEGER_KIND, gad, &DlgExec[ID_STACK], 
      GTIN_Number,    THE_BASE.pb_stacksize,
      STRINGA_Justification, GACT_STRINGRIGHT,
      TAG_END );
   gad = CreateGadget( BUTTON_KIND, gad, &DlgExec[ID_USE],
      TAG_END );
   gad = CreateGadget( BUTTON_KIND, gad, &DlgExec[ID_CANCEL],
      TAG_END );
   gad = CreateGadget( BUTTON_KIND, gad, &DlgExec[ID_HELP],
      TAG_END );

   if ( gad )
   {
      // Parent window for positioning.
      struct Window *pw = DspGetWindow();
      
      struct Window *w = OpenWindowTags( NULL,
         WA_Left,       pw->LeftEdge + pw->BorderLeft,
         WA_Top,        pw->TopEdge + pw->BorderTop,
         WA_Width,      SR_WIN_WI,
         WA_Height,     WinHi,
         WA_DragBar,    TRUE,
         WA_Activate,   TRUE,
         WA_Title,      APP_NAME " Execution data",
         WA_IDCMP,      BUTTONIDCMP|STRINGIDCMP|IDCMP_ACTIVEWINDOW,
         WA_SmartRefresh,TRUE,
         WA_AutoAdjust,  TRUE,
         WA_Gadgets,      glist,
         TAG_END );

      if ( w )
      {
         GT_RefreshWindow( w, NULL );
         if ( dispatch_execopt( w, clgad ) )
         {
            SetComline( (char *)((struct StringInfo *)clgad->SpecialInfo)->Buffer );
            THE_BASE.pb_stacksize = ((struct StringInfo *)stgad->SpecialInfo)->LongInt;
         }
         CloseWindow( w );
      }
   }

   FreeGadgets( glist );       // Muß immer durchlaufen werden!! (gad == NULL)
}



static void SetXTime( ULONG t )
{
   extern void UpdateXTime( char * );
   static char numbuf[20];

   double ms = Tick2Ms( t );

   if ( 1000.0 > ms )
      sprintf( numbuf, "%.3lf ms", ms );
   else
      sprintf( numbuf, "%.3lf s", ms/1000.0 );

   UpdateXTime( numbuf );
}



void pro_exec( void )
{
   size_t comlen;
   char *comlin;
   ULONG rc, time, xtime;

   if ( THE_BASE.pb_symlist == 0 )
   {
      p3err( "No program loaded" );
      return;
   }

   if ( InitTimerCIA() == FALSE )
   {
      p3err( "Timer not available" );
      return;
   }

   // Reload segment list.
   if ( FALSE == UpdateSymlist() )
      return;

   CreateSaveTable();
#if 0
   /* Not yet dynamically allocated. */
   FreeSaveTable();
#endif
   // Set break points.
   if ( FALSE == BreakSet() )
   {
      p3err( "Not enough memory for breakpoint system" );
      return;
   }

   if ( THE_BASE.pb_stacksize < 4000 )
      THE_BASE.pb_stacksize = 4000;

   // Display the command line.
   if ( THE_BASE.pb_comline )
      sprintf( xbuffer, " %s\n", THE_BASE.pb_comline );
   else
      strcpy( xbuffer, "\n" );

   DspWindow2Back();

      /* Display the executed command. */
      FPuts( Output(), THE_BASE.pb_comname );
      FPuts( Output(), xbuffer );

      time = StartTimeMarkCIA();
      StartTimerCIA();

         RunCommand( xSetStart( THE_BASE.pb_seglist ), 
            THE_BASE.pb_stacksize, 
            xbuffer,
            strlen( xbuffer ) );

      StopTimerCIA();                     
      THE_BASE.pb_base_time += xtime = StopTimeMarkCIA( time );

   DspWindow2Front();

   // Timer overflow?
   if ( timer_err )
      p3err( "Timer overflow. Max. profiling time 99 minutes" );
   else
   {
      SetXTime( xtime );
      ShowSymbolList();
   }
      

   ExitTimerCIA();
}
