(* fixmul.mod -- Q8.8 fixed-point vector scaling. Port of bench/src/fixmul.c.
   vout[i] = (vin[i] * factor) >> 8, via the m2vec FIXMUL builtin (a 16x16->32
   signed multiply then >>8, in the __fixmul16 runtime helper). Rounding is
   toward zero (C uses arithmetic >>, i.e. toward -inf; they differ by 1 LSB on
   negative results -- irrelevant to code size, which excludes helper bodies). *)
MODULE Fixmul;
CONST N = 16;
VAR
  vin, vout: ARRAY [0..15] OF INTEGER;
  factor, i: INTEGER;
BEGIN
  FOR i := 0 TO N-1 DO
    vout[i] := FIXMUL(vin[i], factor)
  END
END Fixmul.
