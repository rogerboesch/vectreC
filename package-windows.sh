#!/bin/bash
#
# package-windows.sh - Create a self-contained, redistributable zip of the
#                      VectreC toolchain for Windows.
#
# The resulting vectrec-win64.zip contains cmoc.exe, lwtools, the Vectrex
# stdlib and a bundled GNU C preprocessor (cpp + cc1 + DLLs), so end users
# can unzip and compile Vectrex programs without installing MSYS2.
#
# Usage (inside an MSYS2 MINGW64 shell, after ./build-windows.sh):
#   ./package-windows.sh [install-dir]
#

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
INSTALL_DIR="${1:-$USERPROFILE/retro-tools/vectrec}"
INSTALL_DIR="$(cygpath -u "$INSTALL_DIR" 2>/dev/null || echo "$INSTALL_DIR")"
STAGE_DIR="$SCRIPT_DIR/vectrec-win64"
ZIP_FILE="$SCRIPT_DIR/vectrec-win64.zip"

info()  { printf "\033[1;34m==>\033[0m %s\n" "$1"; }
ok()    { printf "\033[1;32m==>\033[0m %s\n" "$1"; }
fail()  { printf "\033[1;31m==>\033[0m %s\n" "$1" >&2; exit 1; }

[ "$MSYSTEM" = "MINGW64" ] || fail "Run this script from an MSYS2 MINGW64 shell."
[ -f "$INSTALL_DIR/cmoc.exe" ] || fail "Toolchain not found in $INSTALL_DIR. Run ./build-windows.sh first."

# Tools needed for packaging.
pacman -S --noconfirm --needed mingw-w64-x86_64-ntldd zip >/dev/null 2>&1 || true
command -v ntldd >/dev/null || fail "ntldd not found (pacman -S mingw-w64-x86_64-ntldd)"
command -v zip   >/dev/null || fail "zip not found (pacman -S zip)"

# ---------------------------------------------------------------------------
# Stage toolchain
# ---------------------------------------------------------------------------

info "Staging toolchain from $INSTALL_DIR ..."
rm -rf "$STAGE_DIR" "$ZIP_FILE"
mkdir -p "$STAGE_DIR"

cp -f "$INSTALL_DIR/cmoc.exe" "$STAGE_DIR/"
for tool in lwasm lwlink lwar lwobjdump; do
    [ -f "$INSTALL_DIR/$tool.exe" ] && cp -f "$INSTALL_DIR/$tool.exe" "$STAGE_DIR/"
done
cp -r "$INSTALL_DIR/stdlib" "$STAGE_DIR/stdlib"

# ---------------------------------------------------------------------------
# Bundle the GNU C preprocessor (cpp.exe + cc1plus.exe + DLLs)
#
# cpp.exe locates its back end relative to its own path (bin/../lib/gcc/...),
# so the MinGW64 directory layout is replicated inside the bundle. cmoc
# invokes cpp with -xc++, which uses the cc1plus back end (cc1 is therefore
# not needed). Each executable gets its DLL dependencies copied next to it,
# where the Windows loader finds them first.
# ---------------------------------------------------------------------------

info "Bundling C preprocessor..."

CC1_PATH="$(cpp -print-prog-name=cc1plus)"
[ -f "$CC1_PATH" ] || fail "cc1plus.exe not found via 'cpp -print-prog-name=cc1plus'"
CC1_RELDIR="$(dirname "$CC1_PATH" | sed 's|.*/mingw64/||')"   # e.g. lib/gcc/x86_64-w64-mingw32/16.1.0

mkdir -p "$STAGE_DIR/cpp/bin" "$STAGE_DIR/cpp/$CC1_RELDIR"
cp -f /mingw64/bin/cpp.exe "$STAGE_DIR/cpp/bin/"
cp -f "$CC1_PATH" "$STAGE_DIR/cpp/$CC1_RELDIR/"

copy_dlls() {  # copy_dlls <exe> <destdir>
    ntldd -R "$1" 2>/dev/null \
        | grep -io 'mingw64.bin.[a-zA-Z0-9_.+-]*\.dll' \
        | sed 's|.*[/\\]||' | sort -u \
        | while read -r dll; do
            cp -f "/mingw64/bin/$dll" "$2/"
          done
}
copy_dlls /mingw64/bin/cpp.exe "$STAGE_DIR/cpp/bin"
copy_dlls "$CC1_PATH" "$STAGE_DIR/cpp/$CC1_RELDIR"

# ---------------------------------------------------------------------------
# Environment script and README
# ---------------------------------------------------------------------------

cat > "$STAGE_DIR/vectrec-env.ps1" <<'EOF'
# VectreC toolchain environment (redistributable package)
$env:VECTREC = $PSScriptRoot
$env:CMOC_CPP = "$PSScriptRoot\cpp\bin\cpp.exe"
$env:PATH = "$env:VECTREC;$env:PATH"
EOF

# Ship the extensive Windows user guide and the example programs.
cp -f "$SCRIPT_DIR/cmoc/doc/README-WINDOWS.md" "$STAGE_DIR/README-WINDOWS.md"
mkdir -p "$STAGE_DIR/examples"
cp -f "$SCRIPT_DIR/examples/"*.c "$STAGE_DIR/examples/"

# ---------------------------------------------------------------------------
# Zip
# ---------------------------------------------------------------------------

info "Creating $ZIP_FILE ..."
(cd "$SCRIPT_DIR" && zip -q -r "$(basename "$ZIP_FILE")" "$(basename "$STAGE_DIR")")

ok "Package created: $(cygpath -w "$ZIP_FILE")"
du -sh "$STAGE_DIR" "$ZIP_FILE" | sed 's/^/    /'
