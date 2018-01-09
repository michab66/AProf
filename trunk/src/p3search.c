/*
 * $RCSfile: p3search.c,v $
 *
 * Suchsystem für Profile3
 *
 * $Revision: 1.1.1.1 $ $Date: 2002/05/31 23:15:48 $
 *
 * © 1993,94 Michael G. Binz
 */

#include <string.h>
#include <stdlib.h>

#include <clib/gadtools_protos.h>
#include <clib/intuition_protos.h>
#include <clib/exec_protos.h>

#include <pragmas/gadtools_lib.h>
#include <pragmas/intuition_lib.h>
#include <pragmas/exec_lib.h>

#include "version.h"
#include "dsp.h"	
#include "pro.h"


// Umsetzung APTR -> Boolean
#define AP2BOOL( ap )	( ap ? TRUE : FALSE )





/*
 * Definition der Benutzeroberfläche
 */

// Breite des Fensters (Höhe wird berechnet)
#define SR_WIN_WI			340

enum { GD_STRING, GD_CASE, GD_USE, GD_CANCEL } DlgSearchIdx;

static struct NewGadget DlgSearch[] = {
   { 140, 0, 180, 0, "Search string:", NULL, 1, 0, NULL, (APTR)0x02 },
   {  20, 0,  26, 0, "Case sensitive", NULL, 2, PLACETEXT_RIGHT, NULL, NULL },
   { 180, 0,  60, 0, "Start",          NULL, 2, 0, NULL, (APTR)0x02 },
   { 260, 0,  60, 0, "Cancel",         NULL, 2, 0, NULL, (APTR)0x01 },
   {   0, 0,   0, 0, NULL,             NULL, 0, 0, NULL, NULL }
};





/*
 * Statusvariablen des Moduls
 */

// Suchstring (muß bei Programmende freigegeben werden)
static char *pszSearch;
// Startzeile der Suche
static int  fd_lin;
// Startspalte der Suche
static int  fd_col;
// Groß/Kleinschreibung beachten?
static BOOL case_sensitive = FALSE;





/*
 *
 * Moduldestruktor für p3search.c
 *
 */ 
static void FreeSearch( void )
{
   if ( pszSearch )
   {
      free( pszSearch );
      pszSearch = NULL;
   }
}





/* SetSearchString()
 *
 * Definition eines neuen Suchstrings
 */
static void SetSearchString( char *s )
{
   // Letzten Suchstring freigeben
   FreeSearch();
   
   // Neuen einhängen
   if ( s && *s )
      pszSearch = stralloc( s );
}





/* DlgSearchLayout
 *
 * Gadgetpositionen und -höhen werden berechnet
 */
static ULONG DlgSearchLayout( struct NewGadget *gl )
{
   WORD TopFrameHi = DspGetWindow()->BorderTop;
   WORD prLineN;   // Letzte bearbeitete Zeile (Nummer)
   WORD crLineN;   // Gerade bearbeitete Zeile (Nummer)
   WORD crLineP;   // ditto.                   (Pixels)
   WORD crColmP;   // Aktuelle Spaltenposition (Pixels)
   ULONG GadHi;

   /* Berechnung der Gadgethöhe in Abhängigkeit vom default font 
    * (1.5 * Zeilenhöhe)
    */
   GadHi = DspGetFont()->ta_YSize + (DspGetFont()->ta_YSize>>1);

   for ( prLineN = 0, crLineN = gl->ng_GadgetID 
         ; crLineN 
         ; crLineN = (++gl)->ng_GadgetID )
   {
      if ( prLineN != crLineN )
      {
         // Startspalte
         crColmP = INTERWIDTH / 2;

         // Aktuelle Pixelzeile berechnen
         crLineP = TopFrameHi
                  + (crLineN * INTERHEIGHT)
                  + ((crLineN-1) * GadHi);

         // prLineN auf aktuellen Wert setzen
         prLineN = crLineN;
      }

      gl->ng_TopEdge      = crLineP;
      gl->ng_Height      = GadHi;
      gl->ng_TextAttr   = DspGetFont();
      gl->ng_VisualInfo   = DspGetVisInfo();
   }
   
   return crLineP + GadHi + INTERWIDTH/2 + DspGetWindow()->WScreen->WBorBottom;
}




