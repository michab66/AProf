<*STANDARD-*>

MODULE TestAProf;

<*$StackChk-*>

IMPORT SYS := SYSTEM, Kernel;

PROCEDURE P1 ();
  VAR i : INTEGER;
BEGIN (* P1 *)
  FOR i := 1 TO 10000 DO i := i END
END P1;

PROCEDURE P2 ();
  VAR i : INTEGER;
BEGIN (* P2 *)
  FOR i := 1 TO 20000 DO i := i END
END P2;

(*
** This is a dummy needed for the call to Kernel.SetCleanup(). It is
** Kernel.SetCleanup() that makes the call to Kernel.NewSysBlk() that
** illustrates the problem.
*)

<*$ < StackChk- *> (* We should not stack check here *)
PROCEDURE* Cleanup ( VAR rc : LONGINT );
BEGIN (* Cleanup *)
END Cleanup;
<*$ > *>

BEGIN
  Kernel.SetCleanup (Cleanup);
  P1;
  P2;
END TestAProf.
