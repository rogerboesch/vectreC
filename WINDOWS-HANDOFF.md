# Windows build — handoff notes

Dev notes for continuing the CMOC 0.1.98 update on the Windows side. Not a
user doc (that's `cmoc/doc/README-WINDOWS.md`). Delete this file before merging
the branch if you like.

## State as of 2026-06-10 (branch `update-cmoc-0.1.98`)

Two commits done, **verified on macOS only**:
1. `0477830` — re-vendored CMOC **0.1.98** over the old 0.1.67 fork.
2. `8c75c05` — moved the whole vendored tree into **`cmoc/`**; top level now
   holds only vectrec's own files (build scripts, `README*.md`, `examples/`,
   `bench/`).

**Your job:** verify `build-windows.sh` (then `package-windows.sh`) still
produce a working `cmoc.exe` toolchain from the new layout on MSYS2/MINGW64,
and fix anything 0.1.98- or layout-related that breaks. macOS is green; Windows
is unexercised.

## Why this update was easy

CMOC 0.1.98 **upstreamed the Vectrex target** (`--vectrex`, `vx_*` pragmas,
`libcmoc-*-vec.a` all ship in stock CMOC now). So we no longer fork the
compiler. vectrec's only remaining delta:

- **Vectrex stdlib API overlay** (vectrec's snake_case API + CamelCase compat,
  different from upstream's `wait_retrace`/`line`/`move` API):
  `cmoc/src/stdlib/vectrex.h`, `cmoc/src/stdlib/vectrex/{bios.h,compatibility.h,types.h}`,
  `cmoc/src/stdlib/vectrex_bios.c`. Examples use the snake_case names.
- `WINDOWS_BUILD` autoconf conditional (drops the `usim` simulator, which needs
  termios) — `cmoc/configure.ac` + `cmoc/src/Makefile.am`.
- Two extra headers installed via `cmoc/src/stdlib/Makefile.am`.

## Layout change you must know

The build scripts stay at the **top level** as entry points but now `cd` into
`cmoc/` via a `BUILD_DIR="$SCRIPT_DIR/cmoc"` variable before configure/make.
So `./build-windows.sh` is still the command — just know that configure, make,
`src/cmoc.exe`, `src/stdlib/*` all resolve under `cmoc/` now.
`package-windows.sh` reads `cmoc/doc/README-WINDOWS.md`.

## What to actually do

1. Open **MSYS2 MINGW64** shell (the script hard-requires `$MSYSTEM=MINGW64`).
2. `./build-windows.sh` (optionally a custom install dir as `$1`).
   - It auto-installs via pacman: gcc, make, bison, flex, autoconf, automake,
     perl, curl, tar. Builds **lwtools 4.24 from source**. Then in `cmoc/`:
     `./bootstrap` → `./configure --prefix=... --without-writecocofile
     LDFLAGS="-static -static-libgcc -static-libstdc++"` → `make`.
   - It self-tests by compiling a tiny `--vectrex` program and checking for the
     Vectrex ROM header.
3. If it builds, run `./package-windows.sh` → produces `vectrec-win64.zip`
   (bundles cmoc.exe + lwtools + stdlib + a cpp/cc1plus preprocessor + DLLs).
4. Sanity: from the install dir, `cmoc.exe --vectrex -I stdlib -L stdlib -o hello.bin ..\examples\hello.c`
   and confirm `strings hello.bin | grep "g GCE"`.

## Things likely to bite (0.1.98-specific)

- **Bison:** MSYS2 `bison` is 3.x, so you're fine. (On macOS the CLT bison 2.3
  was too old for CMOC's `AM_YFLAGS = -Wno-conflicts-sr -Werror`; `parser.cc`
  and `lexer.cc` are generated + gitignored, so bison/flex must regenerate
  them. Just make sure both are installed — they're in the pacman list.)
- **`vectrex_bios.c` cast:** 0.1.98 builds the libraries with `-Werror`. We
  already added an explicit `(int8_t *)` cast at the `init_music_chk()` call
  (`cmoc/src/stdlib/vectrex_bios.c`). If you see *other* `-Werror` warnings on
  Windows that didn't appear on macOS (signedness, etc.), fix them the same way.
- **`WINDOWS_BUILD`:** confirm configure prints `Windows (MinGW/MSYS) build: yes`
  and that `usim-0.91-cmoc` is NOT in SUBDIRS (it needs termios and won't build
  on MinGW). If `$host_os` doesn't match `mingw*|msys*`, the usim build will be
  attempted and fail.
- **New upstream targets/libs:** 0.1.98 also builds flex/void/thommo libs and
  many new libc functions. Harmless, but it's more `.c`/`.asm` to assemble — if
  some new stdlib source fails to assemble under the Windows lwasm 4.24, that's
  an upstream-vs-toolchain issue, not ours.
- **`cmoc.exe` needs cpp at runtime:** it shells out to the GNU C preprocessor.
  `build-windows.sh` writes a `vectrec-env.ps1` setting `CMOC_CPP` and PATH;
  `package-windows.sh` bundles cpp+cc1plus+DLLs so the zip is standalone.
- Static link flags exist so `cmoc.exe` runs outside MSYS2. If you hit missing
  DLL errors when running outside the shell, check those flags survived.

## If you need to re-vendor CMOC again later

rsync upstream `src` into `cmoc/` (`--delete`, excluding the vectrec-only paths
above + `.claude/`), restore the 5 overlay files, re-apply the 3 patches,
`cd cmoc && ./bootstrap`, build. Upstream tarball: http://sarrazip.com/dev/cmoc-0.1.98.tar.gz
