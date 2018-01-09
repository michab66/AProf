/* :ts=3
 * $RCSfile: pro.h,v $
 *
 * Definitionen für Amiga DOS Profiler
 *
 * $Revision: 1.1.1.1 $ $Date: 2002/05/31 23:15:55 $
 *
 * © 1993,94 Michael G. Binz
 */

#include <stdio.h>
#include <dos/dos.h>
#include <exec/types.h>

#include "p3text.h"


#define PRO_BUFFER   256
#define MAXSYMNAME   256      /* Maximale Symbollänge */


/*
 * Strukturdefinitionen
 */

/* Hält alle symbolbezogenen Informationen */
struct Symbol
{
   struct Symbol *s_next;
   ULONG          s_flags;        // Flags
   union BreakPoint *s_break;     // Zeiger auf zugehörigen BreakPoint
   ULONG          s_radr;         // Relative Adresse von 0x0000
   ULONG          s_cadr;         // Tatsaechliche Adresse
   USHORT         s_hunk;         // Hunknumber

   // Statistische Informationen
   ULONG          s_xcount;       // Anzahl der Aufrufe
   ULONG          s_xtim_inc;     // Gesamtlaufzeit
   ULONG          s_xtim_min;     // minimale Laufzeit
   ULONG          s_xtim_max;     // maximale Laufzeit
   UBYTE          s_name[1];      // Name der Funktion
};

#ifndef __cplusplus
typedef struct Symbol Symbol;
#endif


#define S_HIDE    ( 1<<0 )        /* Symbol wird nicht angezeigt */
#define S_ALIAS   ( 1<<1 )        /* Symbol ist Alias */
#define S_AHIDE   ( 1<<2 )        /* Symbol wurde bei Safe-Profiling entfernt */
#define S_NHIDE   ( 1<<3 )        /* Symbol wird immer angezeigt */

/* Hashtable */
#define HASHSIZE      1009        /* Muß Primzahl sein */




/* Rückgabe für First/NextSymbol */
struct SymInfo
{
   ULONG  si_hunk;                // Hunknummer
   ULONG  si_hunksize;            // Größe des Hunks
   ULONG  si_radr;                // Relative Adresse innerhalb des Hunks
   UBYTE *si_name;                // Name des Symbols
};

#ifndef __cplusplus
typedef struct SymInfo SymInfo;
#endif



union BreakPoint
{
   struct
   {
      struct Symbol    *bp_symbol;
      USHORT            bp_save;
      ULONG             bp_time;
      union BreakPoint *bp_next;
   } fix;

   struct
   {
      struct Symbol    *bp_symbol; // Zeiger auf BP Symbol
      USHORT            bp_save;   // Ursprünglicher Speicherinhalt
      ULONG             bp_time;   // Zwischenspeicher für Zeitwerte
      ULONG             bp_cadr;
   } temp;
};

#ifndef __cplusplus
typedef union BreakPoint BreakPoint;
#endif



struct ProfileBase
{
   ULONG          pb_base_time;    // Gesamtlaufzeit
   struct Symbol *pb_symlist;      // Zeiger auf Symbolliste
   BPTR           pb_seglist;      // Seglist
   BPTR           pb_lock;         // Lock auf Profilee
   char          *pb_comname;      // Name des Sklaven
   char          *pb_comline;      // Kommandozeile des Sklaven
   ULONG          pb_stacksize;    // Stackgröße für Sklaven
   TextObj        pb_display;      // Zeiger auf Anzeige
};

extern struct ProfileBase THE_BASE;



/* Prototypes */

void UpdateXTime( char *str );              //                   p3gui.c
void UpdateModeUnits( void );               //                   p3gui.c
void HandleGadgets( struct IntuiMessage * ); //                  p3gui.c
void ExitGadgets( struct Window * );        //                   p3gui.c
BOOL InitGadgets( struct Window *, APTR );  //                   p3gui.c

struct Symbol *Break2Symbol( ULONG );       //                   p3break.c

