MODULE Collide;
CONST NS = 12;
VAR sx, sy, sw, sh: ARRAY [0..11] OF INTEGER; n, i, j, dx, dy: INTEGER;
BEGIN
  FOR i := 0 TO NS-1 DO sx[i] := i*7 - 40; sy[i] := i*5 - 30; sw[i] := 4; sh[i] := 4 END;
  (*<KERNEL>*)
  n := 0;
  FOR i := 0 TO NS-1 DO
    FOR j := i+1 TO NS-1 DO
      dx := sx[i] - sx[j]; IF dx < 0 THEN dx := -dx END;
      dy := sy[i] - sy[j]; IF dy < 0 THEN dy := -dy END;
      IF dx < sw[i] + sw[j] THEN IF dy < sh[i] + sh[j] THEN n := n + 1 END END
    END
  END
  (*</KERNEL>*)
END Collide.