static BOOL app_dispatch_search( struct Window *w, struct Gadget *sgad, struct Gadget *cgad )
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
                  brk = TRUE;
                  ret = AP2BOOL( ((ULONG)gad->UserData)>>1 );
               }
               else if ( gad = cgad )
                  case_sensitive = ~case_sensitive;

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




// slin - Startzeile für Suche [1-...[

// TODO: Diese Funktion sollte mal neu implementiert werden...

static void app_search( char *ftxt, int slin, int scol )
{
    int lin, col;
    char *srchs, *found = NULL;

    lin = slin, col = 0;                                 // Init

    do
    {
        if (scol)
        {
            srchs = &THE_BASE.pb_display[lin-1][scol-1];
            scol = 0;
        }
        else
            srchs = THE_BASE.pb_display[lin-1];

        if ( case_sensitive )
            found = strstr( srchs, ftxt );
        else
            found = strstri( srchs, ftxt );

        if ( found )
            col = found - THE_BASE.pb_display[lin-1] +1;
        else
            lin = (THE_BASE.pb_display[lin]) ? lin+1 : 1;   // Count 1..
    }
    while ( lin!=slin && !found );                      // Bedingung

    if ( found )
    {
        DspMarkText( lin, col, strlen( ftxt ) );
        fd_lin = lin; fd_col = col;
    }
    else
        p3err( "Pattern not found: '%s'", ftxt );
}




void app_find( void )
{
   static BOOL cleanup = FALSE;
   BOOL DoSearch;
   ULONG WinHi;
   struct Gadget *glist = NULL, *gad, *stgad, *csgad;


   // Destruktor anmelden
   if ( !cleanup )
   {
      atexit( FreeSearch );
      cleanup = TRUE;
   }

   // Ist eine Bildschirmanzeige aufgebaut?
   if ( !THE_BASE.pb_display )
   {
      p3err( "No symbols to search" );
      return;
   }

   WinHi = DlgSearchLayout( DlgSearch );
   
   gad = CreateContext( &glist );

   gad = stgad = CreateGadget( STRING_KIND, gad, &DlgSearch[GD_STRING],
      GTST_String, pszSearch, TAG_END );

   gad = csgad = CreateGadget( CHECKBOX_KIND, gad, &DlgSearch[GD_CASE],
      GTCB_Checked, case_sensitive, TAG_END );

   gad = CreateGadget( BUTTON_KIND, gad, &DlgSearch[GD_USE], TAG_END );
   gad = CreateGadget( BUTTON_KIND, gad, &DlgSearch[GD_CANCEL], TAG_END );

   if ( gad )
   {
      // Parent Window für Position
      struct Window *pw = DspGetWindow();
      
      struct Window *w = OpenWindowTags( NULL,
         WA_Left,         pw->LeftEdge + pw->BorderLeft,
         WA_Top,         pw->TopEdge + pw->BorderTop,
         WA_Width,      SR_WIN_WI,
         WA_Height,      WinHi,
         WA_DragBar,      TRUE,
         WA_Activate,   TRUE,
         WA_Title,      APP_NAME " Search",
         WA_IDCMP,      BUTTONIDCMP|STRINGIDCMP|CHECKBOXIDCMP|IDCMP_ACTIVEWINDOW,
         WA_SmartRefresh,TRUE,
         WA_AutoAdjust,  TRUE,
         WA_Gadgets,      glist,
         TAG_END );

      if ( w )
      {
         GT_RefreshWindow( w, NULL );

         if ( DoSearch = app_dispatch_search( w, stgad, csgad ) )
            SetSearchString( (char *)((struct StringInfo *)stgad->SpecialInfo)->Buffer );
               
         CloseWindow( w );
      }
   }

   FreeGadgets( glist );       // Muß immer durchlaufen werden!! (gad == NULL)

   if ( DoSearch && pszSearch && *pszSearch )             // Start der Suche
      app_search( pszSearch, DspGetCurrentLine(), 0 );

   return;
}



void app_fnext( void )
{
   if ( pszSearch && *pszSearch )
   {
      int lin, col;

      if ( DspIsLineVisible( fd_lin ) )
      {
         lin = fd_lin;
         col = fd_col+1;
      }
      else
      {
         lin = DspGetCurrentLine();
         col = 0;
      }

      app_search( pszSearch, lin, col );
   }
   else
      p3err( "Nothing to search for" );
}

