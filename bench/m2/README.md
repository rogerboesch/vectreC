# m2vec (Modula-2) in the 6809 code-size comparison

This adds a **fourth column** to the [C compiler comparison](../README.md):
**m2vec**, a from-scratch Modula-2 → 6809 cross-compiler written in Rust
(see `~/projects/vecatac/m2vec`). Where the parent benchmark pits three mature
C compilers (cmoc, vbcc, gcc6809) against each other, this asks a different
question: **how does a young, hand-written Modula-2 compiler compare on the same
game kernels?**

All 11 kernels are ported.

## How the ports are written

m2vec is an early compiler. It does **not** have procedures, pointers, or 32-bit
integers yet, so each kernel is a **module body operating on module-global
data** (not a callable function). The algorithm is the same as the C kernel; the
shape is adapted to what m2vec can express. Per-kernel notes live in the `.mod`
files. Notable adaptations:

- **No pointers** → pointer-walking kernels (`memops`, `strupr`, `checksum`,
  `clamp`) become index loops over global arrays.
- **No short-circuit `&&`** → range/compound tests become nested `IF`s or a
  `LOOP` with `EXIT`s (`isort`, `strupr`, `collide`, `bcdscore`).
- **8-bit values** → C's `u8`/`s8` map to `BYTE`/`SHORTINT`, which m2vec now
  computes byte-wide (see optimisation 10); a few kernels still use `INTEGER`
  where the value genuinely needs 16 bits.

Three small m2vec extensions were built to cover the kernels (all validated by
unit tests in the compiler):

- **Integer bit builtins** `SHL/SHR/BITAND/BITOR/BITXOR/BITNOT` (Modula-2 has no
  integer bitwise operators) — for `rng`, `checksum`, `bcdscore`.
- **Array-of-record field access** `objs[i].x` — for `objmove`.
- **`FIXMUL(a,b)`** = `(a*b) >> 8`, a Q8.8 multiply backed by a signed 16×16→32
  runtime helper — for `fixmul`.

## Size metric

Same intent as `../measure.sh` — bytes of the kernel's own code, **excluding
runtime-helper bodies** but including the call sites. m2vec lays out each ROM as

```
[cartridge header] [kernel body] _m2vec_halt [runtime helpers] [ROM data]
```

so the kernel is exactly `addr(_m2vec_halt) - header_size`; the helpers
(`__mul16`, `__fixmul16`, …) sit after `_m2vec_halt` and are excluded, just as
the C measurement excludes the libgcc/cmoc helper bodies. `measure_m2.sh` reads
`addr(_m2vec_halt)` from an lwasm listing.

## Results — code size in bytes (lower = better)

The **m2vec** column is after the codegen optimisations below; **base** is the
first working version, to show the optimisation headroom.

| kernel   | cmoc | vbcc | gcc6809 | m2vec base | **m2vec** |
|----------|-----:|-----:|--------:|-----------:|----------:|
| objmove  |  148 |  119 |  **80** |        533 |       170 |
| collide  |  206 |  203 | **131** |        298 |       237 |
| fixmul   |  112 |  116 |      89 |         71 |    **54** |
| rng      |  118 |   93 |  **64** |        135 |       110 |
| memops   |   58 |   56 |  **41** |        116 |        89 |
| strupr   |   54 |   49 |  **46** |        140 |       105 |
| checksum |   90 |  101 |  **59** |        157 |       122 |
| isort    |  117 |  111 |  **59** |        186 |       124 |
| statem   |  149 |  132 |     132 |        225 |       198 |
| bcdscore |  159 |  105 |     108 |        273 |       216 |
| clamp    |  122 |  103 |     139 |        193 |       152 |
| **TOTAL**| 1333 | 1188 | **948** |       2327 |      1529 |

Normalised to gcc6809 = 1.00: **cmoc 1.41 · vbcc 1.25 · gcc6809 1.00 ·
m2vec 1.61** (was 2.46 before optimisation). m2vec wins `fixmul` (54 B). Run it:
`bench/m2/measure_m2.sh`.

## Results — speed (dynamic cycle count, lower = better)

Measured in the same cycle-accurate exec09 core as `../measure_speed.sh`. m2vec's
`--bench` mode emits a `_run` routine at $1000 ending in `RTS`; the runner times
it entry-to-return. Each kernel is built twice — **full** (data init + kernel)
and **base** (init only) — and `cycles = full - base`, so the init cost cancels.
The timed region is marked in `speed/*.mod` with `(*<KERNEL>*) … (*</KERNEL>*)`;
the base build strips it.

