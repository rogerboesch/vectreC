MODULE Strupr;
VAR dst, src: ARRAY [0..63] OF CHAR; c, i: INTEGER;
BEGIN
  (* "Hello Vectrex World 123" + NUL *)
  src[0] := 72;  src[1] := 101; src[2] := 108; src[3] := 108; src[4] := 111;
  src[5] := 32;  src[6] := 86;  src[7] := 101; src[8] := 99;  src[9] := 116;
  src[10] := 114; src[11] := 101; src[12] := 120; src[13] := 32; src[14] := 87;
  src[15] := 111; src[16] := 114; src[17] := 108; src[18] := 100; src[19] := 32;
  src[20] := 49; src[21] := 50; src[22] := 51; src[23] := 0;
  (*<KERNEL>*)
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
  (*</KERNEL>*)
END Strupr.
