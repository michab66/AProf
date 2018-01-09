/*************************************************************************
 * $RCSfile: p3prefs.c,v $
 *
 * Requester für Filepattern
 *
 * $Revision: 1.1.1.1 $ $Date: 2002/05/31 23:15:46 $
 * 
 * (c) 1992-94 Michael G. Binz
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <intuition/gadgetclass.h>
#include <clib/dos_protos.h>
#include <clib/gadtools_protos.h>
#include <clib/intuition_protos.h>

#include <pragmas/intuition_lib.h>
#include <pragmas/gadtools_lib.h>
#include <pragmas/dos_lib.h>
#include <pragmas/exec_lib.h>

#include "version.h"
#include "dsp.h"
#include "pro.h"


// Verschlüsselung der zusätzlichen Gadget Information
#define GadId( endg, line )   (((WORD)endg<<8)|line)
#define GetEnd( id )          (id>>8)
#define GetLine( id )         ((WORD)id & 0xff)

// Umwandlung APTR->Boolean
#define AP2BOOL( ap )   ( ap ? TRUE : FALSE )

// Dimensionen des Fensters
#define SR_WIN_WI       455

// Save wurde gewählt
static BOOL SaveToolTypes = FALSE;


/*
 * Forward Definitionen
 */
static void PrefsHelp( struct IntuiMessage * );
static void HandleSave( struct IntuiMessage * );


/*
 * Liste der Preferences-Daten
 */
static UWORD mode;
static UWORD unit;
static UWORD sort;
static char *patt;
static BOOL  safe;

/*
 * Zugehörige Setter
 */

static void setMode( struct IntuiMessage *msg )
{
   mode = msg->Code;
}

static void setUnit( struct IntuiMessage *msg )
{
   unit = msg->Code;
}

static void setSort( struct IntuiMessage *msg )
{
   sort = msg->Code;
}

static void setSafe( struct IntuiMessage *msg )
{
   safe = safe ? FALSE : TRUE;
}



/*
 * Bedeutung von endGad:
 *      o EG_NUL      keine weitere Reaktion (standard)
 *      o EQ_QIT      Requester schließen, Daten nicht übernehmen
 *      o EQ_USE      Requester schließen, Daten an Anwendung liefern
 */
enum endGad { EG_NUL, EG_QIT, EG_USE };

/*
 * Bedeutung von gadIds:
 *      Werden als Indizes in das folgende Array von NewGadget-Struktuern verwendet
 */
enum gadIds { GD_PATT, GD_MODE, GD_DUMM,
              GD_SORT, GD_UNIT, GD_SAFE,
              GD_REXX, GD_SAVE, GD_USE,
              GD_CANCL,GD_HELP };

/*
 * DlgPattern[]:
 *  Dieses Array enthält Beschreibungen für alle verwendeten Gadgets. Die
 *  einzelnen Felder sind in den RKM's dokumentiert.
 *  Das ID Feld wurde verwendet, um die Zeile (Text) und weitere Infos zur
 *  Reaktion auf Anwahl zu codieren.
 *  UserData enthält einen Methodenaufruf für das jeweilige Gadget (wenn
 *  einer vorhanden ist) oder NULL.
 */
