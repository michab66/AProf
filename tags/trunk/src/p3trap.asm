; :ts=3
;
; $RCSfile: p3trap.asm,v $
;
; p3trap.asm: Trap-Handler
;
; $Revision: 1.1.1.1 $ $Date: 2002/05/31 23:15:52 $
;
; © 1991-94 Michael G. Binz
;

;
; Importierte Funktionen
;
      xref     _OldTrap
      xref     _TH_HandleFixTrapI,_TH_HandleTmpTrapI
      xref     _TH_HandleFixTrapE,_TH_HandleTmpTrapE
      xref     _StartTimerCIA,_StopTimerCIA   ;
      xref     _SysBase

; 68040 Support
LVO_CacheClearU equ  -636            ; Exec library vector offset

Cregs    reg   d1/d2/a0-a3
Trapn    equr  d0                    ; Aktuelle Trapnummer
Oretn    equr  d1                    ; Ursprüngliche Rücksprungadresse
Nretn    equr  d2                    ; Neue Rücksprungadresse
Status   equr  d3                    ; Statusregister

      dseg

; Tabelle der C Traphandler
FixTab   dc.l  _TH_HandleFixTrapI
         dc.l  _TH_HandleFixTrapE

TmpTab   dc.l  _TH_HandleTmpTrapI
         dc.l  _TH_HandleTmpTrapE

FktIdx   ds.w  1                      ; Selektierte Funktion

; TrapDaten

trapFix  ds.l  1                      ; Exec Trap Nummer für FixTrap
trapTmp  ds.l  1                      ; Exec Trap Nummer für TempTrap
trapCom  ds.w  1                      ; Trap Commando

pfix     ds.l  1                      ; Letzter Fixtrap
regs     ds.l  9                      ; Zwischenspeicher für Register

return   ds.w  1                      ; Wird in FixTrap() verwendet

      cseg



; Traps auseinanderpfriemeln:
;
; Stack Frame:   sp+00  Trapnummer
;                 +04  Statusregister
;                 +06  PC
;

   xdef _ProfilerTrapHandler

_ProfilerTrapHandler
      movem.l   d0-d3/a0-a3,regs          ; Register sichern

      jsr      _StopTimerCIA              ; CIA Timer abschalten

      move.l   (sp)+,Trapn                ; Trapnummer vom Stack
      move.w   (sp)+,Status               ; Statusregister sichern
      move.l   (sp)+,Oretn                ; Rücksprungadresse sichern
      move.l   Oretn,Nretn                ;
      subq.l   #2,Nretn                   ; Ursprungsadresse berechnen

      cmp.l    trapTmp,Trapn              ; Temp Trap?
      bne      1$
      bsr      TempTrap
      bra      GoodEnd

1$    cmp.l    trapFix,Trapn              ; Fix trap?
      bne      2$
      bsr      FixTrap
      bra      GoodEnd

2$    cmp.l    #9,Trapn                   ; Trace trap
      bne      BadEnd
      bsr      TraceTrap

GoodEnd
      move.l   a6,-(sp)                   ; a6 sichern
      movea.l  _SysBase,a6                ; Exec Library
      jsr      LVO_CacheClearU(a6)        ; 68040 Cache zurückschreiben
      move.l   (sp)+,a6                   ; a6 restaurieren

      move.l   Nretn,-(sp)                ; Neuer Rücksprung
      move.w   Status,-(sp)               ; Statusregister zurück

      jsr      _StartTimerCIA             ; CIA Timer wieder an

      movem.l  regs,d0-d3/a0-a3           ; Register wiederherstellen
      rte

BadEnd
      move.l   Oretn,-(sp)
      move.w   Status,-(sp)               ; Statusregister zurück
      move.l   Trapn,-(sp)                ; Trapnummer zurück

      jsr      _StartTimerCIA             ; CIA Timer wieder an

      movem.l  regs,d0-d3/a0-a3           ; Register restaurieren
      move.l   _OldTrap,-(sp)             ; Sprung in alten Traphandler

_ProfilerTrapHandler_end
      rts                                 ; Rückkehr



TempTrap
      movem.l  Cregs,-(sp)
      move.l   Nretn,-(sp)                ; ... Übergeben

      movea.l  #TmpTab,a0                 ; Adresse der Tabelle nach a0
      move.w   FktIdx,d0                  ; Index nach d0
      asl.w    #2,d0                      ; sizeof( (* func) )
      ifnd     .a68k
      movea.l  (0,a0,d0.w),a1             ; Adresse holen - Siehe 68000 Ass. S 167
      endc
      ifd      .a68k
      movea.l  0(a0,d0.w),a1
      endc
      jsr      (a1)                       ; Anspringen

      addq.l   #4,sp                      ; Wieder runter
      movem.l  (sp)+,Cregs
TempTrap_end
      rts                                 ; Zurück



