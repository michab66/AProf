HUNK_HEADER  tableSize=3
    hunk #0  1136 bytes
    hunk #1  24 bytes
    hunk #2  64 bytes

HUNK_CODE #00 (               ) Hunk=000003e9 Size=1136 bytes
 00.00000000  4eb9 Kernel%INIT2A7CA0FB  JSR     Kernel%INIT2A7CA0FB
 00.00000006  6100 0074                 BSR.W   TestAProf%INIT2AD8A924
 00.0000000a  7000                      MOVEQ.L #00,D0
 00.0000000c  4eb9 Kernel.Halt          JSR     Kernel.Halt
 00.00000012  4e71                      NOP     

*00.00000014 TestAProf.P1:
 00.00000014  4e55 0000                 LINK.W  A5,#0000
 00.00000018  4267                      CLR.W   -(A7)
 00.0000001a  3b7c 0001 fffe            MOVE.W  #0001,fffe(A5)
 00.00000020  0c6d 2710 fffe            CMPI.W  #2710,fffe(A5)
 00.00000026  6e00 000e                 BGT.W   00000036
 00.0000002a  3b6d fffe fffe            MOVE.W  fffe(A5),fffe(A5)
 00.00000030  526d fffe                 ADDQ.W  #1,fffe(A5)
 00.00000034  60ea                      BRA.B   00000020
 00.00000036  4e5d                      UNLK    A5
 00.00000038  4e75                      RTS     
 00.0000003a  4e71                      NOP     

*00.0000003c TestAProf.P2:
 00.0000003c  4e55 0000                 LINK.W  A5,#0000
 00.00000040  4267                      CLR.W   -(A7)
 00.00000042  3b7c 0001 fffe            MOVE.W  #0001,fffe(A5)
 00.00000048  0c6d 4e20 fffe            CMPI.W  #4e20,fffe(A5)
 00.0000004e  6e00 000e                 BGT.W   0000005e
 00.00000052  3b6d fffe fffe            MOVE.W  fffe(A5),fffe(A5)
 00.00000058  526d fffe                 ADDQ.W  #1,fffe(A5)
 00.0000005c  60ea                      BRA.B   00000048
 00.0000005e  4e5d                      UNLK    A5
 00.00000060  4e75                      RTS     
 00.00000062  4e71                      NOP     

*00.00000064 TestAProf.Cleanup:
 00.00000064  2f0c                      MOVE.L  A4,-(A7)
 00.00000066  49f9 TestAProf%VAR        LEA.L   TestAProf%VAR,A4
 00.0000006c  4e55 0000                 LINK.W  A5,#0000
 00.00000070  4e5d                      UNLK    A5
 00.00000072  285f                      MOVEA.L (A7)+,A4
 00.00000074  2f57 0004                 MOVE.L  (A7),0004(A7)
 00.00000078  588f                      ADDQ.L  #4,A7
 00.0000007a  4e75                      RTS     

*00.0000007c TestAProf%INIT2AD8A924:
 00.0000007c  49f9 TestAProf%VAR        LEA.L   TestAProf%VAR,A4
 00.00000082  4a14                      TST.B   (A4)
 00.00000084  6600 0022                 BNE.W   000000a8
 00.00000088  50d4                      ST.B    (A4)
 00.0000008a  2f0c                      MOVE.L  A4,-(A7)
 00.0000008c  4eb9 Kernel%INIT2A7CA0FB  JSR     Kernel%INIT2A7CA0FB
 00.00000092  285f                      MOVEA.L (A7)+,A4
 00.00000094  2f3c TestAProf.Cleanup    MOVE.L  #TestAProf.Cleanup,-(A7)
 00.0000009a  4eb9 Kernel.SetCleanup    JSR     Kernel.SetCleanup
 00.000000a0  6100 ff72                 BSR.W   TestAProf.P1
 00.000000a4  6100 ff96                 BSR.W   TestAProf.P2
 00.000000a8  4e75                      RTS     
 00.000000aa  4e71                      NOP     

