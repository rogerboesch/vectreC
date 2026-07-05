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

| kernel   | cmoc | vbcc | gcc6809 | **m2vec** |
|----------|-----:|-----:|--------:|----------:|
| objmove  |  148 |  119 |  **80** |       533 |
| collide  |  206 |  203 | **131** |       298 |
| fixmul   |  112 |  116 |      89 |    **71** |
| rng      |  118 |   93 |  **64** |       135 |
| memops   |   58 |   56 |  **41** |       116 |
| strupr   |   54 |   49 |  **46** |       140 |
| checksum |   90 |  101 |  **59** |       157 |
| isort    |  117 |  111 |  **59** |       186 |
| statem   |  149 |  132 |     132 |       225 |
| bcdscore |  159 |  105 |     108 |       273 |
| clamp    |  122 |  103 |     139 |       193 |
| **TOTAL**| 1333 | 1188 | **948** |      2327 |

Normalised to gcc6809 = 1.00: **cmoc 1.41 · vbcc 1.25 · gcc6809 1.00 ·
m2vec 2.46**. Run it yourself: `bench/m2/measure_m2.sh`.

## Analysis

Overall m2vec is **~1.75× cmoc** (the largest C compiler here) and **~2.5×
gcc6809**. The reasons are structural, not bugs:

1. **No register allocator.** Expressions evaluate into the D accumulator with
   sub-results pushed/pulled on the stack; loop variables reload from RAM
   (`LDD _var`) every use instead of living in `X`/`Y`/`U`. This is the biggest
   single factor across every kernel.
2. **16-bit where C uses 8-bit.** `INTEGER` arrays scale the index by 2
   (`LSLB/ROLA`) and move 16-bit values where C does 8-bit loads.
3. **Long branches everywhere.** m2vec emits `LBRA`/`LBcc` (3–4 bytes) for all
   control flow to stay correct as programs grow; the C compilers size-optimise
   to 2-byte short branches. Branch-heavy kernels (`statem`, `collide`) pay this
   on every arm.
4. **No common-subexpression elimination.** This is what makes **`objmove` the
   outlier (533 B, ~4× cmoc)**: each `objs[i].field` recomputes the element
   address `i*8` from scratch — six `__mul16`-based address computations per
   iteration where C computes the object pointer once.
5. **No strength reduction / tail calls** — gcc6809's biggest wins.

**Where m2vec is competitive:**

- **`fixmul` — m2vec is smallest (71 B).** The whole 16×16→32 multiply-and-shift
  is one `JSR __fixmul16` (helper body excluded), so the kernel is just the tight
  index loop plus the call — the same reason gcc6809's `fixmul` call sites are
  small. m2vec has no per-function prologue to pay here.
- On the flatter kernels (`clamp`, `rng`) m2vec lands within ~1.6× of cmoc.

The benchmark is a useful **optimisation target** for m2vec: items 1–5 are
exactly the codegen passes that would close the gap, and CSE on designator
addresses alone would roughly halve `objmove`.

## Results — speed (dynamic cycle count, lower = better)

Measured in the same cycle-accurate exec09 core as `../measure_speed.sh`. m2vec's
`--bench` mode emits a `_run` routine at $1000 ending in `RTS`; the runner times
it entry-to-return. Each kernel is built twice — **full** (data init + kernel)
and **base** (init only) — and `cycles = full - base`, so the init cost cancels.
The timed region is marked in `speed/*.mod` with `(*<KERNEL>*) … (*</KERNEL>*)`;
the base build strips it.

| kernel   |  cmoc |  vbcc | gcc6809 |   **m2vec** |
|----------|------:|------:|--------:|------------:|
| objmove  |  3648 |  4300 |**1509** |       30057 |
| collide  | 20873 | 12110 |**7644** |       29920 |
| fixmul   | 15096 | 44611 |     n/a |    **5956** |
| rng      |  7360 |  6068 |**3583** |        8321 |
| memops   |  4152 |  2434 |**1726** |        9648 |
| strupr   |  1810 |**1197**|   1444 |        5016 |
| checksum |  9316 |  9090 |**3657** |       17197 |
| isort    | 17329 |  8703 |**6553** |       30176 |
| statem   |   178 |  **88**|    115 |         133 |
| bcdscore |   650 |   354 | **342** |         977 |
| clamp    |  2695 |  1665 |**1473** |        2792 |

Run it: `bench/m2/measure_speed_m2.sh`.

The speed picture mirrors the size one:

- **`fixmul` — m2vec is fastest (5956 cycles).** The 16×16→32 multiply-and-shift
  is one `__fixmul16` call built on the 6809 `MUL`; cmoc and vbcc route through
  generic 32-bit multiply helpers (vbcc's is famously slow here — 44611). Same
  reason m2vec wins on size.
- **`statem` (133) beats cmoc, `clamp` (2792) ties it.** On small, branchy or
  flat kernels m2vec's lack of a function prologue and simple dispatch keep it
  competitive.
- **`objmove` is the worst case (30057, ~8× cmoc).** Every `objs[i].field`
  recomputes `i*8` with a `__mul16` call at runtime — dozens of multiplies per
  frame where C computes the object pointer once. CSE on designator addresses
  is the highest-value optimisation m2vec could add.
- Elsewhere m2vec runs ~2–4× the C compilers, dominated by reloading loop
  variables from RAM every use (no register allocation).

## Caveat

These are **module bodies**, not per-function measurements — there are no call
frames or prologues in the m2vec numbers (nor in these particular C kernels,
whose functions are leaf loops). Once m2vec grows procedures, the kernels can be
rewritten as real callable functions and measured per-function, identically to
`../measure.sh`.
