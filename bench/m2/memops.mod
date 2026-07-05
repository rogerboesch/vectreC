(* memops.mod -- hand-rolled memcpy + memset. Port of bench/src/memops.c.
   C's pointer post-increment loops become index loops over global CHAR arrays
   (m2vec has no pointers); same byte-move work. *)
MODULE Memops;
VAR
  dst, src: ARRAY [0..63] OF CHAR;
  v, n, i: INTEGER;
BEGIN
  i := 0;
  WHILE i < n DO dst[i] := src[i]; i := i + 1 END;
  i := 0;
  WHILE i < n DO dst[i] := v; i := i + 1 END
END Memops.