static struct NewGadget DlgPattern[] = {
   { 137, 0, 308, 0, "Symbol pattern:", NULL, 
      GadId( EG_NUL, 1 ), 0, NULL, NULL },
   {  66, 0, 159, 0, "Mode: ",          NULL, 
      GadId( EG_NUL, 2 ), 0, NULL, setMode },
   { 285, 0,   1, 1, "Sort order:",     NULL, 
      GadId( EG_NUL, 2 ), PLACETEXT_RIGHT | NG_HIGHLABEL, NULL, NULL },
   { 275, 0, 220, 0, NULL,              NULL, 
      GadId( EG_NUL, 3 ), PLACETEXT_RIGHT, NULL, setSort },
   {  66, 0, 159, 0, "Units:",          NULL, 
      GadId( EG_NUL, 3 ), 0, NULL, setUnit },
   {   9, 0,  26, 0, "Safe profiling",  NULL, 
      GadId( EG_NUL, 4 ), PLACETEXT_RIGHT, NULL, setSafe },
   { 139, 0,  86, 0, "Rexx Unmangler:", NULL, 
      GadId( EG_NUL, 5 ), 0, NULL, NULL },
   {  30, 0,  90, 0, "Save",            NULL, 
      GadId( EG_USE, 7 ), 0, NULL, HandleSave },
   { 130, 0,  90, 0, "Use",             NULL, 
      GadId( EG_USE, 7 ), 0, NULL, NULL },
   { 230, 0,  90, 0, "Cancel",          NULL, 
      GadId( EG_QIT, 7 ), 0, NULL, NULL },
   { 330, 0,  90, 0, "Help",            NULL,
      GadId( EG_NUL, 7 ), 0, NULL, PrefsHelp },
   {   0, 0,   0, 0, NULL,              NULL, 
      0,                  0, NULL, NULL },
};



/*
 * Berechnung der Gadget Positionen und -höhen
 */
static ULONG DlgPatternLayout( struct NewGadget *gl )
{
   WORD TopFrameHi = DspGetWindow()->BorderTop;
   WORD prLineN;   // Letzte bearbeitete Zeile (Nummer)
   WORD crLineN;   // Gerade bearbeitete Zeile (Nummer)
   WORD crLineP;   // ditto.                   (Pixels)
   WORD crColmP;   // Aktuelle Spaltenposition (Pixels)
   ULONG GadHi;

   // Berechnung der Gadgethöhe in Abhängigkeit vom default font (1.25 * Zeilenhöhe)
   GadHi = DspGetFont()->ta_YSize + (DspGetFont()->ta_YSize>>1);

   for ( prLineN = 0, crLineN = GetLine( gl->ng_GadgetID )
      ; crLineN
      ; crLineN = GetLine((++gl)->ng_GadgetID) )
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

      gl->ng_TopEdge    = crLineP;
      gl->ng_Height     = GadHi;
      gl->ng_TextAttr   = DspGetFont();
      gl->ng_VisualInfo = DspGetVisInfo();
   }

   // Benötigte Fensterhöhe
   return crLineP + GadHi + INTERWIDTH/2 + DspGetWindow()->WScreen->WBorBottom;
}



/* Aufruf des Hilfesystems */
static void PrefsHelp( struct IntuiMessage *dummy )
{
   ProvideAProfHelp( "dialog-preferences", 0 );
}