*00.000000ac Kernel.FreeMemBlock:
 00.000000ac  4e55 fff8                 LINK.W  A5,#fff8
 00.000000b0  266d 0008                 MOVEA.L 0008(A5),A3
 00.000000b4  2b6b 0004 fff8            MOVE.L  0004(A3),fff8(A5)
 00.000000ba  2e2d fff8                 MOVE.L  fff8(A5),D7
 00.000000be  0807 0000                 BTST.L  #00,D7
 00.000000c2  6700 0014                 BEQ.W   000000d8
 00.000000c6  2e2d fff8                 MOVE.L  fff8(A5),D7
 00.000000ca  5387                      SUBQ.L  #1,D7
 00.000000cc  2b47 fffc                 MOVE.L  D7,fffc(A5)
 00.000000d0  50ad fffc                 ADDQ.L  #8,fffc(A5)
 00.000000d4  6000 003a                 BRA.W   00000110
 00.000000d8  2e2d fff8                 MOVE.L  fff8(A5),D7
 00.000000dc  0807 0001                 BTST.L  #01,D7
 00.000000e0  6700 0022                 BEQ.W   00000104
 00.000000e4  04ad 0000 000c 0008       SUBI.L  #0000000c,0008(A5)
 00.000000ec  2e2d 0008                 MOVE.L  0008(A5),D7
 00.000000f0  5087                      ADDQ.L  #8,D7
 00.000000f2  2647                      MOVEA.L D7,A3
 00.000000f4  2b53 fffc                 MOVE.L  (A3),fffc(A5)
 00.000000f8  06ad 0000 0014 fffc       ADDI.L  #00000014,fffc(A5)
 00.00000100  6000 000e                 BRA.W   00000110
 00.00000104  266d fff8                 MOVEA.L fff8(A5),A3
 00.00000108  2b53 fffc                 MOVE.L  (A3),fffc(A5)
 00.0000010c  50ad fffc                 ADDQ.L  #8,fffc(A5)
 00.00000110  226d 0008                 MOVEA.L 0008(A5),A1
 00.00000114  202d fffc                 MOVE.L  fffc(A5),D0
 00.00000118  2c54                      MOVEA.L (A4),A6
 00.0000011a  4eae ff2e                 JSR     ff2e(A6)
 00.0000011e  4e5d                      UNLK    A5
 00.00000120  2f57 0004                 MOVE.L  (A7),0004(A7)
 00.00000124  588f                      ADDQ.L  #4,A7
 00.00000126  4e75                      RTS     

