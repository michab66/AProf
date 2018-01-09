/**************************************************************************
 * $RCSfile: p3gui.c,v $
 *
 * Gadgetdefinitionen für das Hauptfenster
 *
 * $Revision: 1.1.1.1 $ $Date: 2002/05/31 23:15:40 $
 *
 * © 1993,94 Michael G. Binz
 */

#include <stdlib.h>
#include <string.h>

#include <graphics/text.h>
#include <clib/gadtools_protos.h>
#include <clib/intuition_protos.h>
#include <clib/graphics_protos.h>

#include <pragmas/gadtools_lib.h>
#include <pragmas/intuition_lib.h>
#include <pragmas/graphics_lib.h>

#include "dsp.h"
#include "pro.h"


// Tabellenkopf
#define header_txt   "Function                     HitCnt  Per Call      Over       Min       Max"

// System default text
static struct TextAttr DefaultTxA;


// In UserID ist die Zeile codiert, in der das Gadget erscheinen soll
// UserID = 0 => Ende der Liste
// Die folgenden enum-Definitionen müssen parallel zur Liste laufen!

enum gads { gd_mode=0, gd_units, gd_sort, gd_xtime, gd_leiste, gd_header=11 };

static struct NewGadget p3gads[] = {

   // Textdisplay Gadgets im Hauptfenster
   {  59, 0,  90, 0, "Mode:",      NULL, 1, 0, NULL, NULL },
   { 213, 0, 100, 0, "Units:",     NULL, 1, 0, NULL, NULL },
   { 369, 0,  90, 0, "Sort:",      NULL, 1, 0, NULL, NULL },
   { 515, 0, 100, 0, "Time:",      NULL, 1, 0, NULL, NULL },

   // Buttongadgets im Hauptfenster
   {   6, 0,  82, 0, "Open...",    NULL, 2, 0, NULL, men_open },
   {  94, 0,  82, 0, "Start",      NULL, 2, 0, NULL, pro_exec },
   { 182, 0,  82, 0, "Data...",    NULL, 2, 0, NULL, men_execopt },
   { 270, 0,  82, 0, "Reset",      NULL, 2, 0, NULL, men_reset },
   { 358, 0,  82, 0, "Find...",    NULL, 2, 0, NULL, app_find },
   { 446, 0,  82, 0, "FNext",      NULL, 2, 0, NULL, app_fnext },
   { 534, 0,  82, 0, "Prefs...",   NULL, 2, 0, NULL, men_sympat },

   // Tabellenkopf
   {    -3, 0,  1,  1, header_txt, NULL, 3, PLACETEXT_RIGHT | NG_HIGHLABEL, NULL, NULL },

   // Endekennzeichen - alles Null
   {    0, 0,  0,   0, NULL,        NULL,   0, 0, NULL, NULL }
};


// Hilfsvariable für Aufbau und abschließende Freigabe der Gadgetliste
static struct Gadget *glist;

// Zeiger auf Gadgets, die für SetAttr() benötigt werden
static struct Gadget *gp_xtime, *gp_mode, *gp_unit, *gp_sort;



// TODO: Auch Gadgetbreite sollte von dieser Funktion berechnet werden
static void CalculateGadgetLayout( struct Window *win, APTR visual, struct NewGadget *gl )
{
   WORD TopFrameHi = win->BorderTop;
   WORD prLineN;   // Letzte bearbeitete Zeile (Nummer)
   WORD crLineN;   // Gerade bearbeitete Zeile (Nummer)
   WORD crLineP;   // ditto.                   (Pixels)
   WORD crColmP;   // Aktuelle Spaltenposition (Pixels)
   ULONG GadHi;

   // TextAttr vom Default font holen
   AskFont( win->RPort, &DefaultTxA );

   // Berechnung der Gadgethöhe in Abhängigkeit vom default font (1.5 * Zeilenhöhe)
   GadHi = DefaultTxA.ta_YSize + (DefaultTxA.ta_YSize>>1);

   for ( prLineN = 0, crLineN = gl->ng_GadgetID ; crLineN ; crLineN = (++gl)->ng_GadgetID )
   {
      if ( prLineN != crLineN )
      {
         // Startspalte
         crColmP = INTERWIDTH / 2;

         // Aktuelle Pixelzeile berechnen
         crLineP = win->BorderTop
                     + (crLineN * INTERHEIGHT)
                     + ((crLineN-1) * GadHi);

         // prLineN auf aktuellen Wert setzen
         prLineN = crLineN;
      }

      gl->ng_TopEdge  = crLineP;
      gl->ng_Height   = GadHi;
      gl->ng_TextAttr = &DefaultTxA;      // System default text eintragen
      gl->ng_VisualInfo = visual;
   }

   // Anzeigebereich des Console.device einschränken
   DspSetTopOffset( crLineP + GadHi );

   /* Minimale Fensterhöhe so einstellen, daß immer die
    * ersten Zeilen der Tabelle sichtbar bleiben
    */
   WindowLimits( DspGetWindow(), 0, crLineP + 4*GadHi, 0, 0 );
}



