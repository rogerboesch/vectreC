MODULE Checksum;
VAR p: ARRAY [0..63] OF CHAR; n, sum, h, b, i, result: INTEGER;
BEGIN
  n := 64;
  FOR i := 0 TO 63 DO p[i] := i + 1 END;
  (*<KERNEL>*)
  sum := 0; h := 0; i := 0;
  WHILE i < n DO
    b := p[i]; sum := sum + b;
    h := BITAND(BITOR(SHL(h, 1), SHR(h, 7)), 255);
    h := BITXOR(h, b); i := i + 1
  END;
  result := BITXOR(sum, SHL(h, 8))
  (*</KERNEL>*)
END Checksum.