*00.00000128 Kernel.DoCleanup:
 00.00000128  2f0c                      MOVE.L  A4,-(A7)
 00.0000012a  49f9 Kernel%VAR           LEA.L   Kernel%VAR,A4
 00.00000130  4e55 fff0                 LINK.W  A5,#fff0
 00.00000134  42ad fff0                 CLR.L   fff0(A5)
 00.00000138  0cad 0000 0005 fff0       CMPI.L  #00000005,fff0(A5)
 00.00000140  6e00 0020                 BGT.W   00000162
 00.00000144  2e2d fff0                 MOVE.L  fff0(A5),D7
 00.00000148  2c2c 0036                 MOVE.L  0036(A4),D6
 00.0000014c  0f06                      BTST.L  D7,D6
 00.0000014e  6700 000c                 BEQ.W   0000015c
 00.00000152  202d fff0                 MOVE.L  fff0(A5),D0
 00.00000156  2c54                      MOVEA.L (A4),A6
 00.00000158  4eae fea4                 JSR     fea4(A6)
 00.0000015c  52ad fff0                 ADDQ.L  #1,fff0(A5)
 00.00000160  60d6                      BRA.B   00000138
 00.00000162  266c 002e                 MOVEA.L 002e(A4),A3
 00.00000166  276c 000c 0032            MOVE.L  000c(A4),0032(A3)
 00.0000016c  266c 002e                 MOVEA.L 002e(A4),A3
 00.00000170  276c 0010 002e            MOVE.L  0010(A4),002e(A3)
 00.00000176  2b6c 0032 fff4            MOVE.L  0032(A4),fff4(A5)
 00.0000017c  42ac 0032                 CLR.L   0032(A4)
 00.00000180  4aad fff4                 TST.L   fff4(A5)
 00.00000184  6700 001a                 BEQ.W   000001a0
 00.00000188  486d 000c                 PEA.L   000c(A5)
 00.0000018c  246d fff4                 MOVEA.L fff4(A5),A2
 00.00000190  266a 0004                 MOVEA.L Kernel%VAR(A2),A3
 00.00000194  4e93                      JSR     (A3)
 00.00000196  266d fff4                 MOVEA.L fff4(A5),A3
 00.0000019a  2b53 fff4                 MOVE.L  (A3),fff4(A5)
 00.0000019e  60e0                      BRA.B   00000180
 00.000001a0  2b6c 0026 fffc            MOVE.L  0026(A4),fffc(A5)
 00.000001a6  42ac 0026                 CLR.L   0026(A4)
 00.000001aa  4aad fffc                 TST.L   fffc(A5)
 00.000001ae  6700 001a                 BEQ.W   000001ca
 00.000001b2  266d fffc                 MOVEA.L fffc(A5),A3
 00.000001b6  2b53 fff8                 MOVE.L  (A3),fff8(A5)
 00.000001ba  2f2d fffc                 MOVE.L  fffc(A5),-(A7)
 00.000001be  6100 feec                 BSR.W   Kernel.FreeMemBlock
 00.000001c2  2b6d fff8 fffc            MOVE.L  fff8(A5),fffc(A5)
 00.000001c8  60e0                      BRA.B   000001aa
 00.000001ca  2b6c 002a fffc            MOVE.L  002a(A4),fffc(A5)
 00.000001d0  42ac 002a                 CLR.L   002a(A4)
 00.000001d4  4aad fffc                 TST.L   fffc(A5)
 00.000001d8  6700 001a                 BEQ.W   000001f4
 00.000001dc  266d fffc                 MOVEA.L fffc(A5),A3
 00.000001e0  2b53 fff8                 MOVE.L  (A3),fff8(A5)
 00.000001e4  2f2d fffc                 MOVE.L  fffc(A5),-(A7)
 00.000001e8  6100 fec2                 BSR.W   Kernel.FreeMemBlock
 00.000001ec  2b6d fff8 fffc            MOVE.L  fff8(A5),fffc(A5)
 00.000001f2  60e0                      BRA.B   000001d4
 00.000001f4  4a2c 0014                 TST.B   0014(A4)
 00.000001f8  6700 0012                 BEQ.W   0000020c
 00.000001fc  2c54                      MOVEA.L (A4),A6
 00.000001fe  4eae ff7c                 JSR     ff7c(A6)
 00.00000202  226c 001e                 MOVEA.L 001e(A4),A1
 00.00000206  2c54                      MOVEA.L (A4),A6
 00.00000208  4eae fe86                 JSR     fe86(A6)
 00.0000020c  202d 000c                 MOVE.L  000c(A5),D0
 00.00000210  4e5d                      UNLK    A5
 00.00000212  285f                      MOVEA.L (A7)+,A4
 00.00000214  2f57 0004                 MOVE.L  (A7),0004(A7)
 00.00000218  588f                      ADDQ.L  #4,A7
 00.0000021a  4e75                      RTS     

*00.0000021c Kernel.Halt:
 00.0000021c  2e79 02.0000000c          MOVEA.L 02.0000000c,A7
 00.00000222  2f00                      MOVE.L  D0,-(A7)
 00.00000224  6100 ff02                 BSR.W   Kernel.DoCleanup
 00.00000228  4e75                      RTS     
 00.0000022a  4e71                      NOP     

