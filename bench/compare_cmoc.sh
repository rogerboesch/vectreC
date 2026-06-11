#!/bin/bash
# compare_cmoc.sh - compare two cmoc versions (size + speed) on the bench kernels.
# Column A = $VEC_A (default: installed 0.1.67), column B = $VEC_B (0.1.98 staging).
# Each VEC dir must contain: cmoc, lwasm, lwlink, lwobjdump, stdlib/libcmoc-{std,crt}-vec.a
set -u
cd "$(dirname "$0")"
ROOT=$(pwd)
VEC_A=${VEC_A:-/Users/roger/retro-tools/vectrec}          # 0.1.67
VEC_B=${VEC_B:-/tmp/vec98}                                 # 0.1.98
OUT=/tmp/cmpcmoc; mkdir -p "$OUT"
MK=/Users/roger/retro-tools/6809-compilers/sim/mkimg.py
RUNNER=/Users/roger/retro-tools/6809-compilers/sim/runner
BENCHES="objmove collide fixmul rng memops strupr checksum isort statem bcdscore clamp"

cat > "$OUT/cmoc.link" <<'EOF'
section code load 1000
section rodata
section rwdata
section bss load 4000
entry _run
EOF

# --- code size (bytes of the kernel's own code section) -------------------
size() { # $1=VECdir $2=kernel -> bytes or ERR
  local V=$1 b=$2 t="$OUT/$2.$(basename $1)"
  ( cd "$ROOT/src" && "$V/cmoc" --vectrex -S "$b.c" >/dev/null 2>&1; mv "$b.s" "$t.s" 2>/dev/null )
  [ -f "$t.s" ] || { echo ERR; return; }
  "$V/lwasm" -fobj --output="$t.o" "$t.s" 2>/dev/null || { echo ERR; return; }
  local hex
  hex=$("$V/lwobjdump" "$t.o" 2>/dev/null | awk '
    /^SECTION code$/{inc=1} inc && /CODE [0-9a-fA-F]+ bytes/{print $2; exit}')
  [ -n "$hex" ] && printf "%d" "0x$hex" || echo ERR
}

# --- dynamic cycle count (full - baseline) --------------------------------
run1() { # $1=VECdir $2=kernel $3=define $4=tag -> cycles or ERR
  local V=$1 b=$2 d=$3 t="$OUT/$2.$(basename $1)$4"
  ( cd "$ROOT/drv" && "$V/cmoc" -S --vectrex $d "t_$b.c" 2>/dev/null; mv "t_$b.s" "$t.s" 2>/dev/null )
  [ -f "$t.s" ] || { echo ERR; return; }
  "$V/lwasm" -fobj --output="$t.o" "$t.s" 2>/dev/null || { echo ERR; return; }
  "$V/lwlink" --format=raw --script="$OUT/cmoc.link" --map="$t.map" \
     -L"$V/stdlib" -lcmoc-std-vec -lcmoc-crt-vec -o "$t.bin" "$t.o" 2>/dev/null || { echo ERR; return; }
  python3 "$MK" raw "$t.bin" 1000 > "$t.img" 2>/dev/null
  local a; a=$(grep 'Symbol: _run ' "$t.map" | grep -oE '= [0-9a-fA-F]+' | awk '{print $2}')
  [ -n "$a" ] && "$RUNNER" "$t.img" 0 "$a" 2>/dev/null || echo ERR
}
speed() { # $1=VECdir $2=kernel
  local f base; f=$(run1 "$1" "$2" "" .full); base=$(run1 "$1" "$2" "-DNOKERNEL" .base)
  [[ "$f" =~ ^[0-9]+$ && "$base" =~ ^[0-9]+$ ]] && echo $((f-base)) || echo ERR
}

pct() { [[ "$1" =~ ^[0-9]+$ && "$2" =~ ^[0-9]+$ && "$1" -ne 0 ]] && awk "BEGIN{printf \"%+.1f%%\", ($2-$1)*100.0/$1}" || echo "-"; }

echo "## Code size (bytes, lower=better)"
printf "%-10s %8s %8s %8s\n" kernel 0.1.67 0.1.98 delta
printf "%-10s %8s %8s %8s\n" ------ ------ ------ -----
ta=0; tb=0
for k in $BENCHES; do
  a=$(size "$VEC_A" "$k"); b=$(size "$VEC_B" "$k")
  printf "%-10s %8s %8s %8s\n" "$k" "$a" "$b" "$(pct "$a" "$b")"
  [[ "$a" =~ ^[0-9]+$ ]] && ta=$((ta+a)); [[ "$b" =~ ^[0-9]+$ ]] && tb=$((tb+b))
done
printf "%-10s %8s %8s %8s\n" ------ ------ ------ -----
printf "%-10s %8s %8s %8s\n" TOTAL "$ta" "$tb" "$(pct "$ta" "$tb")"

echo; echo "## Speed (kernel cycles, lower=better)"
printf "%-10s %8s %8s %8s\n" kernel 0.1.67 0.1.98 delta
printf "%-10s %8s %8s %8s\n" ------ ------ ------ -----
for k in $BENCHES; do
  a=$(speed "$VEC_A" "$k"); b=$(speed "$VEC_B" "$k")
  printf "%-10s %8s %8s %8s\n" "$k" "$a" "$b" "$(pct "$a" "$b")"
done
