; :ts=3
;
; Profiler for the Amiga.
;
; p3xseglist.asm - First segment of an p3xseglist.
;
; Copyright © 1993,4 Michael G. Binz
;


      dseg

      xref     _SysBase
LVO_CacheClearU      equ   -636        ; Exec library vector offset

start_adr      ds.l 1                  ; Address of code start.
saveregs       ds.l 2                  ; Intermediate storage for register contents.


      cseg
;
; Start of pseudo segment <==============================================
;

xsegalign                              ; See Bantam Amiga DOS 2.04 S.203.
      cnop     0,4                     ; Start on long word address.
xsegsize
      ds.l     1                       ; Length of segment.
xsegnext                               ; <- Segment BPTR
      ds.l     1                       ; Next segment.

xsegcode
      movem.l  d7/a1,saveregs          ; Save used registers.

      ifnd     .a68k
      move.l   (4,sp),d7
      endc
      ifd      .a68k
      move.l   4(sp),d7                ; Save stack size.
      endc

      subq.l   #8,d7                   ; Size of return address and stack.
      move.l   d7,-(sp)                ; New stack size.
      move.l   start_adr,a1            ; Start address of segment list.
      jsr      (a1)                    ; Entry point into original segment liste.
      addq.l   #4,sp                   ; Restore stack size.
      movem.l  saveregs,d7/a1          ; Restore registers.
xsegcode_end
      rts                              ; Back to ROM.



;
; Initialise the pseudo segment.
;
; C prototype:   BPTR xSetStart( BPTR segl );
;
; In:    segl - Original segment list as received from LoadSeg();
; Out:   BPTR to new segment list including the leading pseudo segment.

      xdef     _xSetStart
_xSetStart
      ifnd     .a68k
      move.l   (4,sp),d0
      endc
      ifd      .a68k
      move.l   4(sp),d0               ; Argument to d0
      endc

      lea.l    xsegsize,a0             ; Load address.
      move.l   #(xsegcode_end-xsegsize),(a0)   ; Set code size.

      lea.l    xsegnext,a0             ; Load address.
      move.l   d0,(a0)                 ; Chain segments.

      addq.l   #1,d0                   ; Add 4 (BPTR!)
      lsl.l    #2,d0                   ; Transform BPTR to address...
      move.l   d0,start_adr            ; ...and save for call.

      move.l   a6,-(sp)                ; Save a6.
      movea.l  _SysBase,a6             ; Exec Library
      jsr      LVO_CacheClearU(a6)     ; 68040 Cache zurückschreiben
      move.l   (sp)+,a6                ; Restore a6.

      lea.l    xsegnext,a0             ; Load address of new SegList.
      move.l   a0,d0                   ; Move around, ...
      lsr.l    #2,d0                   ; ...transform to BPTR...
_xSetStart_end
      rts                              ; and return.

      end                              ; Module end.
