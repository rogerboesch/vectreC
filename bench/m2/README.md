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
- **No signed 8-bit type** → C's `s8` arrays become `INTEGER` (16-bit) where the
  values are used arithmetically, or `CHAR` (unsigned byte) where only the low
  byte is stored.

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
| memops   |   58 |   56 |  **41** |        116 |       108 |
| strupr   |   54 |   49 |  **46** |        140 |       114 |
| checksum |   90 |  101 |  **59** |        157 |       150 |
| isort    |  117 |  111 |  **59** |        186 |       156 |
| statem   |  149 |  132 |     132 |        225 |       198 |
| bcdscore |  159 |  105 |     108 |        273 |       235 |
| clamp    |  122 |  103 |     139 |        193 |       152 |
| **TOTAL**| 1333 | 1188 | **948** |       2327 |      1684 |

Normalised to gcc6809 = 1.00: **cmoc 1.41 · vbcc 1.25 · gcc6809 1.00 ·
m2vec 1.78** (was 2.46 before optimisation). m2vec wins `fixmul` (54 B). Run it:
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
| collide  | 20873 | 12110 |**7644** |      29920 |       24034 |
| fixmul   | 15096 | 44611 |     n/a |       5956 |    **5423** |
| rng      |  7360 |  6068 |**3583** |       8321 |        6694 |
| memops   |  4152 |  2434 |**1726** |       9648 |        8448 |
| strupr   |  1810 |**1197**|   1444 |       5016 |        3402 |
| checksum |  9316 |  9090 |**3657** |      17197 |       15853 |
| isort    | 17329 |  8703 |**6553** |      30176 |       22286 |
| statem   |   178 |  **88**|    115 |        133 |         103 |
| bcdscore |   650 |   354 | **342** |        977 |         791 |
| clamp    |  2695 |  1665 |**1473** |       2792 |    **1939** |

Run it: `bench/m2/measure_speed_m2.sh`. (m2vec beats cmoc on `statem` and
`clamp`, and wins `fixmul` outright.)

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

Where m2vec is now competitive or wins:

- **`fixmul` — smallest (54 B) and fastest (5423).** The whole 16×16→32
  multiply-and-shift is one `__fixmul16` call (helper body excluded), so the
  kernel is just a tight index loop plus the call; cmoc/vbcc inline slower
  generic 32-bit multiplies (vbcc's is 44611 cycles).
- **`statem` (103) and `clamp` (1939) beat cmoc.** Small/flat kernels suit
  m2vec's prologue-free module bodies.
- `objmove` went from a ~8× outlier to ~1.2× cmoc after (3), (4) and (5).

## Remaining gap

m2vec is ~1.78× gcc6809 on size overall. What still separates it:

- **No register allocation in WHILE/LOOP** — the FOR promotion above does not
  cover them, so the WHILE-based kernels (`isort`, `memops`, `strupr`,
  `checksum`, `bcdscore`) still reload their loop variables from RAM each use.
  General (liveness-based) register allocation is the next lever; `isort` is the
  biggest remaining kernel.
- **No branch relaxation** — control flow uses long `LBRA`/`LBcc` (lwasm's
  auto-sizing pragma is unusable: it forces *every* conditional branch long).
- Plus 16-bit `INTEGER` where C uses 8-bit types, and no strength reduction.

## Caveat

These are **module bodies**, not per-function measurements — there are no call
frames or prologues in the m2vec numbers (nor in these particular C kernels,
whose functions are leaf loops). Once m2vec grows procedures, the kernels can be
rewritten as real callable functions and measured per-function, identically to
`../measure.sh`.
