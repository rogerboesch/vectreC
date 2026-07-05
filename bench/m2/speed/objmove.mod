MODULE Objmove;
CONST NOBJ = 16;
TYPE Obj = RECORD x, y, dx, dy: INTEGER END;
VAR objs: ARRAY [0..15] OF Obj; i: INTEGER;
BEGIN
  FOR i := 0 TO NOBJ-1 DO
    objs[i].x := i*200 - 1500; objs[i].y := i*111;
    objs[i].dx := i - 8; objs[i].dy := 7 - i
  END;
  (*<KERNEL>*)
  FOR i := 0 TO NOBJ-1 DO
    objs[i].x := objs[i].x + objs[i].dx;
    objs[i].y := objs[i].y + objs[i].dy;
    IF objs[i].x >  2032 THEN objs[i].x := objs[i].x - 4096 END;
    IF objs[i].x < -2048 THEN objs[i].x := objs[i].x + 4096 END;
    IF objs[i].y >  2032 THEN objs[i].y := objs[i].y - 4096 END;
    IF objs[i].y < -2048 THEN objs[i].y := objs[i].y + 4096 END
  END
  (*</KERNEL>*)
END Objmove.
