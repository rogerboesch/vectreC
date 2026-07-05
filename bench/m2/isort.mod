(* isort.mod -- insertion sort of a small array.
   m2vec (Modula-2) port of bench/src/isort.c.

   Notes on the port:
   - The kernel operates on a module-global array (m2vec has no PROCEDUREs yet,
     so each kernel is a module body rather than a callable function).
   - Modula-2 has no short-circuit "&&"; the C inner test
       while (j > 0 && keys[j-1] > k)
     becomes a LOOP with two EXIT guards.
   - Elements are INTEGER (16-bit) rather than C's s8 (8-bit); m2vec has no
     signed byte type. This inflates array-index scaling vs the C version. *)

MODULE Isort;

CONST N = 16;

VAR
  keys: ARRAY [0..15] OF INTEGER;
  i, j, k: INTEGER;

BEGIN
  i := 1;
  WHILE i < N DO
    k := keys[i];
    j := i;
    LOOP
      IF j = 0 THEN EXIT END;
      IF keys[j-1] <= k THEN EXIT END;
      keys[j] := keys[j-1];
      j := j - 1
    END;
    keys[j] := k;
    i := i + 1
  END
END Isort.
