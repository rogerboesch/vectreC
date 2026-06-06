# VectreC on macOS

Building and installing the VectreC toolchain on macOS.
For a general introduction see the main [README](README.md).


## Quick Start

```bash
git clone https://github.com/rogerboesch/vectreC.git
cd vectreC
./build.sh
```

This will:
1. Check and install prerequisites via Homebrew (if missing)
2. Configure and compile CMOC from source
3. Build the 6809 standard library (all platform targets)
4. Install binaries, libraries, and headers to `~/retro-tools/vectrec/`
5. Verify the installation


## Prerequisites

Installed automatically by `build.sh` if missing:

| Tool         | Purpose                        | Source                 |
|--------------|--------------------------------|------------------------|
| lwtools      | 6809 assembler/linker (lwasm)  | `brew install lwtools` |
| bison        | Parser generator               | Xcode CLT / Homebrew   |
| flex         | Lexer generator                | Xcode CLT / Homebrew   |
| C++ compiler | Builds the CMOC compiler       | Xcode CLT              |


## Custom Install Location

```bash
./build.sh /path/to/your/toolchain
```

The default location is `~/retro-tools/vectrec/`.


## What Gets Installed

```
~/retro-tools/vectrec/
├── cmoc                    # CMOC compiler binary
├── lwasm                   # 6809 assembler
├── lwlink                  # 6809 linker
├── lwar                    # 6809 archiver
├── lwobjdump               # Object file inspector
├── stdlib/                 # Headers + precompiled libraries
│   ├── vectrex.h           # Main Vectrex include
│   ├── vectrex/            # Vectrex-specific headers (bios.h, etc.)
│   ├── libcmoc-crt-vec.a   # Vectrex C runtime
│   └── libcmoc-std-vec.a   # Vectrex standard library
```


## Usage Example

Once installed, compile a Vectrex program with:

```bash
VECTREC=$HOME/retro-tools/vectrec
$VECTREC/cmoc -I$VECTREC/stdlib -L$VECTREC/stdlib --vectrex -o game.bin game.c
```

Or use a Makefile:

```makefile
VECTREC = $(HOME)/retro-tools/vectrec
CMOC    = $(VECTREC)/cmoc
STDLIB  = $(VECTREC)/stdlib

all:
	$(CMOC) -I$(STDLIB) -L$(STDLIB) --vectrex -o game.bin game.c
```

Ready-to-build sample programs are in [examples/](examples/).
