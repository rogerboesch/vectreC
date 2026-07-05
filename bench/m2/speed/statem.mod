MODULE Statem;
VAR state, timer, score, ev: INTEGER;
BEGIN
  state := 1; timer := 5; ev := 2;
  (*<KERNEL>*)
  CASE state OF
    0: IF ev = 1 THEN state := 1; score := 0; timer := 60 END
  | 1: IF timer > 0 THEN timer := timer - 1 ELSE state := 2 END;
       score := score + ev
  | 2: IF ev = 2 THEN state := 3 END
  | 3: IF ev = 1 THEN state := 0 END
  | 4: IF ev = 3 THEN state := 1 END
  ELSE state := 0
  END
  (*</KERNEL>*)
END Statem.
