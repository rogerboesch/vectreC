#!/bin/bash
# measure_speed.sh - dynamic cycle counts via the exec09-core runner.
# For each compiler x kernel, build the driver twice: full (init + kernel call)
# and baseline (init only, -DNOKERNEL).  Link each to a 64K image, run to the
# RTS sentinel, and report kernel cycles = full - baseline (init cancels out).
set -u
cd "$(dirname "$0")"
DRV=drv; OUT=/tmp/spd; mkdir -p "$OUT"
VECTREC=${VECTREC:-/Users/roger/retro-tools/vectrec}
VB=${VB:-/Users/roger/retro-tools/6809-compilers/vbcc}
CC1=${CC1:-/Users/roger/retro-tools/6809-compilers/gcc6809/bin/cc1}; AS=/Users/roger/retro-tools/6809-compilers/gcc6809/bin/as6809; LD=/Users/roger/retro-tools/6809-compilers/gcc6809/bin/aslink
RUNNER=/Users/roger/retro-tools/6809-compilers/sim/runner; MK=/Users/roger/retro-tools/6809-compilers/sim/mkimg.py
GCCLIB=/Users/roger/retro-tools/6809-compilers/gcc6809/lib; GHELP="$GCCLIB/mulhi3.rel $GCCLIB/ashlhi3.rel $GCCLIB/ashrhi3.rel $GCCLIB/lshrhi3.rel $GCCLIB/divAndMod.rel"

cat > "$OUT/cmoc.link" <<'EOF'
section code load 1000
section rodata
section rwdata
section bss load 4000
entry _run
EOF
cat > "$OUT/vbcc.ld" <<'EOF'
MEMORY { ram: org=0x1000, len=0xE000 }
SECTIONS { .text:{*(.text)}>ram .data:{*(.data)}>ram .rodata:{*(.rodata)}>ram .bss(NOLOAD):{*(.bss)}>ram }
EOF

cmoc_run() { # $1=kernel $2=extra-define -> cycles or ERR
  local d=$2 t=$OUT/$1.cmoc$3
  ( cd $DRV && $VECTREC/cmoc -S --vectrex $d t_$1.c 2>/dev/null ) || return
  mv $DRV/t_$1.s $t.s 2>/dev/null || return
  $VECTREC/lwasm -fobj --output=$t.o $t.s 2>/dev/null || { echo ERR; return; }
  $VECTREC/lwlink --format=raw --script=$OUT/cmoc.link --map=$t.map -L$VECTREC/stdlib -lcmoc-std-vec -lcmoc-crt-vec -o $t.bin $t.o 2>/dev/null || { echo ERR; return; }
  python3 $MK raw $t.bin 1000 > $t.img
  local a=$(grep 'Symbol: _run ' $t.map | grep -oE '= [0-9a-fA-F]+' | awk '{print $2}')
  [ -n "$a" ] && $RUNNER $t.img 0 $a 2>/dev/null || echo ERR
}
vbcc_run() {
  local d=$2 t=$OUT/$1.vbcc$3
  $VB/bin/vbcchc12 -cpu=6809 -quiet -O=255 $d $DRV/t_$1.c -o=$t.s 2>/dev/null || { echo ERR; return; }
  $VB/bin/vasm6809_std -quiet -nowarn=62 -opt-branch -opt-offset -Fvobj $t.s -o $t.o 2>/dev/null || { echo ERR; return; }
  $VB/bin/vlink -b rawbin1 -T $OUT/vbcc.ld -L$VB/targets/6809-sim/lib -lvc -M$t.map -o $t.bin $t.o 2>/dev/null || { echo ERR; return; }
  python3 $MK raw $t.bin 1000 > $t.img
  local a=$(grep -iE '\brun\b' $t.map | grep -oiE '0x[0-9a-f]+' | head -1)
  a=${a#0x}; [ -n "$a" ] && $RUNNER $t.img 0 $a 2>/dev/null || echo ERR
}
gcc_run() {
  local d=$2 t=$OUT/$1.gcc$3
  # cc1 ignores -D, so preprocess macros with the host compiler first.
  cc -E -P $d $DRV/t_$1.c -o $t.i 2>/dev/null || { echo ERR; return; }
  $CC1 -quiet -O2 $t.i -o $t.s 2>/dev/null || { echo ERR; return; }
  local ob="g_$1${3//./_}"   # ASxxxx mangles dotted output basenames; keep it dotless
  ( cp $t.s $OUT/$ob.s && cd $OUT && $AS -o $ob.s ) >/dev/null 2>&1 || { echo ERR; return; }
  # ASxxxx aslink: first non-flag arg is the OUTPUT basename, rest are inputs.
  # Distinct base per area: gcc uses .text/.data/.bss, the VIDE helpers use _CODE/_DATA.
  ( cd $OUT && $LD -i -m -b .text=0x1000 -b _CODE=0x2000 -b .data=0x4000 -b .bss=0x4800 -b _DATA=0x5000 \
       o$ob $ob.rel $GHELP ) >$t.lderr 2>&1
  grep -qi 'undefined' $t.lderr && { echo "ERR(helper)"; return; }
  [ -s "$OUT/o$ob.ihx" ] || { echo ERR; return; }
  python3 $MK ihex $OUT/o$ob.ihx > $t.img
  local a=$(grep -oE '[0-9A-Fa-f]{4}  _run\b' $OUT/o$ob.map | awk '{print $1}' | head -1)
  [ -n "$a" ] && $RUNNER $t.img 0 $a 2>/dev/null || echo ERR
}

sub() { # full base -> kernel cycles (or pass through errors)
  [[ "$1" =~ ^[0-9]+$ && "$2" =~ ^[0-9]+$ ]] && echo $(($1-$2)) || echo "${1}"
}

BENCHES="objmove collide fixmul rng memops strupr checksum isort statem bcdscore clamp"
printf "%-10s %10s %10s %10s\n" kernel cmoc vbcc gcc6809
printf "%-10s %10s %10s %10s\n" ------ ---- ---- -------
for b in $BENCHES; do
  cf=$(cmoc_run $b "" .full);  cb=$(cmoc_run $b "-DNOKERNEL" .base);  c=$(sub "$cf" "$cb")
  vf=$(vbcc_run $b "" .full);  vb=$(vbcc_run $b "-DNOKERNEL" .base);  v=$(sub "$vf" "$vb")
  gf=$(gcc_run  $b "" .full);  gb=$(gcc_run  $b "-DNOKERNEL" .base);  g=$(sub "$gf" "$gb")
  printf "%-10s %10s %10s %10s\n" "$b" "$c" "$v" "$g"
done