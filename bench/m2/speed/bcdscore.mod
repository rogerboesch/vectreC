MODULE Bcdscore;
VAR score: ARRAY [0..2] OF CHAR; add, carry, lo, hi, i: INTEGER;
BEGIN
  score[0] := 12H; score[1] := 34H; score[2] := 99H; add := 77H;
  (*<KERNEL>*)
  carry := add; i := 2;
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
  (*</KERNEL>*)
END Bcdscore.
