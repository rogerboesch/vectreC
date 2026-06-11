#!/bin/bash
#
# package-macos.sh - Create a self-contained, redistributable zip of the
#                    VectreC toolchain for macOS (Apple Silicon / arm64).
#
# The resulting vectrec-macos-arm64.zip contains the native cmoc binary,
# lwtools, the Vectrex stdlib and the examples, so end users can unzip,
# source vectrec-env.sh and compile Vectrex programs. macOS supplies the C
# preprocessor (cpp) via the Xcode Command Line Tools, so it is not bundled.
#
# Usage (after ./build-macos.sh):
#   ./package-macos.sh [install-dir]
#

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
INSTALL_DIR="${1:-$HOME/retro-tools/vectrec}"
STAGE_NAME="vectrec-macos-arm64"
STAGE_DIR="$SCRIPT_DIR/$STAGE_NAME"
ZIP_FILE="$SCRIPT_DIR/$STAGE_NAME.zip"

info()  { printf "\033[1;34m==>\033[0m %s\n" "$1"; }
ok()    { printf "\033[1;32m==>\033[0m %s\n" "$1"; }
fail()  { printf "\033[1;31m==>\033[0m %s\n" "$1" >&2; exit 1; }

[ "$(uname -s)" = "Darwin" ] || fail "Run this on macOS."
[ -f "$INSTALL_DIR/cmoc" ] || fail "Toolchain not found in $INSTALL_DIR. Run ./build-macos.sh first."

# Sanity: the prebuilt package targets Apple Silicon.
if file "$INSTALL_DIR/cmoc" | grep -q arm64; then
    :
else
    fail "$INSTALL_DIR/cmoc is not an arm64 binary — build on Apple Silicon to make this package."
fi

# ---------------------------------------------------------------------------
# Stage toolchain
# ---------------------------------------------------------------------------

info "Staging toolchain from $INSTALL_DIR ..."
rm -rf "$STAGE_DIR" "$ZIP_FILE"
mkdir -p "$STAGE_DIR"

cp -f "$INSTALL_DIR/cmoc" "$STAGE_DIR/"
for tool in lwasm lwlink lwar lwobjdump; do
    [ -f "$INSTALL_DIR/$tool" ] && cp -f "$INSTALL_DIR/$tool" "$STAGE_DIR/"
done
cp -R "$INSTALL_DIR/stdlib" "$STAGE_DIR/stdlib"

# Ship the examples and the user guide.
mkdir -p "$STAGE_DIR/examples"
cp -f "$SCRIPT_DIR/examples/"*.c "$STAGE_DIR/examples/"
cp -f "$SCRIPT_DIR/README-MACOS.md" "$STAGE_DIR/README-MACOS.md"

# ---------------------------------------------------------------------------
# Environment script
#
# Sourcing it sets VECTREC + PATH and clears the macOS quarantine flag on the
# toolchain folder, so the freshly downloaded (unsigned) binaries run without
# a Gatekeeper prompt.
# ---------------------------------------------------------------------------

cat > "$STAGE_DIR/vectrec-env.sh" <<'EOF'
# VectreC toolchain environment (redistributable package)
# Usage:  source ./vectrec-env.sh      (or:  . ./vectrec-env.sh)
_VECTREC_DIR="$(cd "$(dirname "${BASH_SOURCE[0]:-$0}")" && pwd)"
export VECTREC="$_VECTREC_DIR"
export PATH="$VECTREC:$PATH"
# Clear the quarantine flag macOS adds to downloaded binaries (best effort).
xattr -dr com.apple.quarantine "$VECTREC" 2>/dev/null || true
EOF

# ---------------------------------------------------------------------------
# Zip
# ---------------------------------------------------------------------------

info "Creating $ZIP_FILE ..."
(cd "$SCRIPT_DIR" && zip -q -r "$(basename "$ZIP_FILE")" "$STAGE_NAME")

ok "Package created: $ZIP_FILE"
du -sh "$STAGE_DIR" "$ZIP_FILE" | sed 's/^/    /'