FixTrap
      move.l   Nretn,pfix                 ; Aktuelle Ursprungsadresse speichern
      move.l   usp,a0                     ; User Stack Pointer holen

      movem.l  Cregs,-(sp)                ; Register vor Aufruf der C-Fkt. sichern

      move.l   a0,-(sp)                   ; Wert des Stackpointer, ...
      move.l   (a0),-(sp)                 ; ...Returnadresse der Funktion...
      move.l   Nretn,-(sp)                ; ...und des Trap Übergeben

      movea.l  #FixTab,a0                 ; Adresse der Tabelle nach a1
      move.w   FktIdx,d0                  ; Index nach d0
      asl.w    #2,d0                      ; sizeof( (* func) )
      ifnd     .a68k
      movea.l  (0,a0,d0.w),a1             ; Adresse holen - Siehe 68000 Ass. S 167
      endc
      ifd      .a68k
      movea.l  0(a0,d0.w),a1
      endc
      jsr      (a1)                       ; Anspringen

      ifnd     .a68k
      lea.l    (12,sp),sp                 ; Argumente von Stack
      endc
      ifd      .a68k
      lea.l    12(sp),sp
      endc

      move.w   d0,return                  ; Wenn kein Trace erwünscht -> FALSE

      movem.l  (sp)+,Cregs                ; Register restaurieren

      move.l   d0,-(sp)                   ; d0 sichern
      move.l   return,d0                  ; Ergebnis des C Traphandler holen
      beq      1$                         ; Sprung, wenn FALSE == 0
      ori.w    #$8000,Status              ; Trace bit on
1$    move.l   (sp)+,d0
FixTrap_end
      rts                                 ; Zurück



TraceTrap
      move.l   pfix,a0
      move.w   trapCom,(a0)               ; Fixbreak wieder setzen
      andi.w   #$7fff,Status              ; Trace bit off
      move.l   Oretn,Nretn                ; An aktueller Adresse weiter
TraceTrap_end
      rts                                 ; Zurück


;
; _InitTrapHandler - Übergabe der zu verwendenden Traps
;
; Prototype:   void InitTrapHandler( LONG trapFix, LONG trapTmp );
;
      xref      _uwGetTrapCommand

      xdef      _InitTrapHandler
_InitTrapHandler
      move.l   #32,d0
      ifnd     .a68k
      add.l    (8,sp),d0
      endc
      ifd      .a68k
      add.l    8(sp),d0
      endc
      move.l   d0,trapTmp

      move.l   #32,d0                     ; Offset der Trapnummer für TrapHandler
      ifnd     .a68k
      add.l    (4,sp),d0                  ; Addieren
      endc
      ifd      .a68k
      add.l    4(sp),d0
      endc
      move.l   d0,trapFix                 ; Sichern

      ifnd     .a68k
      move.l   (4,sp),-(sp)               ; Fixtrap in trap #xx Befehl umrechnen...
      endc
      ifd      .a68k
      move.l   4(sp),-(sp)
      endc

      jsr      _uwGetTrapCommand          ;
      move.w   d0,trapCom                 ; ...und nach trapCom sichern
      addq.l   #4,sp                      ; Argumente vom Stack
_InitTrapHandler_end
      rts



      dseg

; Strings für Anzeige
IStr      dc.b      "Combined",0
EStr      dc.b      "Separate",0

; Tabelle für Anzeige
DspTable  dc.l      IStr,EStr,0

; ToolType
ToolT     dc.b      "PMODE",0

      cseg

;
; _GetModeGadgetTable - Liefert Zeiger auf die Tabelle mit GadgetTexten
;
; Prototype: char **GetModeStringTable( void );
;
         xdef      _GetModeStringTable
_GetModeStringTable
         move.l   a0,-(sp)
         lea.l    DspTable,a0
         move.l   a0,d0
         move.l   (sp)+,a0
_GetModeStringTable_end
         rts




;
; _SetModeIdx - setzt den Index für C TrapHandler
;
; Prototype:    void SetModeIdx( UWORD );
;
         xdef      _SetModeIdx
_SetModeIdx
         ifnd     .a68k
         move.w   (4,sp),FktIdx               ; Argument von Stack
         endc
         ifd      .a68k
         move.w   4(sp),FktIdx
         endc
_SetModeIdx_end
         rts




;
; _GetModeIdx - liest den Index des eingestellten TrapHandlers
;
; Prototype:   UWORD GetModeIdx( void );
;
         xdef      _GetModeIdx
_GetModeIdx
         move.w   FktIdx,d0
_GetModeIdx_end
         rts



; _GetModeToolType - ToolType für Sicherung des Mode
;
; Prototype: char *GetModeToolType( void );
;
         xdef     _GetModeToolType
_GetModeToolType
         move.l   a0,-(sp)
         lea.l    ToolT,a0
         move.l   a0,d0
         move.l   (sp)+,a0
_GetModeToolType_end
         rts


         end                              ; Debug Routinen nicht übersetzen



; _DebugBlinkX - Bildschirm blitzt in der bezeichneten Farbe
;
; Funktionen können aus jedem Kontext aufgerufen werden (Interrput, Trap, etc.)
;
; Prototype: void DebugBlinkX( void );
;
         xdef _DebugBlinkBlue
_DebugBlinkBlue
         move.l   d0,-(sp)
         move.l   #2000,d0
1$       move.w   #$f,$dff180
         subq.l   #1,d0
         bne      1$
         move.l   (sp)+,d0
_DebugBlinkBlue_end
         rts



         xdef _DebugBlinkGreen
_DebugBlinkGreen
         move.l   d0,-(sp)
         move.l   #2000,d0
1$       move.w   #$f0,$dff180
         subq.l   #1,d0
         bne      1$
         move.l   (sp)+,d0
_DebugBlinkGreen_end
         rts



         xdef _DebugBlinkRed
_DebugBlinkRed
         move.l   d0,-(sp)
         move.l   #2000,d0
1$       move.w   #$f00,$dff180
         subq.l   #1,d0
         bne      1$
         move.l   (sp)+,d0
_DebugBlinkRed_end
         rts

         end                                 ; Und Schulz!

