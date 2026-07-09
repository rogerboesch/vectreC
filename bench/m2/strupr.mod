(* strupr.mod -- copy a string while upper-casing it. Port of bench/src/strupr.c.
   'a'=97, 'z'=122; the range test 'a'<=c<='z' becomes nested IFs (no &&). *)
MODULE Strupr;
VAR
  dst, src: ARRAY [0..63] OF CHAR;
  c: BYTE;
  i: INTEGER;
BEGIN
  i := 0;
  LOOP
    c := src[i];
    IF c = 0 THEN EXIT END;
    IF c >= 97 THEN
      IF c <= 122 THEN c := c - 32 END
    END;
    dst[i] := c;
    i := i + 1
  END;
  dst[i] := 0
END Strupr.
