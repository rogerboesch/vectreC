#!/bin/bash
#
# build.sh - Build and install CMOC 6809 cross-compiler for Vectrex development
#
# This script builds CMOC from source and installs it along with the stdlib
# and lwtools into the VectreC toolchain directory.
#
# Usage:
#   ./build.sh              Build and install to default location
#   ./build.sh /custom/path Build and install to custom location
#
# Prerequisites (installed automatically via Homebrew if missing):
#   - lwtools (lwasm, lwlink, lwar) >= 4.11
#   - bison, flex
#   - C++ compiler (Xcode command line tools)
#

set -e

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
# The CMOC source tree (vendored upstream + Vectrex overlay) lives in cmoc/.
BUILD_DIR="$SCRIPT_DIR/cmoc"
INSTALL_DIR="${1:-$HOME/retro-tools/vectrec}"
JOBS="$(sysctl -n hw.ncpu 2>/dev/null || echo 4)"

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
# Step 1: Check / install prerequisites
# ---------------------------------------------------------------------------

info "Checking prerequisites..."

# Xcode command line tools (provides clang++)
if ! check_cmd c++; then
    fail "C++ compiler not found. Install Xcode command line tools: xcode-select --install"
fi

# Homebrew
if ! check_cmd brew; then
    fail "Homebrew not found. Install from https://brew.sh"
fi

# lwtools
if ! check_cmd lwasm; then
    info "Installing lwtools via Homebrew..."
    brew install lwtools
fi

LWASM_VER="$(lwasm --version 2>&1 | grep -oE '[0-9]+\.[0-9]+' | head -1)"
info "lwtools version: $LWASM_VER"

# bison: CMOC's grammar uses Bison 3 options (-Wno-conflicts-sr), so the
# bison 2.3 that macOS ships with the Command Line Tools is too old to
# regenerate the parser. Ensure a modern Homebrew bison and put it ahead of
# the system one on PATH (it is keg-only, so not symlinked into /opt/homebrew/bin).
BISON_MAJOR="$(bison --version 2>/dev/null | grep -oE '[0-9]+\.[0-9]+' | head -1 | cut -d. -f1)"
if [ "${BISON_MAJOR:-0}" -lt 3 ]; then
    if ! brew list bison >/dev/null 2>&1; then
        info "Installing GNU Bison 3 via Homebrew..."
        brew install bison
    fi
    export PATH="$(brew --prefix bison)/bin:$PATH"
fi
info "bison version: $(bison --version | head -1 | grep -oE '[0-9]+\.[0-9]+' | head -1)"

if ! check_cmd flex; then
    info "Installing flex via Homebrew..."
    brew install flex
fi

ok "All prerequisites satisfied"

# ---------------------------------------------------------------------------
# Step 2: Build CMOC compiler
# ---------------------------------------------------------------------------

cd "$BUILD_DIR"

# Clean previous build if any
if [ -f Makefile ]; then
    info "Cleaning previous build..."
    make clean >/dev/null 2>&1 || true
fi

info "Configuring CMOC..."
./configure --prefix="$INSTALL_DIR"

info "Building CMOC compiler (using $JOBS cores)..."
make -j"$JOBS"

ok "CMOC compiler + stdlib built successfully"

# ---------------------------------------------------------------------------
# Step 4: Install to target directory
# ---------------------------------------------------------------------------

info "Installing to $INSTALL_DIR ..."
mkdir -p "$INSTALL_DIR"
mkdir -p "$INSTALL_DIR/stdlib"
mkdir -p "$INSTALL_DIR/stdlib/vectrex"
# CMOC compiler binary
cp -f src/cmoc "$INSTALL_DIR/cmoc"

# lwtools binaries (copy from Homebrew so toolchain is self-contained)
BREW_PREFIX="$(brew --prefix)"
for tool in lwasm lwlink lwar lwobjdump; do
    src_path=""
    if [ -f "$BREW_PREFIX/bin/$tool" ]; then
        src_path="$BREW_PREFIX/bin/$tool"
    elif check_cmd "$tool"; then
        src_path="$(which $tool)"
    fi

    if [ -z "$src_path" ]; then
        warn "$tool not found, skipping"
    elif [ "$(realpath "$src_path")" = "$(realpath "$INSTALL_DIR/$tool" 2>/dev/null)" ]; then
        : # already in place
    else
        cp -f "$src_path" "$INSTALL_DIR/$tool"
    fi
done

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

ok "Installation complete!"

# ---------------------------------------------------------------------------
# Step 5: Verify
# ---------------------------------------------------------------------------

info "Verifying installation..."

"$INSTALL_DIR/cmoc" --version
"$INSTALL_DIR/lwasm" --version 2>&1 | head -1

if [ -f "$INSTALL_DIR/stdlib/libcmoc-crt-vec.a" ] && \
   [ -f "$INSTALL_DIR/stdlib/libcmoc-std-vec.a" ] && \
   [ -f "$INSTALL_DIR/stdlib/vectrex.h" ]; then
    ok "Vectrex stdlib verified"
else
    warn "Some stdlib files may be missing — check $INSTALL_DIR/stdlib/"
fi

echo ""
ok "CMOC toolchain installed at: $INSTALL_DIR"
echo ""
echo "  To build the Vectrex game:"
echo "    cd vectrex/ && make"
echo ""
