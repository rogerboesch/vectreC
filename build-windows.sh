#!/bin/bash
#
# build-windows.sh - Build and install CMOC 6809 cross-compiler for Vectrex
#                    development on Windows (MSYS2 / MinGW-w64).
#
# This script builds CMOC from source and installs it along with the stdlib
# and lwtools into the VectreC toolchain directory. The resulting cmoc.exe
# is a native Windows binary usable from PowerShell or cmd.exe.
#
# Usage (inside an MSYS2 MINGW64 shell):
#   ./build-windows.sh              Build and install to default location
#   ./build-windows.sh /custom/path Build and install to custom location
#
# Prerequisites:
#   - MSYS2 (https://www.msys2.org) - run this script from the MINGW64 shell.
#     Required packages are installed automatically via pacman if missing.
#

set -e

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
INSTALL_DIR="${1:-$USERPROFILE/retro-tools/vectrec}"
INSTALL_DIR="$(cygpath -u "$INSTALL_DIR" 2>/dev/null || echo "$INSTALL_DIR")"
JOBS="$(nproc 2>/dev/null || echo 4)"

LWTOOLS_VER="4.24"
LWTOOLS_URL="http://www.lwtools.ca/releases/lwtools/lwtools-$LWTOOLS_VER.tar.gz"
LWTOOLS_BUILD_DIR="$SCRIPT_DIR/.lwtools-build"

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

info()  { printf "\033[1;34m==>\033[0m %s\n" "$1"; }
ok()    { printf "\033[1;32m==>\033[0m %s\n" "$1"; }
warn()  { printf "\033[1;33m==>\033[0m %s\n" "$1"; }
fail()  { printf "\033[1;31m==>\033[0m %s\n" "$1" >&2; exit 1; }

check_cmd() {
    command -v "$1" >/dev/null 2>&1
}

# ---------------------------------------------------------------------------
# Step 1: Check environment
# ---------------------------------------------------------------------------

if [ "$MSYSTEM" != "MINGW64" ]; then
    fail "This script must be run from an MSYS2 MINGW64 shell.
     Install MSYS2 from https://www.msys2.org (or: winget install MSYS2.MSYS2),
     then open 'MSYS2 MINGW64' from the Start menu and re-run this script."
fi

# ---------------------------------------------------------------------------
# Step 2: Check / install prerequisites
# ---------------------------------------------------------------------------

info "Checking prerequisites..."

PACKAGES=""
check_cmd g++      || PACKAGES="$PACKAGES mingw-w64-x86_64-gcc"
check_cmd make     || PACKAGES="$PACKAGES make"
check_cmd bison    || PACKAGES="$PACKAGES bison"
check_cmd flex     || PACKAGES="$PACKAGES flex"
check_cmd autoconf || PACKAGES="$PACKAGES autoconf"
check_cmd automake || PACKAGES="$PACKAGES automake"
check_cmd perl     || PACKAGES="$PACKAGES perl"
check_cmd curl     || PACKAGES="$PACKAGES curl"
check_cmd tar      || PACKAGES="$PACKAGES tar"

if [ -n "$PACKAGES" ]; then
    info "Installing missing packages:$PACKAGES"
    pacman -S --noconfirm --needed $PACKAGES
fi

# cpp.exe (GNU C preprocessor) is needed by cmoc at runtime.
check_cmd cpp || fail "cpp not found even after installing gcc; check the MinGW64 installation."

ok "All prerequisites satisfied"

# ---------------------------------------------------------------------------
# Step 3: Build lwtools from source (if not already available)
# ---------------------------------------------------------------------------

mkdir -p "$INSTALL_DIR"

if [ -x "$INSTALL_DIR/lwasm.exe" ]; then
    info "lwtools already installed in $INSTALL_DIR, skipping build"
else
    info "Building lwtools $LWTOOLS_VER from source..."
    mkdir -p "$LWTOOLS_BUILD_DIR"
    cd "$LWTOOLS_BUILD_DIR"

    if [ ! -f "lwtools-$LWTOOLS_VER.tar.gz" ]; then
        info "Downloading $LWTOOLS_URL ..."
        curl -fL -o "lwtools-$LWTOOLS_VER.tar.gz" "$LWTOOLS_URL"
    fi

    rm -rf "lwtools-$LWTOOLS_VER"
    tar xzf "lwtools-$LWTOOLS_VER.tar.gz"
    cd "lwtools-$LWTOOLS_VER"
    make -j"$JOBS"

    for tool in lwasm/lwasm lwlink/lwlink lwar/lwar lwlink/lwobjdump; do
        name="$(basename "$tool")"
        if [ -f "$tool.exe" ]; then
            cp -f "$tool.exe" "$INSTALL_DIR/$name.exe"
        elif [ -f "$tool" ]; then
            cp -f "$tool" "$INSTALL_DIR/$name.exe"
        else
            warn "$name not found in lwtools build, skipping"
        fi
    done

    cd "$SCRIPT_DIR"
fi

# Make lwasm/lwlink/lwar visible to cmoc's configure script and to the
# stdlib build, which invoke them by name.
export PATH="$INSTALL_DIR:$PATH"

check_cmd lwasm || fail "lwasm not found after lwtools build"
LWASM_VER="$(lwasm --version 2>&1 | grep -oE '[0-9]+\.[0-9]+' | head -1)"
info "lwtools version: $LWASM_VER"