void UpdateModeUnits( void )
{
#define TXT_LEN   13

   char *poi;
   static char mode_txt[ TXT_LEN ];
   static char unit_txt[ TXT_LEN ];
   static char sort_txt[ TXT_LEN ];

   // Texte in internen Puffer kopieren
   strncpy( sort_txt, (GetSortStringTable())[GetSortIdx()], TXT_LEN-1 );
   strncpy( mode_txt, (GetModeStringTable())[GetModeIdx()], TXT_LEN-1 );
   strncpy( unit_txt, (GetEinheitStringTable())[GetEinheitIdx()], TXT_LEN-1 );

   // Beide am ersten Space abschneiden
   if ( poi = strchr( mode_txt, ' ' ) )
      *poi = '\0';
   if ( poi = strchr( unit_txt, ' ' ) )
      *poi = '\0';
   if ( poi = strchr( sort_txt, ' ' ) )
      *poi = '\0';

   // Darstellen
   GT_SetGadgetAttrs( gp_sort, DspGetWindow(), NULL,
      GTTX_Text,   sort_txt,
      TAG_END );

   GT_SetGadgetAttrs( gp_mode, DspGetWindow(), NULL,
      GTTX_Text,    mode_txt,
      TAG_END );

   GT_SetGadgetAttrs( gp_unit, DspGetWindow(), NULL,
      GTTX_Text,   unit_txt,
      TAG_END );
}



void UpdateXTime( char *str )
{
   GT_SetGadgetAttrs( gp_xtime, DspGetWindow(), NULL, GTTX_Text, str, TAG_END );
}



void ExitGadgets( struct Window *window )
{
   RemoveGList( window, glist, -1 );
   FreeGadgets( glist );
}



BOOL InitGadgets( struct Window *window, APTR visual )
{
   struct Gadget *gad;
   int i;

   // Initialisierung der GadgetPositionen
   CalculateGadgetLayout( window, visual, p3gads );

   gad = CreateContext( &glist );

   // Definition der Anzeigegadgets
   gp_mode = gad = CreateGadget( TEXT_KIND, gad, &p3gads[gd_mode],
      GTTX_Border,   TRUE,
      TAG_END );

   gp_unit = gad = CreateGadget( TEXT_KIND, gad, &p3gads[gd_units],
      GTTX_Border,   TRUE,
      TAG_END );

   gp_sort = gad = CreateGadget( TEXT_KIND, gad, &p3gads[gd_sort],
      GTTX_Border,   TRUE,
      TAG_END );

   gp_xtime = gad = CreateGadget( TEXT_KIND,  gad, &p3gads[gd_xtime],
      GTTX_Border,    TRUE,
      TAG_END );

   // Tabellenkopf (nicht aktiv)
   gad = CreateGadget( TEXT_KIND,  gad, &p3gads[gd_header],
      TAG_END );

   // Mittlere Reihe (Quick ...)
   for ( i = gd_leiste ; p3gads[i].ng_GadgetID == 2 ; i++ )
      gad = CreateGadget( BUTTON_KIND, gad, &p3gads[i], TAG_END );

   // Wenn alles klargegangen ist, dann darstellen
   if ( glist )
   {
      AddGList( window, glist, -1, -1, NULL );
      RefreshGList( glist, window, NULL, -1 );

      // Anzeige initialisieren ( Mode & Units & Sort )
      UpdateModeUnits();

      return TRUE;
   }
   else
      return FALSE;
}



