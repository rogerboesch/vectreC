(* statem.mod -- game state-machine dispatch via CASE.
   m2vec (Modula-2) port of bench/src/statem.c.

   Notes on the port:
   - The C function parameter `ev` becomes a module-global input (m2vec has no
     PROCEDUREs yet).
   - The C `switch` maps directly onto Modula-2 CASE; the `default` arm becomes
     ELSE.
   - `if (timer) timer--` becomes `IF timer > 0 THEN timer := timer - 1`. *)

MODULE Statem;

VAR
  state, timer, score, ev: INTEGER;

BEGIN
  CASE state OF
    0: IF ev = 1 THEN state := 1; score := 0; timer := 60 END
  | 1: IF timer > 0 THEN timer := timer - 1 ELSE state := 2 END;
       score := score + ev
  | 2: IF ev = 2 THEN state := 3 END
  | 3: IF ev = 1 THEN state := 0 END
  | 4: IF ev = 3 THEN state := 1 END
  ELSE
    state := 0
  END
END Statem.
