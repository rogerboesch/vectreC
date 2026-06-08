#!/bin/bash
# measure.sh - compile each benchmark with cmoc, vbcc (and gcc6809 if present)
# and report generated machine-code size in bytes per kernel.
#
# Size metric: bytes of the code/text section of the assembled object for the
# benchmark's own functions (runtime-helper bodies live in libraries and are
# excluded; the call sites that invoke them are included).
set -u
cd "$(dirname "$0")/src"

VECTREC=${VECTREC:-/Users/roger/retro-tools/vectrec}
VB=${VB:-/Users/roger/retro-tools/6809-compilers/vbcc}
GCC_CC1=${GCC_CC1:-/Users/roger/retro-tools/6809-compilers/gcc6809/bin/cc1}        # VIDE prebuilt gcc6809 cc1 (x86_64/Rosetta)
VIDE_AS6809=${VIDE_AS6809:-/Users/roger/retro-tools/6809-compilers/gcc6809/bin/as6809}
OUT=/tmp/bench_out; mkdir -p "$OUT"

BENCHES="objmove collide fixmul rng memops strupr checksum isort statem bcdscore clamp"

# --- size extractors -------------------------------------------------------
cmoc_size() { # $1=basename -> echo bytes (decimal) or ERR
  $VECTREC/cmoc --vectrex -S "$1.c" -o /dev/null >/dev/null 2>"$OUT/$1.cmoc.err"
  $VECTREC/cmoc --vectrex -S "$1.c" >"$OUT/$1.cmoc.log" 2>&1
  [ -f "$1.s" ] || { echo ERR; return; }
  mv "$1.s" "$OUT/$1.cmoc.s"
  $VECTREC/lwasm -fobj --output="$OUT/$1.cmoc.o" "$OUT/$1.cmoc.s" 2>"$OUT/$1.cmoc.aserr" || { echo ERR; return; }
  local hex
  hex=$($VECTREC/lwobjdump "$OUT/$1.cmoc.o" 2>/dev/null | awk '
    /^SECTION code$/ {inc=1}
    inc && /CODE [0-9a-fA-F]+ bytes/ {print $2; exit}')
  [ -n "$hex" ] && printf "%d" "0x$hex" || echo ERR
}

vbcc_size() { # $1=basename
  $VB/bin/vbcchc12 -cpu=6809 -quiet -O=255 "$1.c" -o="$OUT/$1.vbcc.s" 2>"$OUT/$1.vbcc.err" || { echo ERR; return; }
  $VB/bin/vasm6809_std -quiet -nowarn=62 -opt-branch -opt-offset -Fvobj "$OUT/$1.vbcc.s" -o "$OUT/$1.vbcc.o" 2>"$OUT/$1.vbcc.aserr" || { echo ERR; return; }
  $VB/bin/vobjdump "$OUT/$1.vbcc.o" 2>/dev/null | awk -F'Total size:' '/Total size:/{print $2+0; exit}'
}

gcc_size() { # $1=basename ; needs GCC_CC1 (VIDE cc1, x86_64/Rosetta) + as6809
  [ -n "$GCC_CC1" ] || { echo "-"; return; }
  "$GCC_CC1" -quiet -O2 -I. "$1.c" -o "$OUT/$1.gcc.s" 2>"$OUT/$1.gcc.err" || { echo ERR; return; }
  # as6809 (ASxxxx) emits a listing whose Area Table reports ".text size <hex>".
  ( cd "$OUT" && "$VIDE_AS6809" -l -o "$1.gcc.s" >/dev/null 2>"$1.gcc.aserr" )
  local hex
  hex=$(awk '/[0-9]+ \.text +size/ {for(i=1;i<=NF;i++) if($i=="size"){print $(i+1);exit}}' "$OUT/$1.gcc.lst" 2>/dev/null)
  [ -n "$hex" ] && printf "%d" "0x$hex" || echo ERR
}

printf "%-10s %10s %10s %10s\n" "kernel" "cmoc" "vbcc" "gcc6809"
printf "%-10s %10s %10s %10s\n" "------" "----" "----" "-------"
tc=0; tv=0; tg=0
for b in $BENCHES; do
  c=$(cmoc_size "$b"); v=$(vbcc_size "$b"); g=$(gcc_size "$b")
  printf "%-10s %10s %10s %10s\n" "$b" "$c" "$v" "$g"
  [[ "$c" =~ ^[0-9]+$ ]] && tc=$((tc+c))
  [[ "$v" =~ ^[0-9]+$ ]] && tv=$((tv+v))
  [[ "$g" =~ ^[0-9]+$ ]] && tg=$((tg+g))
done
printf "%-10s %10s %10s %10s\n" "------" "----" "----" "-------"
printf "%-10s %10s %10s %10s\n" "TOTAL" "$tc" "$tv" "$tg"

# whole-program size: a complete Pong game (not summed into the kernel total)
echo
printf "%-10s %10s %10s %10s\n" "whole-app" "cmoc" "vbcc" "gcc6809"
printf "%-10s %10s %10s %10s\n" "---------" "----" "----" "-------"
printf "%-10s %10s %10s %10s\n" "pong" "$(cmoc_size pong)" "$(vbcc_size pong)" "$(gcc_size pong)"
