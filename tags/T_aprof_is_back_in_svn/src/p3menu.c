/*:ts=3
 * $RCSfile: p3menu.c,v $
 *
 * Menu definitions for AProf
 *
 * $Revision: 1.2 $ $Date: 2008/04/10 18:41:21 $
 *
 * Copyright © 1993,94 Michael G. Binz
 */

#include <stdio.h>
#include <exec/types.h>
#include <dos/dos.h>
#include <clib/gadtools_protos.h>
#include "pro.h"
#include "dsp.h"



// Dummy function
static void fnull( void ){ return; }



struct NewMenu stx_newmenu[] =
{
   { NM_TITLE, "Files",           NULL, 0, 0, NULL },
   {  NM_ITEM, "Open...",         "O",  0, 0, men_open },
   {  NM_ITEM, "Save",            "W",  0, 0, men_save },
   {  NM_ITEM, "Save as...",      NULL, 0, 0, men_save_as },
   {  NM_ITEM, "Reset",           "R",  0, 0, men_reset },
   {  NM_ITEM, "Print",           "P",  0, 0, men_print },
   {  NM_ITEM, NM_BARLABEL,       NULL, 0, 0, NULL },
   {  NM_ITEM, "Exit",            "Q",  0, 0, app_exit },

   { NM_TITLE, "Action",          NULL, 0, 0, NULL },
   {  NM_ITEM, "Start",           "B",  0, 0, pro_exec },

   { NM_TITLE, "Data",            NULL, 0, 0, NULL },
   {  NM_ITEM, "Exec details...", "E",  0, 0, men_execopt },
   {  NM_ITEM, "Preferences...",  "P",  0, 0, men_sympat },

   { NM_TITLE, "Move",            NULL, 0, 0, NULL },
   {  NM_ITEM, "Find...",         "F",  0, 0, app_find },
   {  NM_ITEM, "Find next",       "N",  0, 0, app_fnext },
   {  NM_ITEM, "Top",             "<",  0, 0, DspTop },
   {  NM_ITEM, "Bottom",          ">",  0, 0, DspBottom },
   {  NM_ITEM, "Page up",         "U",  0, 0, DspPgUp },
   {  NM_ITEM, "Page down",       "D",  0, 0, DspPgDown },

   { NM_TITLE, "Misc",            NULL, 0, 0, NULL },
   {  NM_ITEM, "Help...",         NULL, 0, 0, men_help },
   {  NM_ITEM, "Refresh Window",  "*",  0, 0, app_refresh },
   {  NM_ITEM, NM_BARLABEL,       NULL, 0, 0, NULL },
   {  NM_ITEM, "About...",        "?",  0, 0, pro_about },
#if 0
   {  NM_ITEM, "Gnah...",         NULL, 0, 0, pro_debug }, /* p3main.c */
#endif

   { NM_END,   NULL,    NULL, 0, 0, NULL  }
};
