MODULE Clamp;
CONST M = 8;
VAR xin, yin: ARRAY [0..15] OF INTEGER; xout, yout: ARRAY [0..15] OF CHAR; v, i: INTEGER;
BEGIN
  FOR i := 0 TO M-1 DO xin[i] := i*90 - 300; yin[i] := 400 - i*120 END;
  (*<KERNEL>*)
  FOR i := 0 TO M-1 DO
    v := xin[i];
    IF v >  127 THEN v :=  127 END;
    IF v < -128 THEN v := -128 END;
    xout[i] := v;
    v := yin[i];
    IF v >  127 THEN v :=  127 END;
    IF v < -128 THEN v := -128 END;
    yout[i] := v
  END
  (*</KERNEL>*)
END Clamp.
