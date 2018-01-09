/** :ts=3 *****************************************************************
 *  $RCSfile: p3main.c,v $ - Main-Funktion des Profilers
 *
 *  $Revision: 1.1.1.1 $ $Date: 2002/05/31 23:15:43 $
 *
 *  © 1992-94 Michael G. Binz
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <exec/alerts.h>
#include <dos/dostags.h>
#include <devices/conunit.h>
#include <workbench/startup.h>
#include <workbench/workbench.h>
#include <clib/gadtools_protos.h>
#include <clib/asl_protos.h>
#include <clib/exec_protos.h>
#include <clib/intuition_protos.h>
#include <clib/console_protos.h>
#include <clib/dos_protos.h>
#include <clib/icon_protos.h>

#include <pragmas/asl_lib.h>
#include <pragmas/dos_lib.h>
#include <pragmas/intuition_lib.h>
#include <pragmas/exec_lib.h>
#include <pragmas/icon_lib.h>

#include "version.h"
#include "pro.h"
#include "dsp.h"
#include "p3text.h"


#define MIN_VERSION     37  /* 2.04 */
#define MIN_REXX_VER    36
#define MIN_AGUIDE_VER  0

#define STX_LEFT        0   /* Konfigurierbar über WINDIM Tooltype */
#define STX_TOP         0
#define STX_WIDTH       640
#define STX_HEIGHT      200

// Maximum von zwei Werten
#define Max( a, b )     (((a) > (b))?(a):(b))

extern struct DosLibrary *DOSBase;
struct Library         *GadToolsBase;
struct IntuitionBase   *IntuitionBase;
struct Library         *IconBase;
struct Library         *AslBase;
struct Library         *GfxBase;
APTR                    AslReq;
struct Library         *AmigaGuideBase;
struct Library         *RexxSysBase;

/**********************************************************/
struct ProfileBase THE_BASE;
/**********************************************************/



/* exit_wrong_libraries
 *
 * Erledigt Programmabbruch für den Fall, daß AProf unter
 * einer BS Version < 2.04 gestartet wurde oder wichtige
 * Libraries nicht geöffnet werden konnten.
 *
 * Diese Funktion zeigt unter allen BS Versionen einen
 * Requester mit einer Fehlermeldung an. Sollte
 * intuition nicht verwendbar sein, erfolgt die Anzeige
 * eines 'recoverable alert'
 */
static void exit_wrong_libraries( void )
{
   static struct IntuiText errtxt = {
      0, 0, JAM1, 20, 20, NULL, "Need Kick/WB 2.04 or better", NULL
   };
   static struct IntuiText boolTextNeg = {
      0, 0, JAM1, 4, 6, NULL, "Cancel", NULL
   };

   if ( !IntuitionBase )
   {
      // Irgendeine intuition.library da?
      IntuitionBase = (struct IntuitionBase *)OpenLibrary( "intuition.library", 0 );

      if ( !IntuitionBase )
         // Recoverable Alert: Keine intuition.library
         Alert( AT_Recovery|AG_OpenLib|AO_Intuition );
      else
         // AutoRequest ist in allen intuition.librarys vorhanden!
         AutoRequest( NULL, &errtxt, NULL, &boolTextNeg, 0, 0, 300, 50 );
   }

   exit( RETURN_FAIL );
}



/* CloseLibs
 *
 * Schliessen aller geöffneten Libraries.
 */
static void CloseLibs( void )
{
   if ( GadToolsBase )
      CloseLibrary( GadToolsBase );

   if ( IntuitionBase )
      CloseLibrary( (struct Library *)IntuitionBase );

   if ( IconBase )
      CloseLibrary( IconBase );

   if ( GfxBase )
      CloseLibrary( GfxBase );

   if ( AslBase )
   {
      if ( AslReq )
         FreeAslRequest( AslReq );

      CloseLibrary( AslBase );
   }

   if ( AmigaGuideBase )
      CloseLibrary( AmigaGuideBase );

   if ( RexxSysBase )
      CloseLibrary( RexxSysBase );
}




