# 6809 C compiler comparison: cmoc vs gcc6809 vs vbcc

How well each compiler optimizes **code size** and **execution speed** on
representative Vectrex game kernels (Motorola 6809).

## Compilers under test

| Compiler | Version | How it was obtained on this Apple-Silicon Mac |
|----------|---------|-----------------------------------------------|
| **cmoc**    | 0.1.67          | Native arm64 build (already installed in `~/retro-tools/vectrec`). |
| **vbcc**    | 0.9g, `vbcchc12` (6809/6309/68HC12 backend), `-O=255` | Built from source: `vbcc.tar.gz` + `vasm6809_std` + `vlink`. |
| **gcc6809** | GCC 4.3.6 (`gcc6809`/dftools), `-O2` | Prebuilt x86_64 `cc1`, run under **Rosetta 2**. |

> Building gcc6809 natively on arm64-darwin was attempted and **fails**: the
> toolchain compiles (after ~10 patches for modern clang/macOS — see notes
> below), but the resulting `cc1` crashes (`EXC_BAD_ACCESS` in
> `mark_jump_label_1`) on any function with **2+ parameters** — an
> aarch64-host codegen bug.

## Benchmarks (`src/`)

11 freestanding, pure-C kernels modelled on real Vectrex game code, using only
local fixed-width typedefs (no headers) so the *identical* source compiles on
all three compilers. `int` is 16-bit and `long` is 32-bit on all three.

`objmove` (sprite movement+wrap), `collide` (AABB collision), `fixmul` (Q8.8
fixed-point scaling, 32-bit mul), `rng` (xorshift16), `memops` (memcpy/memset),
`strupr` (string upcase), `checksum` (ROM hash), `isort` (insertion sort),
`statem` (switch state machine), `bcdscore` (BCD add), `clamp` (signed clamp).

## Results

### Code size — bytes of generated machine code (lower = better)

| kernel   | cmoc | vbcc | gcc6809 |
|----------|-----:|-----:|--------:|
| objmove  |  148 |  119 |  **80** |
| collide  |  206 |  203 | **131** |
| fixmul   |  112 |  116 |  **89** |
| rng      |  118 |   93 |  **64** |
| memops   |   58 |   56 |  **41** |
| strupr   |   54 |   49 |  **46** |
| checksum |   90 |  101 |  **59** |
| isort    |  117 |  111 |  **59** |
| statem   |  149 |  132 | **132** |
| bcdscore |  159 |  105 |     108 |
| clamp    |  122 |  103 |     139 |
| **TOTAL**| **1333** | **1188** | **948** |

gcc6809 is smallest overall (**~29% smaller than cmoc, ~20% smaller than vbcc**),
winning 9/11. vbcc is consistently between the two. cmoc is largest, mainly from
its U-frame-pointer prologue (`PSHS U / LEAU ,S / LEAS -n,S`) on every function.

### Whole-program size — a complete Pong (`src/pong.c`)

The kernels are tiny loops; a whole game mixes them with control flow, rendering
and a main loop. `pong.c` is a full Vectrex Pong (ball physics, paddle AI,
collision, scoring, draw list) — portable C with the BIOS routines as uniform
`extern` calls, so the comparison stays apples-to-apples (BIOS bodies excluded,
call sites included). Code size of the whole program:

| program | cmoc | vbcc | gcc6809 |
|---------|-----:|-----:|--------:|
| pong    | 1092 |  874 | **789** |

Normalised (gcc6809 = 1.00): **cmoc 1.38 · vbcc 1.11 · gcc6809 1.00**. Same
ordering as the kernels, but on real whole-program code **vbcc closes most of the
gap to gcc6809** (1.11 here vs 1.25 on the kernel aggregate) — gcc6809's biggest
wins come from arithmetic-heavy inner loops (strength reduction), which are a
smaller fraction of a full game. cmoc stays ~1.4× larger; its per-function
prologue overhead scales with the number of functions. (No speed row: Pong is a
BIOS-driven infinite frame loop, not a measurable kernel.)

### Speed — dynamic cycle count of the kernel (lower = better)

Measured by running the linked machine code in a cycle-accurate 6809 simulator
(exec09 core). For each kernel the driver is built twice — with and without the
kernel call — and the init cost is subtracted, isolating the kernel.

| kernel   |  cmoc |  vbcc | gcc6809 |
|----------|------:|------:|--------:|
| objmove  |  3648 |  4300 | **1509** |
| collide  | 20873 | 12110 | **7644** |
| fixmul   | **15096** | 44611 | n/a¹ |
| rng      |  7360 |  6068 | **3583** |
| memops   |  4152 |  2434 | **1726** |
| strupr   |  1810 | **1197** | 1444 |
| checksum |  9316 |  9090 | **3657** |
| isort    | 17329 |  8703 | **6553** |
| statem   |   178 |  **88** |  115 |
| bcdscore |   650 |   354 | **342** |
| clamp    |  2695 |  1665 | **1473** |

¹ gcc6809 `fixmul` needs `__mulsi3` (32-bit multiply), which lives in `libgcc`
and is not bundled with gcc — so it can't be linked/run here. (Its *size*, 89 B,
is still the smallest.)

**gcc6809 is fastest on 8 of the 10 runnable kernels**, often by ~2×
(e.g. `checksum` 3657 vs ~9300; `objmove` 1509 vs 3648/4300). Its big wins come
from strength reduction (replacing index multiplies with pointer adds —
`leax 6,x` / `leay 200,y`) and tail calls. vbcc wins `strupr` and `statem`;
cmoc wins `fixmul` (its 32-bit multiply path is **3× faster than vbcc's**).

## Takeaways

- **gcc6809 is the strongest optimizer** for both size and speed — it is a real
  optimizing GCC backend (strength reduction, tail calls, good register use).
  Cost: it's a 2008-era GCC 4.3.6 that is painful to build on modern hosts.
- **vbcc** is a solid, modern, easy-to-build middle ground: ~20% larger and
  generally slower than gcc6809, but far better than cmoc on most kernels, with
  a notable weakness in 32-bit multiply.
- **cmoc** is the simplest/most portable and builds cleanly everywhere, but
  generates the largest and (usually) slowest code; its per-function
  frame-pointer prologue and helper-call-heavy 16-bit math cost it. It does win
  `objmove` and `fixmul`.

## Reproduce

```sh
bench/measure.sh        # code size, 3-way
bench/measure_speed.sh  # dynamic cycle counts, 3-way
```

Paths to the toolchains/simulator are set at the top of each script.