*00.0000022c Kernel.NewSysBlk:
 00.0000022c  2f0c                      MOVE.L  A4,-(A7)
 00.0000022e  49f9 Kernel%VAR           LEA.L   Kernel%VAR,A4
 00.00000234  4e55 fffc                 LINK.W  A5,#fffc
 00.00000238  42ad fffc                 CLR.L   fffc(A5)
 00.0000023c  4aad 000e                 TST.L   000e(A5)
 00.00000240  6f00 006c                 BLE.W   000002ae
 00.00000244  2e2d 000e                 MOVE.L  000e(A5),D7
 00.00000248  5687                      ADDQ.L  #3,D7
 00.0000024a  0287 ffff fffc            ANDI.L  #fffffffc,D7
 00.00000250  2b47 000e                 MOVE.L  D7,000e(A5)
 00.00000254  2e2d 000e                 MOVE.L  000e(A5),D7
 00.00000258  5087                      ADDQ.L  #8,D7
 00.0000025a  2007                      MOVE.L  D7,D0
 00.0000025c  223c 0001 0000            MOVE.L  #00010000,D1
 00.00000262  2c54                      MOVEA.L (A4),A6
 00.00000264  4eae ff3a                 JSR     ff3a(A6)
 00.00000268  2b40 fffc                 MOVE.L  D0,fffc(A5)
 00.0000026c  4aad fffc                 TST.L   fffc(A5)
 00.00000270  6700 003c                 BEQ.W   000002ae
 00.00000274  4a2d 000c                 TST.B   000c(A5)
 00.00000278  6700 0014                 BEQ.W   0000028e
 00.0000027c  266d fffc                 MOVEA.L fffc(A5),A3
 00.00000280  26ac 0026                 MOVE.L  0026(A4),(A3)
 00.00000284  296d fffc 0026            MOVE.L  fffc(A5),0026(A4)
 00.0000028a  6000 0010                 BRA.W   0000029c
 00.0000028e  266d fffc                 MOVEA.L fffc(A5),A3
 00.00000292  26ac 002a                 MOVE.L  002a(A4),(A3)
 00.00000296  296d fffc 002a            MOVE.L  fffc(A5),002a(A4)
 00.0000029c  2e2d 000e                 MOVE.L  000e(A5),D7
 00.000002a0  5287                      ADDQ.L  #1,D7
 00.000002a2  266d fffc                 MOVEA.L fffc(A5),A3
 00.000002a6  2747 0004                 MOVE.L  D7,0004(A3)
 00.000002aa  50ad fffc                 ADDQ.L  #8,fffc(A5)
 00.000002ae  202d fffc                 MOVE.L  fffc(A5),D0
 00.000002b2  6000 0002                 BRA.W   000002b6
 00.000002b6  4e5d                      UNLK    A5
 00.000002b8  285f                      MOVEA.L (A7)+,A4
 00.000002ba  2f57 0006                 MOVE.L  (A7),0006(A7)
 00.000002be  5c8f                      ADDQ.L  #6,A7
 00.000002c0  4e75                      RTS     
 00.000002c2  4e71                      NOP     

*00.000002c4 Kernel.TrapHandler:
 00.000002c4  0c97 0000 0009            CMPI.L  #00000009,(A7)
 00.000002ca  6734                      BEQ.B   00000300
 00.000002cc  0c97 0000 0002            CMPI.L  #00000002,(A7)
 00.000002d2  652c                      BCS.B   00000300
 00.000002d4  0c97 0000 000b            CMPI.L  #0000000b,(A7)
 00.000002da  6310                      BLS.B   000002ec
 00.000002dc  0c97 0000 0020            CMPI.L  #00000020,(A7)
 00.000002e2  651c                      BCS.B   00000300
 00.000002e4  0c97 0000 0025            CMPI.L  #00000025,(A7)
 00.000002ea  6214                      BHI.B   00000300
 00.000002ec  201f                      MOVE.L  (A7)+,D0
 00.000002ee  0680 0000 0064            ADDI.L  #00000064,D0
 00.000002f4  41f9 Kernel.Halt          LEA.L   Kernel.Halt,A0
 00.000002fa  2f48 0002                 MOVE.L  A0,0002(A7)
 00.000002fe  4e73                      RTE     
 00.00000300  2039 02.00000010          MOVE.L  02.00000010,D0
 00.00000306  6704                      BEQ.B   0000030c
 00.00000308  2f00                      MOVE.L  D0,-(A7)
 00.0000030a  4e75                      RTS     
 00.0000030c  588f                      ADDQ.L  #4,A7
 00.0000030e  4e73                      RTE     

