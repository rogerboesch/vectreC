MODULE Isort;
CONST N = 16;
VAR keys: ARRAY [0..15] OF INTEGER; i, j, k: INTEGER;
BEGIN
  FOR i := 0 TO N-1 DO keys[i] := N - i END;
  (*<KERNEL>*)
  i := 1;
  WHILE i < N DO
    k := keys[i]; j := i;
    LOOP
      IF j = 0 THEN EXIT END;
      IF keys[j-1] <= k THEN EXIT END;
      keys[j] := keys[j-1]; j := j - 1
    END;
    keys[j] := k; i := i + 1
  END
  (*</KERNEL>*)
END Isort.