static BOOL OpenLibs( void )
{
   if ( NULL == ( GadToolsBase = OpenLibrary( "gadtools.library", MIN_VERSION ) ) )
      return TRUE;

   if ( NULL == ( IntuitionBase = (struct IntuitionBase *)OpenLibrary( "intuition.library", MIN_VERSION ) ) )
      return TRUE;

   if ( NULL == ( IconBase = OpenLibrary( "icon.library", MIN_VERSION ) ) )
      return TRUE;

   if ( NULL == ( GfxBase = OpenLibrary( "graphics.library", MIN_VERSION ) ) )
      return TRUE;

   if ( AslBase = OpenLibrary( (UBYTE *)AslName, MIN_VERSION ) )
   {
      AslReq = AllocAslRequestTags( ASL_FileRequest,
         ASL_Hail,   APP_NAME " File Request",
         TAG_DONE );
   }

   if ( AslBase == NULL || AslReq == NULL )
      return TRUE;

   /* Die folgenden Bibliotheken verursachen keinen Abbruch mehr,
    * wenn sie nicht geöffnet werden können
    */

   // Optionale Bibliothek für Hilfesystem
   if ( NULL == ( AmigaGuideBase = OpenLibrary( "amigaguide.library", MIN_AGUIDE_VER ) ) )
   {
      extern struct NewMenu stx_newmenu[];
      struct NewMenu *nm = stx_newmenu;

      // Spricht ein Menüeintrag die Hilfefunktion an?
      while ( nm->nm_Type != NM_END && nm->nm_UserData != men_help )
         nm++;

      // Hilfefunktion deaktivieren
      if ( nm->nm_Type != NM_END && nm->nm_UserData == men_help )
         nm->nm_Flags |= NM_ITEMDISABLED;
   }

   if ( RexxSysBase = OpenLibrary( "rexxsyslib.library", MIN_REXX_VER ) )
      // Rexx initialisieren
      RexxInit();
   /*
    * TODO: RexxFunc Gadget sperren
    */

   /* Kein Fehler aufgetreten */
   return FALSE;
}



/* extract_name
 *
 * Wurde eine Datei über Kommandozeile, DefaultTool oder ExtendedSelect
 * gewählt, dann erfolgt Rückgabe von Zeiger auf den Namen sonst NULL
 */
static char *extract_name( int c, char *v[] )
{
   static char filename[ PRO_BUFFER ];

   if ( c )                        // CLI
   {
      /* Nur erste Datei auf Kommandozeile!
       * argv[1] kann nur gültig oder NULL sein
       */
      return v[1];
   }
   else                            // Workbench
   {
      struct WBStartup *wbst;
      struct WBArg     *wbag;
      int               curarg;   // Current Argument

      wbst = (struct WBStartup *)v;
      wbag = wbst->sm_ArgList;

      if ( wbst->sm_NumArgs > 1 )
      {
         // Überspringe Argumente solange nicht wa_Lock und wa_Name
         // belegt sind
         for ( curarg = 1 ;
            (curarg < wbst->sm_NumArgs) && !wbag[curarg].wa_Lock && !wbag[curarg].wa_Name;
            ++curarg )
            ; // Gültiges Argument suchen

         if ( curarg < wbst->sm_NumArgs )
         {
            if ( NameFromLock( wbag[curarg].wa_Lock, filename, PRO_BUFFER ) )
            {
               if ( AddPart( filename, wbag[curarg].wa_Name, PRO_BUFFER ) && filename[0] )
                  return filename;
            }
         }
      }
   }

   return NULL;
}





static void getwindim( int *h, int *w, int *l, int *t )
{
   struct DiskObject *dso;

   // Initialisieren der Werte
   *h = *w = *l = *t = 0;

   if ( dso = GetDiskObject( "PROGDIR:" APP_NAME ) )
   {
      char *tt;

      if ( tt = FindToolType( (UBYTE **)dso->do_ToolTypes, GetDspToolType() ) )
      {
         int ih, iw, il, it, num;

         if ( 4 == sscanf( tt, "%d/%d/%d/%d", &il, &it, &iw, &ih ) )
            *h = ih, *w = iw, *l = il, *t = it;
      }

      FreeDiskObject( dso );
   }
}





static void ReadToolTypes( void )
{
   struct DiskObject *dso;

   if ( dso = GetDiskObject( "PROGDIR:" APP_NAME ) )
   {
      char *tt;

      if ( tt = FindToolType( (UBYTE **)dso->do_ToolTypes, GetEinheitToolType() ) )
         SetEinheitIdx( Table2Idx( GetEinheitStringTable(), tt ) );

      if ( tt = FindToolType( (UBYTE **)dso->do_ToolTypes, GetModeToolType() ) )
         SetModeIdx( Table2Idx( GetModeStringTable(), tt ) );

      if ( tt = FindToolType( (UBYTE **)dso->do_ToolTypes, GetPatternToolType() ) )
         SetPattern( tt );

      if ( tt = FindToolType( (UBYTE **)dso->do_ToolTypes, GetSortToolType() ) )
         SetSortIdx( Table2Idx( GetSortStringTable(), tt ) );

      if ( tt = FindToolType( (UBYTE **)dso->do_ToolTypes, GetRexxToolType() ) )
         SetRexxFunc( tt );

      if ( tt = FindToolType( (UBYTE **)dso->do_ToolTypes, GetSafeProfToolType() ) )
      {
         if ( !stricmp( "TRUE", tt ) || !stricmp( "ON", tt ) )
            SetSaveProf( TRUE );
         else if ( !stricmp( "FALSE", tt ) || !stricmp( "OFF", tt ) )
            SetSaveProf( FALSE );
      }

      FreeDiskObject( dso );
   }
}



