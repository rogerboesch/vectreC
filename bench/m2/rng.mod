(* rng.mod -- 16-bit xorshift PRNG filling a buffer. Port of bench/src/rng.c.
   Uses m2vec bit builtins (SHL/SHR/BITXOR); C's u8 noise[] is a CHAR array so
   the store keeps the low byte. rng_state/x are treated as 16-bit (shifts are
   logical, matching C's u16). *)
MODULE Rng;
CONST N = 32;
VAR
  noise: ARRAY [0..31] OF CHAR;
  rng_state, x, i: INTEGER;
BEGIN
  x := rng_state;
  FOR i := 0 TO N-1 DO
    x := BITXOR(x, SHL(x, 7));
    x := BITXOR(x, SHR(x, 9));
    x := BITXOR(x, SHL(x, 8));
    noise[i] := x
  END;
  rng_state := x
END Rng.