# ---------------------------------------------------------------------------
# Step 4: Build CMOC compiler
# ---------------------------------------------------------------------------

cd "$SCRIPT_DIR"

# Clean previous build if any
if [ -f Makefile ]; then
    info "Cleaning previous build..."
    make clean >/dev/null 2>&1 || true
fi

info "Configuring CMOC..."
# - cygpath -m gives a mixed-style prefix (C:/...) that is valid both in
#   MSYS2 and from PowerShell/cmd (it is compiled into cmoc as PKGDATADIR).
# - Static linking avoids dependencies on the MinGW runtime DLLs, so that
#   cmoc.exe runs outside the MSYS2 environment.
./bootstrap
./configure --prefix="$(cygpath -m "$INSTALL_DIR")" \
            --without-writecocofile \
            LDFLAGS="-static -static-libgcc -static-libstdc++"

info "Building CMOC compiler (using $JOBS cores)..."
make -j"$JOBS"

ok "CMOC compiler + stdlib built successfully"

# ---------------------------------------------------------------------------
# Step 5: Install to target directory
# ---------------------------------------------------------------------------

info "Installing to $INSTALL_DIR ..."
mkdir -p "$INSTALL_DIR/stdlib"
mkdir -p "$INSTALL_DIR/stdlib/vectrex"

# CMOC compiler binary
cp -f src/cmoc.exe "$INSTALL_DIR/cmoc.exe"

# Standard library archives
for lib in src/stdlib/libcmoc-*.a; do
    [ -f "$lib" ] && cp -f "$lib" "$INSTALL_DIR/stdlib/"
done

# Standard library object files (vectrex target)
for obj in src/stdlib/*.vec_o; do
    [ -f "$obj" ] && cp -f "$obj" "$INSTALL_DIR/stdlib/"
done

# Header files
for hdr in src/stdlib/*.h; do
    [ -f "$hdr" ] && cp -f "$hdr" "$INSTALL_DIR/stdlib/"
done

# Vectrex-specific headers
for hdr in src/stdlib/vectrex/*.h; do
    [ -f "$hdr" ] && cp -f "$hdr" "$INSTALL_DIR/stdlib/vectrex/"
done

# Assembly include files
for inc in src/stdlib/*.inc; do
    [ -f "$inc" ] && cp -f "$inc" "$INSTALL_DIR/stdlib/"
done

# Generate a PowerShell environment script so the toolchain can be used
# outside MSYS2. cmoc invokes the GNU C preprocessor (cpp); CMOC_CPP tells
# it where to find MinGW64's cpp.exe. The MinGW64 bin directory must also be
# on the PATH because cpp.exe spawns cc1.exe, whose DLLs live in that
# directory (it is appended at the END so it cannot shadow user tools).
MINGW_BIN_WIN="$(cygpath -w /mingw64/bin)"
INSTALL_DIR_WIN="$(cygpath -w "$INSTALL_DIR")"
cat > "$INSTALL_DIR/vectrec-env.ps1" <<EOF
# VectreC toolchain environment (generated by build-windows.sh)
\$env:VECTREC = "$INSTALL_DIR_WIN"
\$env:CMOC_CPP = "$MINGW_BIN_WIN\\cpp.exe"
\$env:PATH = "\$env:VECTREC;\$env:PATH;$MINGW_BIN_WIN"
EOF

ok "Installation complete!"

# ---------------------------------------------------------------------------
# Step 6: Verify
# ---------------------------------------------------------------------------

info "Verifying installation..."

"$INSTALL_DIR/cmoc.exe" --version
"$INSTALL_DIR/lwasm.exe" --version 2>&1 | head -1

if [ -f "$INSTALL_DIR/stdlib/libcmoc-crt-vec.a" ] && \
   [ -f "$INSTALL_DIR/stdlib/libcmoc-std-vec.a" ] && \
   [ -f "$INSTALL_DIR/stdlib/vectrex.h" ]; then
    ok "Vectrex stdlib verified"
else
    warn "Some stdlib files may be missing — check $INSTALL_DIR/stdlib/"
fi

# Smoke test: compile a minimal Vectrex program with the installed toolchain.
info "Compiling a test Vectrex program..."
TESTDIR="$(mktemp -d)"
cat > "$TESTDIR/,check.c" <<'EOF'
#include <vectrex/bios.h>
#include <vectrex/stdlib.h>
int main() { moveto_d(0, 0); wait_recal(); return 0; }
EOF
"$INSTALL_DIR/cmoc.exe" --vectrex \
    -I "$(cygpath -m "$INSTALL_DIR/stdlib")" \
    -L "$(cygpath -m "$INSTALL_DIR/stdlib")" \
    -o "$TESTDIR/,check.bin" "$TESTDIR/,check.c"
if strings "$TESTDIR/,check.bin" 2>/dev/null | grep -q 'g GCE'; then
    ok "Test program compiled: Vectrex ROM header present"
else
    warn "Test program built but ROM header not detected — check manually"
fi
rm -rf "$TESTDIR"

echo ""
ok "CMOC toolchain installed at: $(cygpath -w "$INSTALL_DIR")"
echo ""
echo "  To use from PowerShell:"
echo "    . $INSTALL_DIR_WIN\\vectrec-env.ps1"
echo "    cmoc --vectrex -I \$env:VECTREC\\stdlib -L \$env:VECTREC\\stdlib -o game.bin game.c"
echo ""