void WriteToolTypes( struct IntuiMessage *dummy )
{
   struct DiskObject *dso;
   char *string;
   char line[100];
   TextObj toolt = TxtAlloc();
   char **tmp;

   if ( !toolt )
   {
      p3err( "Not enough memory to save prefs" );
      return;
   }

   if ( NULL == (dso = GetDiskObject( "PROGDIR:" APP_NAME ) ) )
   {
      p3err( "Can't save settings - icon not found" );
      TxtFree( toolt );
      return;
   }

   sprintf( line, "%s=%s", GetDspToolType(), GetDspToolSize() );
   toolt = TxtPutLine( toolt, line );

   sprintf( line, "%s=%s",
      GetEinheitToolType(),
      Str2ToolType( (GetEinheitStringTable())[ GetEinheitIdx() ] ) );
   toolt = TxtPutLine( toolt, line );

   sprintf( line, "%s=%s",
      GetModeToolType(),
      Str2ToolType( (GetModeStringTable())[ GetModeIdx() ] ) );
   toolt = TxtPutLine( toolt, line );

   if ( string = GetPattern() )
   {
      sprintf( line, "%s=%s", GetPatternToolType(), string );
      toolt = TxtPutLine( toolt, line );
   }

   sprintf( line, "%s=%s",
      GetSortToolType(),
      Str2ToolType( (GetSortStringTable())[ GetSortIdx() ] ) );
   toolt = TxtPutLine( toolt, line );

   if ( string = GetRexxFunc() )
   {
      sprintf( line, "%s=%s", GetRexxToolType(), string );
      toolt = TxtPutLine( toolt, line );
   }

   sprintf( line, "%s=%s", 
      GetSafeProfToolType(), 
      GetSaveProf() ? "TRUE" : "FALSE" );
   toolt = TxtPutLine( toolt, line );

   toolt = TxtPutLine( toolt, NULL );

   if ( !TxtError( toolt ) )
   {
      /* ToolTypes setzen */
      tmp = dso->do_ToolTypes;
      dso->do_ToolTypes = (char **)toolt;

      if ( PutDiskObject( "PROGDIR:" APP_NAME, dso ) )
         p3err( "Saved current settings" );
      else
         p3err( "Write to " APP_NAME ".info failed" );

      dso->do_ToolTypes = tmp;
   }
   else
      p3err( "Error while creating prefs array" );

   /* Speicher freigeben */
   TxtFree( toolt );
   FreeDiskObject( dso );
}



/* AppIDCMPHandler
 *
 * Erweiterung des in dsp.c verwendeten Event Handlers
 */
static void p3IDCMPHandler( struct IntuiMessage *msg )
{
   switch ( msg->Class )
   {
      case IDCMP_RAWKEY:
         // 0x5f == Hilfetaste
         if ( 0x5f == msg->Code )
            men_help();
         break;

      default:
         break;
   }
}





/* Anzeige eines Fehlertextes
 *
 * fmt: format string wie printf
 */
void p3err( const char *fmt, ... )
{
   va_list args;

   if ( DspGetWindow() )
   {
      char *msg;

      if ( msg = (char *)malloc( PRO_BUFFER ) )
      {
         va_start( args, fmt );
         vsprintf( msg, fmt, args );
         DspMessage( msg );
         va_end( args );
         free( msg );
      }
      else
         DspMessage( "*** Error ***" );
   }
}





#ifdef SIM_CLI
/* DestroyCli
 *
 * Gibt Cli-Struktur frei. Wird von atexit ausgeführt.
 */
static void DestroyCli( void )
{
   struct Process *p;
   BPTR file;

   if ( file = SelectOutput( 0 ) )
   {
      SelectInput( 0 );
      Close( file );
   }

   Forbid();
   p = (struct Process *)FindTask( NULL );
   if ( p && Cli() )
   {
      NewFreeDosObject( DOS_CLI, Cli() );
      p->pr_CLI = 0;
      ((struct MsgPort **)
         BADDR( DOSBase->dl_Root->rn_TaskArray ))[ p->pr_TaskNum ] = NULL;
      p->pr_TaskNum = 0;
   }
   Permit();
}





