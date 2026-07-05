(* collide.mod -- O(n^2) AABB overlap counting. Port of bench/src/collide.c.
   Parallel arrays (sx,sy,sw,sh) stand in for the C arrays; the short-circuit
   `dx<... && dy<...` becomes nested IFs; abs via `IF <0 THEN neg`. *)
MODULE Collide;
CONST NS = 12;
VAR
  sx, sy, sw, sh: ARRAY [0..11] OF INTEGER;
  n, i, j, dx, dy: INTEGER;
BEGIN
  n := 0;
  FOR i := 0 TO NS-1 DO
    FOR j := i+1 TO NS-1 DO
      dx := sx[i] - sx[j];
      IF dx < 0 THEN dx := -dx END;
      dy := sy[i] - sy[j];
      IF dy < 0 THEN dy := -dy END;
      IF dx < sw[i] + sw[j] THEN
        IF dy < sh[i] + sh[j] THEN
          n := n + 1
        END
      END
    END
  END
END Collide.
