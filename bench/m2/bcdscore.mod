(* bcdscore.mod -- add a value to a packed BCD score. Port of bench/src/bcdscore.c.
   The C `for (i=2; i>=0; i--)` with `if(!carry) break` becomes a LOOP with EXITs
   (m2vec's EXIT leaves a LOOP; FOR has no break). Nibble ops via SHL/SHR/BITAND. *)
MODULE Bcdscore;
VAR
  score: ARRAY [0..2] OF CHAR;
  add, carry, lo, hi, i: INTEGER;
BEGIN
  carry := add;
  i := 2;
  LOOP
    IF i < 0 THEN EXIT END;
    lo := BITAND(score[i], 15) + BITAND(carry, 15);
    hi := SHR(score[i], 4) + SHR(carry, 4);
    carry := 0;
    IF lo > 9 THEN lo := lo - 10; hi := hi + 1 END;
    IF hi > 9 THEN hi := hi - 10; carry := 1 END;
    score[i] := BITOR(SHL(hi, 4), lo);
    IF carry = 0 THEN EXIT END;
    i := i - 1
  END
END Bcdscore.
