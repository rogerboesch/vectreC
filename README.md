# VectreC

A ready-to-build CMOC 6809 cross-compiler toolchain for Vectrex game development on macOS.

Based on [CMOC](http://sarrazip.com/dev/cmoc.html) by Pierre Sarrazin (v0.1.67) with an
enhanced C-wrapper library for Vectrex BIOS functions.


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


## C-Wrapper for Vectrex

The initial version of the C-Wrapper library was written by Johan Van den Brande.
This version contains a subset of the most important BIOS calls with naming
conventions compatible with [gcc6809](https://github.com/jmatzen/gcc6809) to
simplify porting code between toolchains.

### Project Goals

* Provide a full working C wrapper for Vectrex BIOS in CMOC
* Use similar names as gcc6809 to simplify porting
* Include joystick abstraction (controller.h functionality) directly in the wrapper

Progress: [Tracking spreadsheet](https://docs.google.com/spreadsheets/d/1cExHWU5yljcQpqY_yl2xyezuNjqsNToQXfamVc6qApY/edit?usp=sharing)


## Credits

* **CMOC compiler** by Pierre Sarrazin — [Homepage](http://sarrazip.com/dev/cmoc.html)
* **C-Wrapper** originally by Johan Van den Brande
* **VIDE** by Malban — [GitHub](https://github.com/malbanGit/Vide) (recommended Vectrex IDE)
* Many thanks to the Vectrex community for help, examples, and documentation


## License

See the file [COPYING](COPYING). Note that it does not apply to the files
under `src/usim-0.91-cmoc`.
All changes and additions by Roger Boesch are free to use, change and distribute
without any limitations.
