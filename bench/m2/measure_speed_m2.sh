#!/bin/bash
# measure_speed_m2.sh - dynamic cycle counts for the m2vec (Modula-2) kernel
# ports, next to the three C compilers from ../measure_speed.sh.
#
# Method (matches the parent benchmark): each kernel is built twice -- "full"
# (data init + kernel) and "base" (init only) -- run in the cycle-accurate
# exec09 runner, and kernel cycles = full - base (the init cancels out).
#
# The m2vec speed kernels live in speed/*.mod and mark the timed region with
#     (*<KERNEL>*) ... (*</KERNEL>*)
# The base build is derived by stripping that region. m2vec's --bench mode emits
# a _run routine at $1000 ending in RTS, which the runner times entry-to-return.
set -u
cd "$(dirname "$0")"

M2VEC_DIR=${M2VEC_DIR:-$HOME/projects/m2vec-dev}
M2VEC=$M2VEC_DIR/target/release/m2vec
RUNNER=${RUNNER:-$HOME/retro-tools/6809-compilers/sim/runner}
OUT=/tmp/spd_m2; mkdir -p "$OUT"

[ -x "$M2VEC" ] || ( cd "$M2VEC_DIR" && cargo build --release -q ) || { echo "cannot build m2vec"; exit 1; }

# Compile a .mod in --bench mode and time it in the simulator. Echoes cycles/ERR.
run_cycles() { # $1 = mod file, $2 = tag
  local bin="$OUT/$2.bin"
  "$M2VEC" "$1" --bench -o "$bin" >"$OUT/$2.log" 2>&1 || { echo ERR; return; }
  # --bench lays code at $1000 with _run first, so org = entry = 0x1000.
  local c
  c=$("$RUNNER" "$bin" 1000 1000 2>"$OUT/$2.simerr")
  [[ "$c" =~ ^[0-9]+$ ]] && echo "$c" || echo ERR
}

# Kernel cycles = full - base, isolating the timed region.
m2_speed() { # $1 = kernel
  local full="speed/$1.mod" base="$OUT/$1.base.mod"
  [ -f "$full" ] || { echo "-"; return; }
  awk '/\(\*<KERNEL>\*\)/{k=1;next} /\(\*<\/KERNEL>\*\)/{k=0;next} !k' "$full" > "$base"
  local f b
  f=$(run_cycles "$full" "$1.full")
  b=$(run_cycles "$base" "$1.base")
  [[ "$f" =~ ^[0-9]+$ && "$b" =~ ^[0-9]+$ ]] && echo $((f - b)) || echo ERR
}

# Reference cycle counts (from ../README.md speed table). "cmoc vbcc gcc6809".
ref() {
  case "$1" in
    objmove)  echo "3648 4300 1509"   ;;
    collide)  echo "20873 12110 7644" ;;
    fixmul)   echo "15096 44611 -"    ;;
    rng)      echo "7360 6068 3583"   ;;
    memops)   echo "4152 2434 1726"   ;;
    strupr)   echo "1810 1197 1444"   ;;
    checksum) echo "9316 9090 3657"   ;;
    isort)    echo "17329 8703 6553"  ;;
    statem)   echo "178 88 115"       ;;
    bcdscore) echo "650 354 342"      ;;
    clamp)    echo "2695 1665 1473"   ;;
    *)        echo "- - -"            ;;
  esac
}

KERNELS="objmove collide fixmul rng memops strupr checksum isort statem bcdscore clamp"

printf "%-10s %10s %10s %10s %10s\n" "kernel" "cmoc" "vbcc" "gcc6809" "m2vec"
printf "%-10s %10s %10s %10s %10s\n" "------" "----" "----" "-------" "-----"
for k in $KERNELS; do
  set -- $(ref "$k")
  printf "%-10s %10s %10s %10s %10s\n" "$k" "$1" "$2" "$3" "$(m2_speed "$k")"
done

echo
echo "Cycles = full(init+kernel) - base(init only), in the exec09 core. m2vec is"
echo "an early compiler; slower code is expected (see README.md). fixmul gcc6809"
echo "is n/a in the parent benchmark (libgcc 32-bit multiply not linkable)."