*00.00000310 Kernel.SetCleanup:
 00.00000310  2f0c                      MOVE.L  A4,-(A7)
 00.00000312  49f9 Kernel%VAR           LEA.L   Kernel%VAR,A4
 00.00000318  4e55 fffc                 LINK.W  A5,#fffc
 00.0000031c  2f3c 0000 0008            MOVE.L  #00000008,-(A7)
 00.00000322  51e7                      SF.B    -(A7)
 00.00000324  6100 ff06                 BSR.W   Kernel.NewSysBlk
 00.00000328  2b40 fffc                 MOVE.L  D0,fffc(A5)
 00.0000032c  4aad fffc                 TST.L   fffc(A5)
 00.00000330  6600 0008                 BNE.W   0000033a
 00.00000334  7019                      MOVEQ.L #19,D0
 00.00000336  6100 fee4                 BSR.W   Kernel.Halt
 00.0000033a  266d fffc                 MOVEA.L fffc(A5),A3
 00.0000033e  26ac 0032                 MOVE.L  0032(A4),(A3)
 00.00000342  296d fffc 0032            MOVE.L  fffc(A5),0032(A4)
 00.00000348  266d fffc                 MOVEA.L fffc(A5),A3
 00.0000034c  276d 000c 0004            MOVE.L  000c(A5),0004(A3)
 00.00000352  4e5d                      UNLK    A5
 00.00000354  285f                      MOVEA.L (A7)+,A4
 00.00000356  2f57 0004                 MOVE.L  (A7),0004(A7)
 00.0000035a  588f                      ADDQ.L  #4,A7
 00.0000035c  4e75                      RTS     
 00.0000035e  4e71                      NOP     

*00.00000360 Kernel.AllocUserTraps:
 00.00000360  4e55 fffc                 LINK.W  A5,#fffc
 00.00000364  42ac 0036                 CLR.L   0036(A4)
 00.00000368  42ad fffc                 CLR.L   fffc(A5)
 00.0000036c  0cad 0000 0005 fffc       CMPI.L  #00000005,fffc(A5)
 00.00000374  6e00 0024                 BGT.W   0000039a
 00.00000378  202d fffc                 MOVE.L  fffc(A5),D0
 00.0000037c  2c54                      MOVEA.L (A4),A6
 00.0000037e  4eae feaa                 JSR     feaa(A6)
 00.00000382  4a00                      TST.B   D0
 00.00000384  6d00 000e                 BLT.W   00000394
 00.00000388  7e01                      MOVEQ.L #01,D7
 00.0000038a  2c2d fffc                 MOVE.L  fffc(A5),D6
 00.0000038e  edaf                      LSL.L   D6,D7
 00.00000390  8fac 0036                 OR.L    D7,0036(A4)
 00.00000394  52ad fffc                 ADDQ.L  #1,fffc(A5)
 00.00000398  60d2                      BRA.B   0000036c
 00.0000039a  4e5d                      UNLK    A5
 00.0000039c  4e75                      RTS     
 00.0000039e  4e71                      NOP     

