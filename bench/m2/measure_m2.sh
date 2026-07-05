#!/bin/bash
# measure_m2.sh - compile each m2vec (Modula-2) kernel port and report the
# generated machine-code size, next to the three C compilers from ../measure.sh.
#
# Size metric (matches ../measure.sh intent): bytes of the kernel's own code,
# excluding runtime-helper bodies but including the call sites that invoke them.
#
# m2vec compiles a whole cartridge ROM laid out as:
#     [cartridge header] [kernel body] _m2vec_halt [runtime helpers] [ROM data]
# so the kernel body is exactly the bytes between the end of the header and the
# _m2vec_halt label. The runtime helpers (__mul16, __fixmul16, ...) sit after
# _m2vec_halt and are excluded, just as the C measurement excludes libgcc/cmoc
# helper bodies. We read the _m2vec_halt address from an lwasm listing:
#
#     kernel = addr(_m2vec_halt) - header_size
#
# The header is deterministic: 19 fixed bytes + the (upper-cased) MODULE name.
set -u
cd "$(dirname "$0")"

M2VEC_DIR=${M2VEC_DIR:-$HOME/projects/vecatac/m2vec}
M2VEC=$M2VEC_DIR/target/release/m2vec
LWASM=${LWASM:-$HOME/retro-tools/vectrec/lwasm}
OUT=/tmp/bench_m2; mkdir -p "$OUT"

if [ ! -x "$M2VEC" ]; then
  echo "building m2vec release binary..."
  ( cd "$M2VEC_DIR" && cargo build --release -q ) || { echo "cannot build m2vec"; exit 1; }
fi

# m2vec kernel code size in bytes, or ERR.
m2_size() { # $1 = basename (module file without .mod)
  local mod="$1.mod" bin="$OUT/$1.bin" asm="$OUT/$1.asm" lst="$OUT/$1.lst"
  "$M2VEC" "$mod" -o "$bin" >"$OUT/$1.log" 2>&1 || { echo ERR; return; }
  # m2vec writes the assembly next to the ROM; re-assemble it for a listing.
  "$LWASM" --raw --output="$OUT/$1.reasm.bin" --list="$lst" "$asm" >/dev/null 2>&1 || { echo ERR; return; }
  local halt_hex halt name title_len header
  halt_hex=$(awk '/_m2vec_halt:$/ { print $1; exit }' "$lst")
  [ -n "$halt_hex" ] || { echo ERR; return; }
  halt=$((16#$halt_hex))
  name=$(awk '/MODULE/ { gsub(/;/, "", $2); print $2; exit }' "$mod")
  title_len=${#name}
  header=$(( 19 + title_len ))
  echo $(( halt - header ))
}

# Reference numbers (from ../README.md, cmoc/vbcc/gcc6809 code-size table).
# Kept as a case for bash 3.2 (macOS) which has no associative arrays.
ref() { # $1 = kernel -> "cmoc vbcc gcc6809"
  case "$1" in
    objmove)  echo "148 119 80"  ;;
    collide)  echo "206 203 131" ;;
    fixmul)   echo "112 116 89"  ;;
    rng)      echo "118 93 64"   ;;
    memops)   echo "58 56 41"    ;;
    strupr)   echo "54 49 46"    ;;
    checksum) echo "90 101 59"   ;;
    isort)    echo "117 111 59"  ;;
    statem)   echo "149 132 132" ;;
    bcdscore) echo "159 105 108" ;;
    clamp)    echo "122 103 139" ;;
    *)        echo "- - -"       ;;
  esac
}

KERNELS="objmove collide fixmul rng memops strupr checksum isort statem bcdscore clamp"

printf "%-10s %8s %8s %8s %8s\n" "kernel" "cmoc" "vbcc" "gcc6809" "m2vec"
printf "%-10s %8s %8s %8s %8s\n" "------" "----" "----" "-------" "-----"
tc=0; tv=0; tg=0; tm=0
for k in $KERNELS; do
  set -- $(ref "$k"); c=$1; v=$2; g=$3
  m=$(m2_size "$k")
  printf "%-10s %8s %8s %8s %8s\n" "$k" "$c" "$v" "$g" "$m"
  tc=$((tc+c)); tv=$((tv+v)); tg=$((tg+g))
  [[ "$m" =~ ^[0-9]+$ ]] && tm=$((tm+m))
done
printf "%-10s %8s %8s %8s %8s\n" "------" "----" "----" "-------" "-----"
printf "%-10s %8s %8s %8s %8s\n" "TOTAL" "$tc" "$tv" "$tg" "$tm"

echo
echo "m2vec is an early compiler (no register allocation, stack-based expression"
echo "evaluation, 16-bit INTEGER where C uses 8-bit types, long branches). Larger"
echo "code is expected; see README.md for the analysis and per-kernel notes."
