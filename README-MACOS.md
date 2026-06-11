# VectreC on macOS

**A complete C toolchain for programming the Vectrex.**

VectreC is a ready-to-build distribution of the [CMOC](http://sarrazip.com/dev/cmoc.html)
6809 cross-compiler (v0.1.98) by Pierre Sarrazin, bundled with the
[lwtools](http://www.lwtools.ca) assembler/linker by William Astle and an
enhanced C wrapper library for the Vectrex BIOS. You write C, you get a
`.bin` ROM image that runs in an emulator or on real hardware.

On macOS you build the toolchain once from source with `./build-macos.sh` (it
installs its own prerequisites via Homebrew). Everything then lives in a single
folder you can put on your `PATH`. For a general introduction see the main
[README](README.md).

---

## 1. Requirements

- macOS 12 or later (Apple Silicon or Intel)
- [Xcode Command Line Tools](https://developer.apple.com/) — provides `clang++`
  and the C preprocessor (`xcode-select --install`)
- [Homebrew](https://brew.sh) — used to install the build prerequisites
- A Vectrex emulator to run your games (see [section 8](#8-running-your-game))

## 2. Installation

```bash
git clone https://github.com/rogerboesch/vectreC.git
cd vectreC
./build-macos.sh
```

This will:

1. Check and install prerequisites via Homebrew (if missing).
2. Configure and compile CMOC from source as a native binary.
3. Build the 6809 standard library (all platform targets, including Vectrex).
4. Install binaries, libraries and headers to `~/retro-tools/vectrec/`.
5. Verify the install by compiling a test Vectrex program.

**Prerequisites** (installed automatically by `build-macos.sh` via Homebrew if missing):

| Tool         | Purpose                        | Source                 |
|--------------|--------------------------------|------------------------|
| lwtools      | 6809 assembler/linker (lwasm)  | `brew install lwtools` |
| bison        | Parser generator (3.x)         | `brew install bison`   |
| flex         | Lexer generator                | Xcode CLT / Homebrew   |
| C++ compiler | Builds the CMOC compiler       | Xcode CLT              |

> macOS ships an old bison 2.3 with the Command Line Tools that is too old for
> CMOC's parser. `build-macos.sh` installs a modern Homebrew bison 3.x and uses it
> automatically — you don't need to do anything.

**Custom install location:**

```bash
./build-macos.sh /path/to/your/toolchain
```

The default is `~/retro-tools/vectrec/`.

### Set up your environment

Point a `VECTREC` variable at the install folder and put it on your `PATH` so
`cmoc`, `lwasm` etc. are found directly:

```bash
export VECTREC="$HOME/retro-tools/vectrec"
export PATH="$VECTREC:$PATH"
```

Add those two lines to `~/.zshrc` (or `~/.bash_profile`) to make them permanent.
The rest of this guide assumes `VECTREC` is set and the folder is on `PATH`.

Verify it works:

```bash
cmoc --version          # -> cmoc (cmoc 0.1.98)
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

```bash
cmoc --vectrex -I $VECTREC/stdlib -L $VECTREC/stdlib -o hello.bin hello.c
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
| `-I <dir>`        | Header search path — pass `$VECTREC/stdlib`                     |
| `-L <dir>`        | Library search path — pass `$VECTREC/stdlib`                    |
| `-O0` / `-O1` / `-O2` | Optimization level (default `-O2`)                          |
| `-c`              | Compile to an object file (`.o`) only, do not link              |
| `-D NAME=value`   | Define a preprocessor macro                                     |
| `--intermediate`  | Keep intermediate files (`.s` asm, `.lst` listing, `.map`)      |
| `--intdir=<dir>`  | Put those intermediate files in `<dir>`                         |
| `--verbose`       | Show the preprocessor/assembler/linker commands being run       |
| `--version`       | Show compiler version                                           |

Multi-file projects work the way you'd expect:

```bash
cmoc --vectrex -I $VECTREC/stdlib -L $VECTREC/stdlib -o game.bin main.c player.c enemies.c
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

### Shell script (`compile.sh`)

```bash
#!/bin/bash
export VECTREC="$HOME/retro-tools/vectrec"
cmoc --vectrex -I $VECTREC/stdlib -L $VECTREC/stdlib -o game.bin main.c \
    && echo "OK: game.bin" || exit 1
```

### Makefile

```makefile
VECTREC = $(HOME)/retro-tools/vectrec
CMOC    = $(VECTREC)/cmoc
STDLIB  = $(VECTREC)/stdlib

game.bin: main.c player.c
	$(CMOC) --vectrex -I$(STDLIB) -L$(STDLIB) -o $@ $^
```

## 8. Running your game

Emulators, in rough order of popularity for development:

- **[VIDE](http://vide.malban.de)** by Malban — a full Vectrex IDE (Java, runs
  on macOS) with an excellent emulator and debugger. Load the `.bin` via
  *Vecci → Run*.
- **[ParaJVE](http://www.vectrex.fr/ParaJVE/)** — Java-based, easy to use, good
  compatibility.
- **[vecx](https://github.com/jhawthorn/vecx)** — minimal, open source; builds
  on macOS with SDL.

For real hardware, a flash cartridge such as the **VecMulti** or
**VecFever** lets you copy `.bin` files onto the cartridge.

## 9. What gets installed

```
~/retro-tools/vectrec/
├── cmoc                    CMOC C compiler (native binary)
├── lwasm                   6809 assembler        (lwtools)
├── lwlink                  6809 linker           (lwtools)
├── lwar                    6809 archiver         (lwtools)
├── lwobjdump               object file inspector (lwtools)
└── stdlib/                 headers and precompiled libraries
    ├── vectrex.h               main include (pulls in vectrex/bios.h)
    ├── vectrex/                bios.h, stdlib.h, types.h, compatibility.h
    ├── libcmoc-crt-vec.a       C runtime (startup code, arithmetic helpers)
    └── libcmoc-std-vec.a       standard library
```

The ready-to-build sample programs (`hello.c`, `controller.c`, `romdata.c`,
`pong.c`) stay in the cloned repository under [`examples/`](examples/) — they
are not copied into the install folder.

How a build works internally: `cmoc` runs the system C preprocessor (`cpp`) on
your source, compiles the result to 6809 assembly, assembles it with `lwasm`,
and links it with `lwlink` against `libcmoc-crt-vec.a` / `libcmoc-std-vec.a`
into a raw ROM image.

## 10. Troubleshooting

**`zsh: command not found: cmoc` (or `lwasm`)**
The toolchain folder is not on `PATH`. Run the two `export` lines from
[section 2](#2-installation) (and add them to `~/.zshrc` to make them stick).

**`cmoc: fatal error: preprocessor failed.`**
The C preprocessor (`cpp`) is missing — install the Xcode Command Line Tools
with `xcode-select --install`. (If you keep `cpp` somewhere non-standard, set
`CMOC_CPP` to its full path.)

**`error: undeclared identifier 'wait_retrace'` (or `intensity`, `move`, ...)**
Older CMOC/Vectrex tutorials use older function names. This package's
wrapper follows gcc6809-style names: `wait_recal()`, `intensity_a()`,
`moveto_d()`. Check `$VECTREC/stdlib/vectrex/bios.h` for the authoritative
list; `<vectrex/compatibility.h>` maps the old CamelCase BIOS names.

**`error: invalid pragma directive: const_data start`**
`#pragma const_data` is from older CMOC versions. Declare the data `const`
instead — it is placed in ROM automatically (see [section 6](#6-programming-the-vectrex)).

**My title screen text shows garbage symbols**
The Vectrex font is uppercase-only — use capital letters in `vx_title` and
in all strings you print.

**Everything is invisible / too dim**
Set `intensity_a(0x7f)` after every `wait_recal()`. Note that reading the
joystick can reset the intensity, so set intensity *after* the controller
calls.

## 11. A complete example: building Pong step by step

[`examples/pong.c`](examples/pong.c) is a full two-player Pong game — paddles,
a bouncing ball, scoring, controller input, sound effects and startup music. It
ties together everything in sections 5 and 6, so it's a good end-to-end test of
your setup. Here is the whole build, from a fresh terminal to a running ROM.

**Step 1 — set up the environment** (section 2) and `cd` into the cloned repo:

```bash
export VECTREC="$HOME/retro-tools/vectrec"
export PATH="$VECTREC:$PATH"
cd ~/path/to/vectreC          # the cloned repository
cmoc --version                # -> cmoc (cmoc 0.1.98)
```

**Step 2 — compile `pong.c`.** It is a single source file, so one command
produces the ROM:

```bash
cmoc --vectrex -I $VECTREC/stdlib -L $VECTREC/stdlib -o pong.bin examples/pong.c
```

**Step 3 — read the output.** A successful build prints one **warning** and
nothing else:

```
pong.c:120: warning: `const char *' used as parameter 1 (list) of function
draw_vlc() which is `char *' (not const-correct)
```

This is harmless — `pong.c` passes a `const` vertex array to a BIOS wrapper
that declares a non-`const` pointer; the data isn't modified (same pattern as
section 6). The command exits 0 and `pong.bin` (~4.5 KB) appears in the folder:

```bash
echo $?                       # -> 0
ls -l pong.bin                # -> about 4567 bytes
```

**Step 4 — sanity-check the ROM header (optional).** Every Vectrex ROM starts
with the copyright/title header the BIOS reads at boot. `pong.c` sets it with
the `#pragma vx_*` lines at the top of the file, so the bytes `g GCE 2020`
and the title `g PONG` are embedded near the start of the image:

```bash
head -c 64 pong.bin | LC_ALL=C tr -c '[:print:]' '.'; echo
# -> g GCE 2020....P..g PONG......
```

(`strings pong.bin | head` shows the same text.)

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
| `play_sound()` / `vx_music` audio           | section 6 (sound)     |
| `abs()`, `sprintf()` from `<vectrex/stdlib.h>` | section 6 (headers) |

**To work on your own copy**, just copy the file out and build that instead:

```bash
cp examples/pong.c mypong.c
# edit mypong.c ...
cmoc --vectrex -I $VECTREC/stdlib -L $VECTREC/stdlib -o mypong.bin mypong.c
```

If the build fails before producing `pong.bin`, see [section 10](#10-troubleshooting)
— the most common cause is not having the toolchain on `PATH` in this terminal
(`command not found: cmoc`).

## 12. Rebuilding and updating

To pull a newer version and rebuild:

```bash
cd ~/path/to/vectreC
git pull
./build-macos.sh
```

`build-macos.sh` runs `make clean` first, so a rebuild is always from a clean state.
To wipe and reinstall from scratch, remove `~/retro-tools/vectrec/` first.

The full source lives at <https://github.com/rogerboesch/vectreC>. The vendored
CMOC compiler source is under `cmoc/`; vectrec's own files (build scripts,
examples, docs) are at the top level.

### Creating a redistributable package

To build a self-contained `vectrec-macos-arm64.zip` (the same artifact attached
to the GitHub release — `cmoc` + lwtools + stdlib + examples + this guide), run
after `./build-macos.sh`:

```bash
./package-macos.sh
```

Users of the zip just unzip, `source ./vectrec-env.sh`, and compile. The package
is Apple Silicon (arm64) only and needs the Xcode Command Line Tools for the C
preprocessor; Intel Mac users build from source with `./build-macos.sh`.

## 13. Credits and license

- **CMOC compiler** — Pierre Sarrazin, <http://sarrazip.com/dev/cmoc.html> (GPLv3)
- **lwtools** — William Astle, <http://www.lwtools.ca> (GPLv3)
- **Vectrex C wrapper** — originally by Johan Van den Brande, extended by Roger Boesch
- **VectreC distribution** — Roger Boesch, free to use, change and distribute without limitations

Thanks to the Vectrex community for documentation, examples and inspiration.
