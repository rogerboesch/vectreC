(* checksum.mod -- ROM checksum (sum + rolling xor hash). Port of bench/src/checksum.c.
   h is kept 8-bit via BITAND(...,255); the rotate-left-1 is BITOR(SHL(h,1),SHR(h,7)). *)
MODULE Checksum;
VAR
  p: ARRAY [0..63] OF CHAR;
  n, sum, i, result: INTEGER;
  h, b: BYTE;
BEGIN
  sum := 0; h := 0; i := 0;
  WHILE i < n DO
    b := p[i];
    sum := sum + b;
    h := BITAND(BITOR(SHL(h, 1), SHR(h, 7)), 255);
    h := BITXOR(h, b);
    i := i + 1
  END;
  result := BITXOR(sum, SHL(h, 8))
END Checksum.
