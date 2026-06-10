# VectreC

A ready-to-build CMOC 6809 cross-compiler toolchain for Vectrex game development.

Based on [CMOC](http://sarrazip.com/dev/cmoc.html) by Pierre Sarrazin (v0.1.98) with an
enhanced C-wrapper library for Vectrex BIOS functions. You write C, you get a `.bin`
ROM image that runs in an emulator or on real hardware.

The toolchain consists of:

- **cmoc** — the CMOC C compiler, targeting the Motorola 6809
- **lwtools** — assembler, linker and archiver (lwasm, lwlink, lwar)
- **stdlib** — Vectrex headers and precompiled libraries, including the C wrapper
  for the Vectrex BIOS
- **examples/** — small, ready-to-build sample programs


## Getting Started

Pick your platform:

| Platform    | Instructions                       | Notes                                              |
|-------------|------------------------------------|----------------------------------------------------|
| **macOS**   | [README_macOS.md](README_macOS.md)     | Build from source with `./build.sh` (Homebrew)     |
| **Windows** | [README_Windows.md](README_Windows.md) | Prebuilt download available, or build with MSYS2   |

Once installed, compiling a Vectrex game looks the same everywhere:

```
cmoc --vectrex -I <toolchain>/stdlib -L <toolchain>/stdlib -o game.bin game.c
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
* **lwtools** by William Astle — [Homepage](http://www.lwtools.ca)
* **C-Wrapper** originally by Johan Van den Brande
* **VIDE** by Malban — [GitHub](https://github.com/malbanGit/Vide) (recommended Vectrex IDE)
* Many thanks to the Vectrex community for help, examples, and documentation


## License

See the file [COPYING](COPYING). Note that it does not apply to the files
under `src/usim-0.91-cmoc`.
All changes and additions by Roger Boesch are free to use, change and distribute
without any limitations.
