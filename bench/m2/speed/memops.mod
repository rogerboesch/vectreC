MODULE Memops;
VAR dst, src: ARRAY [0..63] OF CHAR; v, n, i: INTEGER;
BEGIN
  n := 40; v := 170;
  FOR i := 0 TO 39 DO src[i] := i * 3 END;
  (*<KERNEL>*)
  i := 0; WHILE i < n DO dst[i] := src[i]; i := i + 1 END;
  i := 0; WHILE i < n DO dst[i] := v; i := i + 1 END
  (*</KERNEL>*)
END Memops.
