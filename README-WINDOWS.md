# VectreC for Windows

**A complete C toolchain for programming the Vectrex — no installation required.**

VectreC is a ready-to-use distribution of the [CMOC](http://sarrazip.com/dev/cmoc.html)
6809 cross-compiler (v0.1.98) by Pierre Sarrazin, bundled with the
[lwtools](http://www.lwtools.ca) assembler/linker by William Astle and an
enhanced C wrapper library for the Vectrex BIOS. You write C, you get a
`.bin` ROM image that runs in an emulator or on real hardware.

Everything in this package is self-contained: unzip it anywhere and start
compiling. No MSYS2, no Visual Studio, no admin rights needed.

---

## 1. Requirements

- Windows 10 or 11, 64-bit
- PowerShell (preinstalled) or cmd.exe
- A Vectrex emulator to run your games (see [section 8](#8-running-your-game))

## 2. Installation

1. Unzip `vectrec-win64.zip` to any folder, e.g. `C:\vectrec`.
   The folder can be moved or renamed later — nothing is hardcoded.
2. Open PowerShell in that folder and load the environment:

   ```powershell
   . .\vectrec-env.ps1
   ```

   This sets three things **for the current session only**:

   | Variable    | Meaning                                                        |
   |-------------|----------------------------------------------------------------|
   | `VECTREC`   | Path of the toolchain folder                                   |
   | `CMOC_CPP`  | Path of the bundled C preprocessor (used internally by cmoc)   |
   | `PATH`      | Toolchain folder prepended, so `cmoc`, `lwasm` etc. just work  |

3. *(Optional)* To make this permanent, add the same line to your PowerShell
   profile (`notepad $PROFILE`), giving it the absolute path:

   ```powershell
   . C:\vectrec\vectrec-env.ps1
   ```

**Using cmd.exe instead of PowerShell:**

```bat
set VECTREC=C:\vectrec
set CMOC_CPP=%VECTREC%\cpp\bin\cpp.exe
set PATH=%VECTREC%;%PATH%
```

## 3. Your first program

Create `hello.c`:

```c
#include <vectrex/bios.h>

int main()
{
    while (1)
    {
        wait_recal();               // sync to the 50 Hz frame; do this every frame
        intensity_a(0x7f);          // set beam intensity (0x00..0x7f)
        print_str_c(0x10, -0x50, "HELLO WORLD!");
    }
    return 0;
}
```

Compile it:

```powershell
cmoc --vectrex -I $env:VECTREC\stdlib -L $env:VECTREC\stdlib -o hello.bin hello.c
```

That's it — `hello.bin` is a Vectrex ROM image. Load it into an emulator
(see [section 8](#8-running-your-game)) and you should see HELLO WORLD! on screen.

> **Notes:** the Vectrex character set is **uppercase only** — lowercase codes
> map to symbols. The const-correctness warning about the string literal is
> harmless (the BIOS wrappers take non-`const` `char *`); silence it with a
> `(char *)` cast if you like.

## 4. Compiler usage

```
cmoc --vectrex [options] file.c [file2.c ...]
```

Commonly used options:

| Option            | Effect                                                          |
|-------------------|-----------------------------------------------------------------|
| `--vectrex`       | Target the Vectrex (always required)                            |
| `-o file.bin`     | Output filename (default: first source name with `.bin`)        |
| `-I <dir>`        | Header search path — pass `$env:VECTREC\stdlib`                 |
| `-L <dir>`        | Library search path — pass `$env:VECTREC\stdlib`                |
| `-O0` / `-O1` / `-O2` | Optimization level (default `-O2`)                          |
| `-c`              | Compile to an object file (`.o`) only, do not link              |
| `-D NAME=value`   | Define a preprocessor macro                                     |
| `--intermediate`  | Keep intermediate files (`.s` asm, `.lst` listing, `.map`)      |
| `--intdir=<dir>`  | Put those intermediate files in `<dir>`                         |
| `--verbose`       | Show the preprocessor/assembler/linker commands being run       |
| `--version`       | Show compiler version                                           |

Multi-file projects work the way you'd expect:

```powershell
cmoc --vectrex -I $env:VECTREC\stdlib -L $env:VECTREC\stdlib -o game.bin main.c player.c enemies.c
```

CMOC implements a substantial subset of C (no `float`/`double` on the
Vectrex target, no standard C library beyond what `stdlib` provides). See the
CMOC manual at <http://sarrazip.com/dev/cmoc.html> for the full language
reference.

## 5. The cartridge ROM header

Every Vectrex cartridge starts with a header the BIOS reads at boot — it
defines the copyright line, the startup title screen, and the startup music.
Customize it with pragmas at the top of your main source file:

```c
#pragma vx_copyright "2026"            // copyright string (year)
#pragma vx_title_pos -100, -100        // title position: y, x
#pragma vx_title_size -8, 80           // title size: height, width (note negative height)
#pragma vx_title "g MY GAME"           // title text; 'g' prints the (c) sign; UPPERCASE only
#pragma vx_music vx_music_1            // startup music: vx_music_1 .. vx_music_13
```

All pragmas are optional — without them you get a default header
(`g GCE 2015`).

## 6. Programming the Vectrex

### Coordinates: Y comes first

The Vectrex BIOS convention puts the **y coordinate before x**, and the C
wrappers keep that order: `moveto_d(y, x)`, `dot_d(y, x)`,
`print_str_c(y, x, str)`. The visible area spans roughly -128..127 on both
axes, with (0, 0) in the center of the screen.

### The frame loop

A Vectrex program is one endless loop. Call `wait_recal()` once per
iteration — it synchronizes with the 50 Hz screen refresh and recalibrates
the beam to the center. After it, set intensity and draw:

```c
while (1)
{
    wait_recal();        // frame sync + beam to center
    intensity_a(0x7f);   // brightness 0x00..0x7f (must be set every frame)
    // ... move and draw ...
}
```

Useful primitives from `<vectrex/bios.h>`:

| Function                          | Purpose                                       |
|-----------------------------------|-----------------------------------------------|
| `wait_recal()`                    | Frame sync; start of every frame              |
| `intensity_a(i)`                  | Beam brightness, 0x00–0x7f                    |
| `reset0ref()`                     | Move beam back to screen center               |
| `set_scale(s)`                    | Scale factor for subsequent moves/draws       |
| `moveto_d(y, x)`                  | Move the (invisible) beam                     |
| `draw_line_d(y, x)`               | Draw a line relative to current position      |
| `draw_vl_a(n, list)`              | Draw `n` connected lines from a y,x pair list |
| `dot_d(y, x)` / `dot_list(n, l)`  | Draw dots                                     |
| `print_str_c(y, x, str)`          | Print text (uppercase!)                       |
| `set_text_size(h, w)`             | Text size for subsequent prints               |
| `random()`                        | Pseudo-random `int8_t`                        |

### Put data in ROM with `const`

A cartridge has plenty of ROM but the Vectrex has under 1 KB of usable RAM.
Declare constant tables (vector lists, level data, text) as `const` and the
compiler places them in the read-only `rodata` section, which stays in
cartridge ROM instead of being copied to RAM:

```c
const char box[8] = {        // 4 lines of y,x pairs (relative moves)
     50,   0,
      0,  50,
    -50,   0,
      0, -50,
};
const char rom_text[] = "PRESS ANY BUTTON";

// inside the frame loop:
draw_vl_a(4, (int8_t *)box);
print_str_c(-100, -60, (char *)rom_text);
```

The casts are needed because the BIOS wrappers take non-`const` pointers;
the data is not modified. You can confirm the placement by compiling with
`--intermediate` and looking for `rodata` in the generated `.map` file.

> Older Vectrex/CMOC tutorials use `#pragma const_data start/end` for this —
> that pragma no longer exists in this CMOC version; plain `const` replaces it.

### Reading the controller

The wrapper includes a joystick/button abstraction. Enable the axes you
need once at startup (each enabled axis costs CPU time per frame), then poll
inside the loop:

```c
#include <vectrex/bios.h>

int main()
{
    int8_t y = 0, x = 0;

    controller_enable_1_x();    // joystick 1, x axis
    controller_enable_1_y();    // joystick 1, y axis

    while (1)
    {
        wait_recal();
        intensity_a(0x7f);

        controller_check_joysticks();
        controller_check_buttons();

        if (controller_joystick_1_right()) x += 2;
        if (controller_joystick_1_left())  x -= 2;
        if (controller_joystick_1_up())    y += 2;
        if (controller_joystick_1_down())  y -= 2;

        if (controller_button_1_1_pressed())  { x = 0; y = 0; }

        moveto_d(y, x);
        draw_line_d(20, 0);     // a small vertical line as the "player"
    }
    return 0;
}
```

Button helpers come in two flavors: `..._pressed()` fires once per press,
`..._held()` is true as long as the button is down. Buttons are numbered
`controller_button_<joystick>_<button>`, e.g. `controller_button_1_4_*` is
the rightmost button on controller 1.

### Sound

Play one of the 13 built-in BIOS tunes with
`play_music(vx_music_5)` and keep calling `update_audio()` (or `do_sound()`)
once per frame inside the loop. `explosion_sound()` provides an effect;
`sound_byte(reg, value)` gives raw access to the AY-3-8912 sound chip
registers (see the `tone_*`/`amplitude_*`/`noise` defines in `bios.h`).

### Headers in this package

| Header                  | Contents                                                |
|-------------------------|---------------------------------------------------------|
| `<vectrex/bios.h>`      | BIOS wrappers, controller, music — the main include     |
| `<vectrex/stdlib.h>`    | `memcpy`, `memset`, `strlen`, `abs`, `rand`, ...        |
| `<vectrex/types.h>`     | `int8_t`, `uint8_t`, `int16_t`, `uint16_t`, ...         |
| `<vectrex/compatibility.h>` | Old CamelCase BIOS names (`Moveto_d`, ...) for porting |

## 7. Automating builds

### PowerShell script (`build.ps1`)

```powershell
. C:\vectrec\vectrec-env.ps1
cmoc --vectrex -I $env:VECTREC\stdlib -L $env:VECTREC\stdlib -o game.bin main.c
if ($LASTEXITCODE -eq 0) { Write-Host "OK: game.bin" } else { exit 1 }
```

### Makefile (if you have GNU make installed)

```makefile
VECTREC = C:/vectrec
CMOC    = $(VECTREC)/cmoc.exe
STDLIB  = $(VECTREC)/stdlib

game.bin: main.c player.c
	$(CMOC) --vectrex -I$(STDLIB) -L$(STDLIB) -o $@ $^
```

Forward slashes work fine in all paths passed to cmoc.

## 8. Running your game

Emulators, in rough order of popularity for development:

- **[VIDE](http://vide.malban.de)** by Malban — a full Vectrex IDE with an
  excellent emulator and debugger. Load the `.bin` via *Vecci → Run*.
- **[ParaJVE](http://www.vectrex.fr/ParaJVE/)** — easy to use, good
  compatibility.
- **[vecx](https://github.com/jhawthorn/vecx)** — minimal, open source.

For real hardware, a flash cartridge such as the **VecMulti** or
**VecFever** lets you copy `.bin` files onto the cartridge from Windows.

## 9. Package contents

```
vectrec-win64\
├── cmoc.exe            CMOC C compiler (statically linked)
├── lwasm.exe           6809 assembler        (lwtools 4.24)
├── lwlink.exe          6809 linker           (lwtools 4.24)
├── lwar.exe            6809 archiver         (lwtools 4.24)
├── lwobjdump.exe       object file inspector (lwtools 4.24)
├── stdlib\             headers and precompiled libraries
│   ├── vectrex.h            main include (pulls in vectrex\bios.h)
│   ├── vectrex\             bios.h, stdlib.h, types.h, compatibility.h
│   ├── libcmoc-crt-vec.a    C runtime (startup code, arithmetic helpers)
│   └── libcmoc-std-vec.a    standard library
├── cpp\                bundled GNU C preprocessor (used internally by cmoc)
├── examples\           ready-to-build samples (hello.c, controller.c, romdata.c, pong.c)
├── vectrec-env.ps1     environment setup for PowerShell
└── README-WINDOWS.md   this file
```

The first three programs in `examples\` compile as-is and are the same code
shown in sections 3 and 6 of this guide. `pong.c` is a complete game built
step by step in [section 11](#11-a-complete-example-building-pong-step-by-step).

How a build works internally: `cmoc.exe` runs the bundled preprocessor
(`cpp`) on your source, compiles the result to 6809 assembly, assembles it
with `lwasm`, and links it with `lwlink` against `libcmoc-crt-vec.a` /
`libcmoc-std-vec.a` into a raw ROM image.

## 10. Troubleshooting

**`cmoc: fatal error: preprocessor failed.`**
`CMOC_CPP` is not set in this shell. Run `. .\vectrec-env.ps1` from the
toolchain folder first (or set `CMOC_CPP` to `<toolchain>\cpp\bin\cpp.exe`).

**`cmoc: fatal error: could not start assembler` or `'lwasm' is not recognized ...`**
The toolchain folder is not on `PATH`. Same fix: load `vectrec-env.ps1`.

**`error: undeclared identifier 'wait_retrace'` (or `intensity`, `move`, ...)**
Older CMOC/Vectrex tutorials use older function names. This package's
wrapper follows gcc6809-style names: `wait_recal()`, `intensity_a()`,
`moveto_d()`. Check `stdlib\vectrex\bios.h` for the authoritative list;
`<vectrex/compatibility.h>` maps the old CamelCase BIOS names.

**`error: invalid pragma directive: const_data start`**
`#pragma const_data` is from older CMOC versions. Declare the data `const`
instead — it is placed in ROM automatically (see section 6).

**My title screen text shows garbage symbols**
The Vectrex font is uppercase-only — use capital letters in `vx_title` and
in all strings you print.

**Everything is invisible / too dim**
Set `intensity_a(0x7f)` after every `wait_recal()`. Note that reading the
joystick can reset the intensity, so set intensity *after* the controller
calls.

**Windows SmartScreen / antivirus flags the exes**
The binaries are unsigned, freshly built executables — this is a false
positive. Build them yourself from source if in doubt (see the project
repository).

**Spaces in paths**
Folder paths with spaces work for source/output files and `-I`/`-L`
directories. Avoid spaces in the toolchain location itself if you use the
cmd.exe setup.

## 11. A complete example: building Pong step by step

`examples\pong.c` is a full two-player Pong game — paddles, a bouncing ball,
scoring, controller input, sound effects and startup music. It ties together
everything in sections 5 and 6, so it's a good end-to-end test of your setup.
Here is the whole build, from a fresh PowerShell window to a running ROM.

**Step 1 — open PowerShell in the toolchain folder and load the environment.**
Assuming you unzipped to `C:\vectrec` (section 2):

```powershell
cd C:\vectrec
. .\vectrec-env.ps1
```

Verify the compiler is on the path:

```powershell
cmoc --version          # -> cmoc (cmoc 0.1.98)
```

**Step 2 — compile `pong.c`.** It is a single source file, so one command
produces the ROM. You can keep your sources anywhere; here we build the
bundled copy and write the output to the current folder:

```powershell
cmoc --vectrex -I $env:VECTREC\stdlib -L $env:VECTREC\stdlib -o pong.bin $env:VECTREC\examples\pong.c
```

**Step 3 — read the output.** A successful build prints one **warning** and
nothing else:

```
pong.c:120: warning: `const char *' used as parameter 1 (list) of function
draw_vlc() which is `char *' (not const-correct)
```

This is harmless — `pong.c` passes a `const` vertex array to a BIOS wrapper
that declares a non-`const` pointer; the data isn't modified (same pattern as
section 6). The command exits with code 0 and `pong.bin` (~4.5 KB) appears in
the folder. Confirm it:

```powershell
$LASTEXITCODE          # -> 0
Get-Item pong.bin      # -> Length about 4567 bytes
```

**Step 4 — sanity-check the ROM header (optional).** Every Vectrex ROM starts
with the copyright/title header the BIOS reads at boot. `pong.c` sets it with
the `#pragma vx_*` lines at the top of the file, so the bytes `g GCE 2020`
and the title `g PONG` are embedded near the start of the image:

```powershell
$bytes = [System.IO.File]::ReadAllBytes("$PWD\pong.bin")
-join ($bytes[0..63] | ForEach-Object { if ($_ -ge 32 -and $_ -le 126) {[char]$_} else {'.'} })
# -> g GCE 2020....P..g PONG......
```

**Step 5 — run it.** Load `pong.bin` into a Vectrex emulator (see
[section 8](#8-running-your-game)). Use two controllers (or the emulator's key
mappings) — left stick moves player 1's paddle, right stick player 2's.

**What this example demonstrates**, mapped to the rest of this guide:

| Feature in `pong.c`                         | Covered in            |
|---------------------------------------------|-----------------------|
| `#pragma vx_title` / `vx_music` ROM header  | section 5             |
| `const` vertex tables placed in ROM         | section 6 ("`const`") |
| `moveto_d(y, x)` + `draw_vlc()` drawing      | section 6 (Y-first)   |
| `controller_*` joystick polling             | section 6 (controller)|
| `play_sound()` / `vx_music` audio            | section 6 (sound)     |
| `abs()`, `sprintf()` from `<vectrex/stdlib.h>` | section 6 (headers) |

**To work on your own copy**, just copy the file out and build that instead:

```powershell
Copy-Item $env:VECTREC\examples\pong.c .\mypong.c
# edit mypong.c ...
cmoc --vectrex -I $env:VECTREC\stdlib -L $env:VECTREC\stdlib -o mypong.bin mypong.c
```

If the build fails before producing `pong.bin`, see [section 10](#10-troubleshooting)
— the two most common causes are not having run `vectrec-env.ps1` in this
window (`preprocessor failed` / `lwasm not recognized`).

## 12. Building this package from source

Everything above uses the prebuilt `vectrec-win64.zip`, which is the
recommended path and needs no toolchain. If you'd rather build `cmoc.exe`
yourself, you can — you only need **MSYS2** to build it; the result still runs
from plain PowerShell/cmd afterwards. The full source lives at
<https://github.com/rogerboesch/vectreC>.

### Build with MSYS2

1. Install [MSYS2](https://www.msys2.org) (or `winget install MSYS2.MSYS2`).
2. Open the **MSYS2 MINGW64** shell from the Start menu (not the plain MSYS or
   UCRT64 shell — the script requires `MSYSTEM=MINGW64`).
3. Clone and build:

   ```bash
   git clone https://github.com/rogerboesch/vectreC.git
   cd vectreC
   ./build-windows.sh
   ```

This will:

1. Install missing build tools via pacman (gcc, bison, flex, autotools, ...).
2. Download and build lwtools from source.
3. Configure and compile CMOC as a native, statically linked `cmoc.exe`.
4. Install everything to `%USERPROFILE%\retro-tools\vectrec\`.
5. Verify the install by compiling a test Vectrex program.

The installed toolchain runs from **PowerShell or cmd** — MSYS2 is only needed
to build it. As with the prebuilt package, `cmoc.exe` invokes the GNU C
preprocessor at compile time, so either keep `C:\msys64\mingw64\bin` on your
`PATH` or set `CMOC_CPP` to the full path of `cpp.exe`. The generated
`vectrec-env.ps1` does this for you:

```powershell
. $env:USERPROFILE\retro-tools\vectrec\vectrec-env.ps1
cmoc --vectrex -I $env:VECTREC\stdlib -L $env:VECTREC\stdlib -o game.bin game.c
```

**Custom install location:**

```bash
./build-windows.sh /c/path/to/your/toolchain   # MSYS2 path syntax
```

The default is `%USERPROFILE%\retro-tools\vectrec\`.

**Prerequisites** (installed automatically by `build-windows.sh` via pacman if
missing):

| Tool         | Purpose                        | Package                |
|--------------|--------------------------------|------------------------|
| lwtools      | 6809 assembler/linker (lwasm)  | built from source      |
| bison        | Parser generator               | `bison`                |
| flex         | Lexer generator                | `flex`                 |
| C++ compiler | Builds the CMOC compiler       | `mingw-w64-x86_64-gcc` |
| autotools    | Build system                   | `autoconf`, `automake` |

### Creating the redistributable package

To build your own `vectrec-win64.zip` (cmoc + lwtools + stdlib + bundled
preprocessor + examples + this guide), run after `./build-windows.sh`, still in
the MINGW64 shell:

```bash
./package-windows.sh
```

Users of the zip just unzip, dot-source `vectrec-env.ps1`, and compile —
without installing anything.

## 13. Credits and license

- **CMOC compiler** — Pierre Sarrazin, <http://sarrazip.com/dev/cmoc.html> (GPLv3)
- **lwtools** — William Astle, <http://www.lwtools.ca> (GPLv3)
- **Vectrex C wrapper** — originally by Johan Van den Brande, extended by Roger Boesch
- **VectreC distribution** — Roger Boesch, free to use, change and distribute without limitations

Thanks to the Vectrex community for documentation, examples and inspiration.