| kernel   |  cmoc |  vbcc | gcc6809 | m2vec base |   **m2vec** |
|----------|------:|------:|--------:|-----------:|------------:|
| objmove  |  3648 |  4300 |**1509** |      30057 |        4372 |
| collide  | 20873 | 12110 |**7644** |      29920 |        23242 |
| fixmul   | 15096 | 44611 |     n/a |       5956 |    **5423** |
| rng      |  7360 |  6068 |**3583** |       8321 |        6694 |
| memops   |  4152 |  2434 |**1726** |       9648 |    **2549** |
| strupr   |  1810 |**1197**|   1444 |       5016 |    **1671** |
| checksum |  9316 |  9090 |**3657** |      17197 |    **8191** |
| isort    | 17329 |  8703 |**6553** |      30176 |       13214 |
| statem   |   178 |  **88**|    115 |        133 |        97 |
| bcdscore |   650 |   354 | **342** |        977 |        677 |
| clamp    |  2695 |  1665 |**1473** |       2792 |    **1747** |

Run it: `bench/m2/measure_speed_m2.sh`. (m2vec now beats cmoc on **8 of 11**
kernels — `fixmul`, `isort`, `memops`, `strupr`, `checksum`, `rng`, `statem`,
`clamp` — after the optimisations below.)

## Codegen optimisations applied

Starting from the naive first version ("base" columns), these passes were added
to close the gap — the benchmark drove each one:

1. **Immediate arithmetic** — `x±c`, `x<c`, … use `ADDD/SUBD/CMPD #c` instead of
   a push/pull sequence. The single biggest peephole for loop and index math.
2. **Fixpoint peephole pass** — removes dead code after unconditional transfers,
   redundant `STD`/`LDD` reloads and `PSHS D`/`PULS D` pairs, branch-to-next-
   label, and unreferenced labels.
3. **Power-of-two index scaling** — `i*8` for an 8-byte record becomes three
   shifts instead of a `__mul16` call. This alone took `objmove` from 30057 to
   8457 cycles.
4. **CSE on element addresses** — `objs[i]`'s base is computed once and reused
   across its fields (via indexed `off,X`) instead of recomputed per access.
   `objmove` again: 8457 → 4905 cycles, 429 → 189 bytes.
5. **FOR-counter register promotion** — a FOR counter whose body makes no BIOS
   call lives in `Y` (or `U` when nested) instead of RAM: init `TFR`, exit test
   `CMPY`, increment `LEAY`, replacing per-iteration `LDD/ADDD/STD`. Helps every
   FOR kernel (`fixmul` 71→54 B, `clamp` now beats cmoc, etc.).
6. **WHILE/LOOP counter promotion** — the same idea for `WHILE`/`LOOP`: the most
   beneficial scalar 16-bit variable is held in `Y`/`U` across the loop (loaded
   before, spilled after), and `var := var±const` becomes a single `LEA`. Since
   a register read via `TFR` costs one cycle more than `LDD` on the 6809, the
   candidate is chosen by a cost model (`benefit ≈ 11·self-increments −
   references`) so only the loop counter — not a read-only index — is promoted.
   `isort` 22286→21089, `memops` 8448→7604, `strupr` 3402→3167,
   `checksum` 15853→15167 cycles; total size 1684→1661 B.
7. **Array-index strength reduction** — in a WHILE/LOOP with an induction
   variable `i` (`i := i ± c`), each scalar array accessed only as `arr[i±k]` is
   walked by a pointer register (`Y`/`U`) holding `&arr[i]`, so the access is a
   bare `off,reg` deref instead of recomputing `base + i*elem`, and the pointer
   is bumped by `c*elem` (`LEA`) at the induction. Up to two arrays per loop; `i`
   stays in RAM so a trailing `arr[i] := 0` still works with no live-range
   analysis. `isort` 21089→13214 (−37%), `memops` 7604→6065, `strupr`
   3167→2598, `bcdscore` 793→689 cycles; total size 1661→1633 B.
