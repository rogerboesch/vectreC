(* clamp.mod -- clamp draw vectors to the signed 8-bit beam range.
   Port of bench/src/clamp.c (the clamp_vlist loop, with clamp8 inlined).
   xin/yin are s16 (INTEGER); xout/yout are s8 (CHAR, byte store). *)
MODULE Clamp;
CONST N = 16;
VAR
  xin, yin: ARRAY [0..15] OF INTEGER;
  xout, yout: ARRAY [0..15] OF CHAR;
  v, i: INTEGER;
BEGIN
  FOR i := 0 TO N-1 DO
    v := xin[i];
    IF v >  127 THEN v :=  127 END;
    IF v < -128 THEN v := -128 END;
    xout[i] := v;
    v := yin[i];
    IF v >  127 THEN v :=  127 END;
    IF v < -128 THEN v := -128 END;
    yout[i] := v
  END
END Clamp.