/* Bearbeitung des 'Save' Buttons */
static void HandleSave( struct IntuiMessage *dummy )
{
   SaveToolTypes = TRUE;
}


  
/* Window Dispatcher */
static BOOL dispatch_sympat( struct Window *w, struct Gadget *sgad )
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
            case IDCMP_GADGETDOWN:
            {
               struct Gadget *gad = (struct Gadget *)im->IAddress;

               // Aufruf des Setters, wenn definiert
               if ( gad->UserData )
               {
                  void (* gadmethod)( struct IntuiMessage * );

                  if ( gadmethod = (void (*)(struct IntuiMessage *))gad->UserData )
                     gadmethod( im );
               }

               if ( GetEnd( gad->GadgetID ) )
               {
                  brk = TRUE;
                  ret = AP2BOOL( GetEnd( gad->GadgetID ) >>1 );
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




void men_sympat( void )
{
   extern struct Library *RexxSysBase;

   ULONG WinHi;
   struct Gadget *glist = NULL, *gad, *stgad, *rxgad;

   // Temporäre Variablen initialisieren
   mode = GetModeIdx();
   unit = GetEinheitIdx();
   sort = GetSortIdx();
   safe = GetSaveProf();

   // Gadgetpositionen und -höhen berechnen
   WinHi = DlgPatternLayout( DlgPattern );

   gad = CreateContext( &glist );

   gad = stgad =    CreateGadget( STRING_KIND, gad, &DlgPattern[GD_PATT],
      GTST_String,   GetPattern(),
      TAG_END );

   if ( RexxSysBase )
   {
      gad = rxgad = CreateGadget( STRING_KIND, gad, &DlgPattern[GD_REXX],
         GTST_String,   GetRexxFunc(),
         TAG_END );
   }
   else
   {
      gad = rxgad = CreateGadget( STRING_KIND, gad, &DlgPattern[GD_REXX],
         GA_Disabled,   TRUE,
         TAG_END );
   }

   gad =   CreateGadget( CYCLE_KIND, gad, &DlgPattern[GD_MODE],
      GTCY_Labels,   GetModeStringTable(),
      GTCY_Active,   GetModeIdx(),
      TAG_END );

   gad = CreateGadget( CYCLE_KIND, gad, &DlgPattern[GD_UNIT],
      GTCY_Labels,   GetEinheitStringTable(),
      GTCY_Active,   GetEinheitIdx(),
      TAG_END );

   gad = CreateGadget( MX_KIND, gad, &DlgPattern[GD_SORT],
      GTMX_Labels,   GetSortStringTable(),
      GTMX_Active,   GetSortIdx(),
      TAG_END );

   gad = CreateGadget( TEXT_KIND, gad, &DlgPattern[GD_DUMM], TAG_END );

   gad = CreateGadget( CHECKBOX_KIND, gad, &DlgPattern[GD_SAFE],
      GTCB_Checked,   GetSaveProf(),
      TAG_END );

   gad = CreateGadget( BUTTON_KIND, gad, &DlgPattern[GD_SAVE],
      TAG_END );
   gad = CreateGadget( BUTTON_KIND, gad, &DlgPattern[GD_USE],
      TAG_END );
   gad = CreateGadget( BUTTON_KIND, gad, &DlgPattern[GD_CANCL],
      TAG_END );
   gad = CreateGadget( BUTTON_KIND, gad, &DlgPattern[GD_HELP],
      TAG_END );

   if ( gad )
   {
      // Parent Window für Positionierung
      struct Window *pw = DspGetWindow();

      struct Window *w = OpenWindowTags( NULL,
         WA_Left,         pw->LeftEdge + pw->BorderLeft,
         WA_Top,         pw->TopEdge + pw->BorderTop,
         WA_Width,      SR_WIN_WI,
         WA_Height,      WinHi,
         WA_DragBar,      TRUE,
         WA_Activate,   TRUE,
         WA_Title,      APP_NAME " Preferences",
         WA_IDCMP,      BUTTONIDCMP|STRINGIDCMP|MXIDCMP|CYCLEIDCMP|IDCMP_ACTIVEWINDOW,
         WA_SmartRefresh,TRUE,
         WA_AutoAdjust,  TRUE,
         WA_Gadgets,      glist,
         TAG_END );

      if ( w )
      {
         char *pat;

         GT_RefreshWindow( w, NULL );

         if ( dispatch_sympat( w, stgad ) )
            pat = (char *)((struct StringInfo *)stgad->SpecialInfo)->Buffer;
         else
            pat = NULL;

         CloseWindow( w );

         // Use wurde gewählt
         if ( pat )
         {
            if ( *pat == '\0' )
               pat = NULL;

            SetRexxFunc( (UBYTE *)((struct StringInfo *)rxgad->SpecialInfo)->Buffer );
            SetPattern( pat );
            SetModeIdx( mode );
            SetEinheitIdx( unit );
            SetSortIdx( sort );
            SetSaveProf( safe );

            // Save wurde gewählt
            if ( SaveToolTypes )
            {
               WriteToolTypes( NULL );
               SaveToolTypes = FALSE;
            }

            // Neue Werte anzeigen
            UpdateModeUnits();

            if ( THE_BASE.pb_symlist )
               ShowSymbolList();
         }

      }
   }

   FreeGadgets( glist );       // Muß immer durchlaufen werden!! (gad == NULL)
}


