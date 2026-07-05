MODULE Rng;
CONST N = 32;
VAR noise: ARRAY [0..31] OF CHAR; rng_state, x, i: INTEGER;
BEGIN
  rng_state := 0ACE1H;
  (*<KERNEL>*)
  x := rng_state;
  FOR i := 0 TO N-1 DO
    x := BITXOR(x, SHL(x, 7));
    x := BITXOR(x, SHR(x, 9));
    x := BITXOR(x, SHL(x, 8));
    noise[i] := x
  END;
  rng_state := x
  (*</KERNEL>*)
END Rng.