BOOL BreakSet( void );
BOOL UpdateSymlist( void );
int strncmpi( char *, char *, int );
char *strstri( char *, char * );
char *strins( char *s, int p, char *i );
char *stralloc( char * );
void p3qsort( void *[], int, int, int (*)(const void *, const void *) );
char *RequestFilenameR( void );             //                     p3funcs.c
char *RequestFilenameW( void );             //                     p3funcs.c

UWORD Table2Idx( char **, char * );         //                     p3funcs.c
char *Str2ToolType( char * );               //                     p2funcs.c

struct SymInfo *GetSymbol( FILE * );

void ShowSymbolList( void );                //                     p3symdis.c


#ifdef __cplusplus
extern "C" {
#endif

void ProfilerTrapHandler( void );           //                     p3trap.asm

void SetModeIdx( UWORD );                   //                     p3trap.asm
UWORD GetModeIdx( void );                   //                     p3trap.asm
char **GetModeStringTable( void );          //                     p3trap.asm
char *GetModeToolType( void );              //                     p3trap.asm

BPTR xSetStart( BPTR );                     //                     p3xseglist.asm

void TH_HandleTmpTrapE( ULONG );            //                     p3traphandler.c
BOOL TH_HandleFixTrapE( ULONG, ULONG, ULONG ); //                  p3traphandler.c
void TH_HandleTmpTrapI( ULONG );            //                     p3traphandler.c
BOOL TH_HandleFixTrapI( ULONG, ULONG, ULONG ); //                  p3traphandler.c
BOOL TH_CLearStack( void );                 //                     p3traphandler.c

UWORD uwGetTrapCommand( LONG );             //                     p3break.c
void InitTrapHandler( LONG, LONG );         //                     p3break.c

#ifdef __cplusplus
}
#endif


BOOL SetSaveProf( BOOL what );              //                     p3traphandler.c
BOOL GetSaveProf( void );                   //                     p3traphandler.c
char *GetSafeProfToolType( void );          //                     p3traphandler.c

UWORD GetEinheitIdx( void );                //                     p3symdis.c
void SetEinheitIdx( UWORD );                //                     p3symdis.c
char **GetEinheitStringTable( void );       //                     p3symdis.c
char *GetEinheitToolType( void );           //                     p3symdis.c
void SetSortIdx( UWORD idx );               //                     p3symdis.c
UWORD GetSortIdx( void );                   //                     p3symdis.c
char **GetSortStringTable( void );          //                     p3symdis.c
char *GetSortToolType( void );              //                     p3symdis.c
void SetPattern( char * );                  //                     p3symdis.c
char *GetPattern( void );                   //                     p3symdis.c
char *GetPatternToolType( void );           //                     p3symdis.c

void LoadExecutable( char * );
void men_open( void );
void men_save( void );                      // File/Save           p3funcs.c
void men_save_as( void );                   // File/Save as        p3funcs.c
void men_print( void );                     // File/Print          p3funcs.c

void men_help( void );                      // Misc/Help           p3amigaguide.c
void ProvideAProfHelp( char *node, long line ); //                 p3amigaguide.c

void men_reset( void );                     // File/Reset
void men_sympat( void );                    // Misc/Symbol filter
void pro_about( void );

void WriteToolTypes( struct IntuiMessage * ); /* Prefs/Save        p3main.c */
#ifdef SIM_CLI
void pro_debug( void );                     /*                     p3main.c */
#endif
void app_exit( BOOL * );
void app_find( void );
void app_fnext( void );
void app_refresh( void );
void p3err( const char *fmt, ... );
void pro_exec( void );
void men_execopt( void );

void SetPattern( char * );                  //                     p3pattern.c

UBYTE *GetRexxFunc( void );                 //                     p3rexx.c
void SetRexxFunc( UBYTE * );                //                     p3rexx.c
char *GetRexxToolType( void );              //                     p3rexx.c
BOOL RexxInit( void );                      //                     p3rexx.c
char *RexxUnmangle( char * );               //                     p3rexx.c

BOOL IsCodeHit( ULONG ret );                //                     p3load.c
void FreeSaveTable( void );                 //                     p3load.c
ULONG **CreateSaveTable( void );            //                     p3load.c

#ifdef __MAXON__
int stricmp( const char *, const char * );
void NewFreeDosObject( ULONG, void * );
#endif
