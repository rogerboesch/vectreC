(* objmove.mod -- sprite movement with screen wrap. Port of bench/src/objmove.c.
   Uses an ARRAY OF RECORD (objs[i].x etc). Velocities dx/dy are INTEGER here
   (C uses s8); the Q sub-pixel wrap constants are 127*16 / 256*16 / -128*16. *)
MODULE Objmove;
CONST NOBJ = 16;
TYPE
  Obj = RECORD x, y, dx, dy: INTEGER END;
VAR
  objs: ARRAY [0..15] OF Obj;
  i: INTEGER;
BEGIN
  FOR i := 0 TO NOBJ-1 DO
    objs[i].x := objs[i].x + objs[i].dx;
    objs[i].y := objs[i].y + objs[i].dy;
    IF objs[i].x >  2032 THEN objs[i].x := objs[i].x - 4096 END;
    IF objs[i].x < -2048 THEN objs[i].x := objs[i].x + 4096 END;
    IF objs[i].y >  2032 THEN objs[i].y := objs[i].y - 4096 END;
    IF objs[i].y < -2048 THEN objs[i].y := objs[i].y + 4096 END
  END
END Objmove.