*00.000003a0 Kernel%INIT2A7CA0FB:
 00.000003a0  49f9 Kernel%VAR           LEA.L   Kernel%VAR,A4
 00.000003a6  4a2c 003a                 TST.B   003a(A4)
 00.000003aa  6600 00c2                 BNE.W   0000046e
 00.000003ae  50ec 003a                 ST.B    003a(A4)
 00.000003b2  2948 0016                 MOVE.L  A0,0016(A4)
 00.000003b6  2940 001a                 MOVE.L  D0,001a(A4)
 00.000003ba  294f 0008                 MOVE.L  A7,0008(A4)
 00.000003be  58ac 0008                 ADDQ.L  #4,0008(A4)
 00.000003c2  28b8 0004                 MOVE.L  0004.W,(A4)
 00.000003c6  93c9                      SUBA.L  A1,A1
 00.000003c8  2c54                      MOVEA.L (A4),A6
 00.000003ca  4eae feda                 JSR     feda(A6)
 00.000003ce  2940 002e                 MOVE.L  D0,002e(A4)
 00.000003d2  266c 002e                 MOVEA.L 002e(A4),A3
 00.000003d6  4aab 00ac                 TST.L   00ac(A3)
 00.000003da  57ec 0014                 SEQ.B   0014(A4)
 00.000003de  4a2c 0014                 TST.B   0014(A4)
 00.000003e2  6700 0026                 BEQ.W   0000040a
 00.000003e6  206c 002e                 MOVEA.L 002e(A4),A0
 00.000003ea  41e8 005c                 LEA.L   005c(A0),A0
 00.000003ee  2c54                      MOVEA.L (A4),A6
 00.000003f0  4eae fe80                 JSR     fe80(A6)
 00.000003f4  206c 002e                 MOVEA.L 002e(A4),A0
 00.000003f8  41e8 005c                 LEA.L   005c(A0),A0
 00.000003fc  2c54                      MOVEA.L (A4),A6
 00.000003fe  4eae fe8c                 JSR     fe8c(A6)
 00.00000402  2940 001e                 MOVE.L  D0,001e(A4)
 00.00000406  6000 0006                 BRA.W   0000040e
 00.0000040a  42ac 001e                 CLR.L   001e(A4)
 00.0000040e  6100 ff50                 BSR.W   Kernel.AllocUserTraps
 00.00000412  266c 002e                 MOVEA.L 002e(A4),A3
 00.00000416  296b 0032 000c            MOVE.L  0032(A3),000c(A4)
 00.0000041c  266c 002e                 MOVEA.L 002e(A4),A3
 00.00000420  296b 002e 0010            MOVE.L  002e(A3),0010(A4)
 00.00000426  266c 002e                 MOVEA.L 002e(A4),A3
 00.0000042a  277c Kernel.TrapHandler 0032 MOVE.L #Kernel.TrapHandler,0032(A3)
 00.00000432  266c 002e                 MOVEA.L 002e(A4),A3
 00.00000436  42ab 002e                 CLR.L   002e(A3)
 00.0000043a  42ac 0022                 CLR.L   0022(A4)
 00.0000043e  42ac 0004                 CLR.L   0004(A4)
 00.00000442  42ac 0026                 CLR.L   0026(A4)
 00.00000446  42ac 002a                 CLR.L   002a(A4)
 00.0000044a  42ac 0032                 CLR.L   0032(A4)
 00.0000044e  43f9 Kernel%CONST         LEA.L   Kernel%CONST,A1
 00.00000454  7021                      MOVEQ.L #21,D0
 00.00000456  2c54                      MOVEA.L (A4),A6
 00.00000458  4eae fdd8                 JSR     fdd8(A6)
 00.0000045c  2940 0004                 MOVE.L  D0,0004(A4)
 00.00000460  4aac 0004                 TST.L   0004(A4)
 00.00000464  6600 0008                 BNE.W   0000046e
 00.00000468  7064                      MOVEQ.L #64,D0
 00.0000046a  6100 fdb0                 BSR.W   Kernel.Halt
 00.0000046e  4e75                      RTS     

HUNK_DATA #01 (               ) Hunk=000003ea Size=24 bytes
 01.00000000 Kernel%CONST
 01.00000000  6d61 7468 6965 6565 7369 6e67 6261 732e
 01.00000010  6c69 6272 6172 7900

HUNK_BSS  #02 (               ) Hunk=000003eb Size=64 bytes
 02.00000000 TestAProf%VAR
 02.00000004 Kernel%VAR
