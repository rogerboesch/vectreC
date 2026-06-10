# VectreC on Windows

Using and building the VectreC toolchain on Windows.
For a general introduction see the main [README](README.md).


## Option 1: Prebuilt package (recommended)

Download `vectrec-win64.zip` from the
[latest release](https://github.com/rogerboesch/vectreC/releases/latest) —
a self-contained toolchain that runs on any 64-bit Windows 10/11 machine.
No installation, no MSYS2, no admin rights:

```powershell
# unzip, then inside the folder:
. .\vectrec-env.ps1
cmoc --vectrex -I $env:VECTREC\stdlib -L $env:VECTREC\stdlib -o hello.bin examples\hello.c
```

The package includes an extensive user guide,
[README-WINDOWS.md](cmoc/doc/README-WINDOWS.md), covering setup, your first
program, compiler options, Vectrex programming (ROM header pragmas, ROM
data, controller, sound), emulators, and troubleshooting.


## Option 2: Build from source (MSYS2)

1. Install [MSYS2](https://www.msys2.org) (or `winget install MSYS2.MSYS2`).
2. Open the **MSYS2 MINGW64** shell from the Start menu.
3. Build and install:

```bash
git clone https://github.com/rogerboesch/vectreC.git
cd vectreC
./build-windows.sh
```

This will:
1. Install missing build tools via pacman (gcc, bison, flex, autotools, ...)
2. Download and build lwtools from source
3. Configure and compile CMOC as a native, statically linked `cmoc.exe`
4. Install everything to `%USERPROFILE%\retro-tools\vectrec\`
5. Verify the installation by compiling a test Vectrex program

The installed toolchain runs from **PowerShell or cmd** (MSYS2 is only needed
to build it). One caveat: `cmoc.exe` invokes the GNU C preprocessor (`cpp`) at
compile time. Either keep `C:\msys64\mingw64\bin` on your `PATH`, or set the
`CMOC_CPP` environment variable to the full path of `cpp.exe`. The generated
`vectrec-env.ps1` does this for you:

```powershell
. $env:USERPROFILE\retro-tools\vectrec\vectrec-env.ps1
cmoc --vectrex -I $env:VECTREC\stdlib -L $env:VECTREC\stdlib -o game.bin game.c
```

### Custom install location

```bash
./build-windows.sh /c/path/to/your/toolchain   # MSYS2 path syntax
```

The default location is `%USERPROFILE%\retro-tools\vectrec\`.

### Prerequisites

Installed automatically by `build-windows.sh` via pacman if missing:

| Tool         | Purpose                        | Package                          |
|--------------|--------------------------------|----------------------------------|
| lwtools      | 6809 assembler/linker (lwasm)  | built from source                |
| bison        | Parser generator               | `bison`                          |
| flex         | Lexer generator                | `flex`                           |
| C++ compiler | Builds the CMOC compiler       | `mingw-w64-x86_64-gcc`           |
| autotools    | Build system                   | `autoconf`, `automake`           |


## Creating the redistributable package

To build your own `vectrec-win64.zip` (cmoc + lwtools + stdlib + bundled
preprocessor + examples + user guide), run after `./build-windows.sh`,
still in the MINGW64 shell:

```bash
./package-windows.sh
```

Users of the zip just unzip, dot-source `vectrec-env.ps1`, and compile —
without installing anything.
