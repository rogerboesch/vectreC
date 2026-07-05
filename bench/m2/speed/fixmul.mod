MODULE Fixmul;
CONST N = 16;
VAR vin, vout: ARRAY [0..15] OF INTEGER; factor, i: INTEGER;
BEGIN
  factor := 384;
  FOR i := 0 TO N-1 DO vin[i] := i*300 - 2000 END;
  (*<KERNEL>*)
  FOR i := 0 TO N-1 DO vout[i] := FIXMUL(vin[i], factor) END
  (*</KERNEL>*)
END Fixmul.