8. **Access + increment folding** — when a strength-reduced array is accessed
   exactly once, unconditionally, as `arr[i]` with step +1, the load/store and
   the pointer bump combine into one post-increment: `LDB ,U+` / `STB ,Y+`
   (bytes), `LDD ,U++` / `STD ,Y++` (words), dropping the separate `LEA`. Arrays
   accessed more than once (`isort` `keys[j]`/`keys[j-1]`) keep the `off,reg`
   deref. `memops` 6065→5585, `strupr` 2598→2415, `checksum` 15166→14910
   cycles; total size 1633→1621 B.
9. **Dead-counter elimination** — when a `WHILE i < n` (step +1) has `i` dead
   after the loop and used only to index strength-reduced arrays, the counter is
   dropped: a limit `&arr[n]` is computed once, the condition becomes an unsigned
   pointer compare (`CMPY __srlim` / `LBHS`), and `i := i+1` just advances the
   pointers. Liveness uses the module-body continuation; `strupr` keeps `i` (its
   trailing `dst[i] := 0` reads it). `memops` 5585→2629 (−53%, now beats cmoc),
   `checksum` 14910→12544; total size 1621→1606 B.
10. **8-bit `BYTE`/`SHORTINT` arithmetic** — a byte-only expression (a byte
    variable/element, a fitting constant, or an arithmetic/bit/shift chain over
    such) evaluates in `B` (`LDB`, `ADDB/SUBB #c`, `ANDB/ORB/EORB`,
    `LSLB/LSRB/ASRB`, plus two-operand ops via one `PSHS B`/`,S+`); byte
    comparisons use `CMPB` with an unsigned/signed branch. Mixed-width contexts
    fall back to 16-bit with the usual `CLRA`/`SEX`. `checksum` 12544→8831 (now
    beats cmoc), `strupr` 2415→2161, `memops` 2629→2549; total size
    1606→1577 B.
11. **Local value cache** — a basic-block cache tracks which scalar variable `D`
    and `B` hold and drops a `LDD`/`LDB` that reloads a value already in the
    register. Comparisons (`CMPx`) and index ops don't clobber `A/B/D`, so a byte
    value survives the `IF`s that read it; labels and calls reset the cache, and
    store aliasing is handled. This is the 6809-appropriate register allocation:
    `D` is the only arithmetic register and can't be reserved, so the win is
    cutting redundant memory traffic. `strupr` 2161→1901, `checksum` 8831→8191,
    `collide` 24034→23242, `statem` 103→97; total size 1577→1547 B.
12. **Cross-block value cache** — a forward MUST dataflow over the assembly
    (CFG of fall-through + branch edges, meet = intersection at merges, iterated
    to a fixpoint) replaces the basic-block cache: it keeps a register value
    across a label whose predecessors all agree. Since a comparison doesn't
    clobber `A/B/D`, `strupr`'s `c` now stays in `B` across its whole range test
    instead of reloading each `IF`. `strupr` 1901→1671 (beats cmoc), `clamp`
    1939→1747; total size 1547→1529 B.

Where m2vec is now competitive or wins:

- **`fixmul` — smallest (54 B) and fastest (5423).** The whole 16×16→32
  multiply-and-shift is one `__fixmul16` call (helper body excluded), so the
  kernel is just a tight index loop plus the call; cmoc/vbcc inline slower
  generic 32-bit multiplies (vbcc's is 44611 cycles).
- **`memops` (2549), `strupr` (1671), `checksum` (8191), `isort` (13214),
  `statem` (97), `clamp` (1747) beat cmoc** — after passes (5)–(12) m2vec wins
  8 of 11 kernels on speed.
- `objmove` went from a ~8× outlier to ~1.2× cmoc after (3), (4) and (5).

## Remaining gap

m2vec is ~1.61× gcc6809 on size overall. What still separates it:

- **No branch relaxation** — control flow uses long `LBRA`/`LBcc` (lwasm's
  auto-sizing pragma is unusable: it forces *every* conditional branch long),
  the biggest remaining size cost.
- **`objmove`, `collide`, `bcdscore` still trail cmoc on speed** — record-array
  index math and multi-way branch chains where cmoc's mature instruction
  selection is still ahead.

## Caveat

These are **module bodies**, not per-function measurements — there are no call
frames or prologues in the m2vec numbers (nor in these particular C kernels,
whose functions are leaf loops). Once m2vec grows procedures, the kernels can be
rewritten as real callable functions and measured per-function, identically to
`../measure.sh`.