BOOL CreateCli( void )
{
   const int DosBufferLen = 50;
   BPTR cli;

   // Neue Cli-Struktur von DOS aufbauen lassen
   if ( cli = (BPTR)AllocDosObjectTags( DOS_CLI,
      ADO_DirLen,      DosBufferLen,
      ADO_CommNameLen, DosBufferLen,
      ADO_CommFileLen, DosBufferLen,
      ADO_PromptLen,   DosBufferLen,
      TAG_DONE ) )
   {
      struct Process *p;
      BPTR console;
      ULONG curcli;

      Forbid();
         p = (struct Process *)FindTask( NULL );

         /* Cli-Struktur in Process-Struktur hängen */
         p->pr_CLI = MKBADDR( cli );

         /* Freien CLI-Slot suchen ... */
         for ( curcli = 1; curcli <= MaxCli() ; curcli++ )
         {
            if ( !FindCliProc( curcli ) )
            {
               /* ... und aktuellem Prozess zuweisen */
               p->pr_TaskNum = curcli;
               ((struct MsgPort **)
                  BADDR( DOSBase->dl_Root->rn_TaskArray ))[ curcli ] = &(p->pr_MsgPort);
               break;
            }
         }
      Permit();

      /* TODO: Bei CON: sollte zumindest ein Name für
       * die Console angegeben werden.
       */
      if ( !Output() && !Input()
           && (console = Open( "CON:////AProf Shell/CLOSE/AUTO", MODE_READWRITE )) )
      {
         SelectOutput( console );
         SelectInput( console );
         SetProgramName( APP_NAME );

         return TRUE;
      }
   }

   return FALSE;
}



#if 0
void pro_debug( void )
{
   struct Process *p;

   Forbid();
   p = (struct Process *)FindTask( NULL );
   Permit();

   if ( p )
   {
      p3err( "pr_TaskNum = %lx, Cli = %lx, CliProc = %lx, PgmName = %lx",
         (ULONG)p->pr_TaskNum,
         (ULONG)Cli(),
         (ULONG)FindCliProc( p->pr_TaskNum ),
         (ULONG)SetProgramName( "Micha" ) );
   }
}
#endif
#endif



int main( int argc, char *argv[] )
{
#ifdef MCH_AMIGA
   extern int Enable_Abort;
#endif
   int stx_win_hi, stx_win_wi, stx_win_lf, stx_win_tp;
   char *fname;
   extern struct NewMenu stx_newmenu[];


#ifdef MCH_AMIGA
   Enable_Abort = 0;
#endif

   // Destruktor für Libraries
   atexit( CloseLibs );

   // Libraries öffnen
   if ( OpenLibs() )
      exit_wrong_libraries();

   // Fenstergröße aus Icon lesen
   getwindim( &stx_win_hi, &stx_win_wi, &stx_win_lf, &stx_win_tp );
      stx_win_hi = Max( stx_win_hi, STX_HEIGHT );
      stx_win_wi = Max( stx_win_wi, STX_WIDTH );
      stx_win_lf = Max( stx_win_lf, STX_LEFT );
      stx_win_tp = Max( stx_win_tp, STX_TOP );

   // Tooltypes lesen
   ReadToolTypes();

#ifdef SIM_CLI
   if ( NULL == Cli() )
      CreateCli();
#endif

   // Fenster öffnen
   if ( !DspInit( stx_win_lf, stx_win_tp, stx_win_wi, stx_win_hi, stx_newmenu, p3IDCMPHandler ) )
      return RETURN_FAIL;
   else
      // Fenster wird automatisch bei Programmende geschlossen
      atexit( DspExit );

   // Gadgets darstellen
   InitGadgets( DspGetWindow(), DspGetVisInfo() );

   // Copyright als Fenstertitel
   DspSetWindowTitle( APP_NAME "  Version " APP_VERSION  " © 1993,94 Michael G. Binz" );

   // Wenn Dateiname ermittelt werden kann, dann laden, sonst Request
   if ( fname = extract_name( argc, argv ) )
      LoadExecutable( fname );
   else if ( AmigaGuideBase )
      p3err( "Use help key for documentation" );

   // Hauptprogramm starten
   DspStart();
   p3err( "Closing down..." );   // Kann endlos lang dauern...

   // Gadgets entfernen
   ExitGadgets( DspGetWindow() );

#if SIM_CLI
   if ( 0 == argc && Cli() )
      DestroyCli();
#endif

   return RETURN_OK;
}



/* wbmain
 *
 * Einsprung für Start von der Workbench für
 * Maxon C++
 */
#ifdef __MAXON__
extern "C" void wbmain( WBStartup *ws )
{
   main( 0, (char **)ws );
}
#endif
