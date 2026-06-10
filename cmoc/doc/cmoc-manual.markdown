<head>
<title>The CMOC C-like 6809-targeting cross-compiler</title>
<style>
/* Indent code snippets. */
pre { margin-left: 2em }
</style>
</head>
<!-- To use a specific font for this document, insert "font-family: Times New Roman; ",
     for example, inside the style attribute of the <body> element.
-->
<body style="margin-left: 50px; margin-right: 50px; margin-top: 30px; margin-bottom: 30px;">

The CMOC C-like 6809-targeting cross-compiler
=============================================

**By Pierre Sarrazin (sarrazip@sarrazip.com)**

Date of this manual: 2026-03-05

Copyright &copy; 2003-2026

<http://sarrazip.com/dev/cmoc.html>

Distributed under the
**[GNU General Public License](http://www.gnu.org/licenses/gpl-3.0.en.html)**,
**version 3 or later** (see the License section).

Version of CMOC covered by this manual: **0.1.98**


Introduction
------------

CMOC is a [Unix](https://en.wikipedia.org/wiki/Unix)-based program that
compiles a [C](https://en.wikipedia.org/wiki/C_%28programming_language%29)-like
language, generates [Motorola 6809](https://en.wikipedia.org/wiki/Motorola_6809)
assembly language programs and assembles them into executables for the
[Color Computer](https://en.wikipedia.org/wiki/TRS-80_Color_Computer)'s
Disk Basic environment.
It can also target
the [Dragon](https://en.wikipedia.org/wiki/Dragon_32/64) computer,
the [Vectrex](https://en.wikipedia.org/wiki/Vectrex) video game console,
the [Thomson](https://en.wikipedia.org/wiki/Technicolor_SA)
[MO](https://en.wikipedia.org/wiki/Thomson_MO5) and
[TO](https://en.wikipedia.org/wiki/Thomson_TO7) computers,
and
the [OS-9](https://en.wikipedia.org/wiki/OS-9) and
[FLEX](https://en.wikipedia.org/wiki/FLEX%20%28operating_system%29) operating systems.

The efficiency of the generated [machine code](https://en.wikipedia.org/wiki/Machine_code)
is modest, but that code will be faster than the equivalent interpreted
[Color Basic](https://en.wikipedia.org/wiki/Color_BASIC) program.
This was the initial goal of the CMOC project.

CMOC itself is written in [C++](https://en.wikipedia.org/wiki/C%2B%2B)
for a Unix-like system.  The source code is somewhat complicated
because of the anarchic development process that gave rise to CMOC
(one of the classes is named
<A HREF="https://en.wiktionary.org/wiki/contraption">Contraption</A>).
Years ago, development was able to take off because having a working
product had higher priority than having a clean but theoretical design.


<!-- --name="c_features" -->
C Language Features
-------------------

### Unsupported C features

- `float` and `double` arithmetic for a target other than the CoCo Disk Basic
  and DragonDOS environments.
  (CMOC generates calls to the floating point routines of those Basic interpreters.)

- Double-precision [floating-point](https://en.wikipedia.org/wiki/Floating_point)
  arithmetic (the `double` keyword is accepted but is an alias for `float`).

- 64-bit arithmetic (no `long long` type).

- The `volatile` keyword. (It is accepted but ignored,
  and using it causes a warning to be issued.)

- Bit fields. The _TYPE NAME : BITS_ notation is actually accepted by the compiler,
  since version 0.1.52, but the field is allocated with the given _TYPE_,
  regardless of the number of bits specified after the colon.

- Typedefs local to a function (global typedefs are supported).

- Redefining a typedef name, even with an identical definition (e.g.,
  `typedef int INT; typedef int INT;`).

- Structs local to a function (global structs are supported).

- Indirection of a pointer to a *struct* used as an r-value (e.g., `*ptr` alone).
  The l-value case is supported, e.g., `(*ptr).field`.

- The `register` keyword is accepted but ignored.

- A `continue` statement in a switch() body.

- An expression of type float or double as an argument of a switch().

### Supported C and C99 features

- Single-precision [floating-point](https://en.wikipedia.org/wiki/Floating_point)
  arithmetic under the CoCo Disk Basic and (since version 0.1.79) DragonDOS
  environments (the `double` keyword is accepted but is an alias for `float`).

- 8-, 16- and 32-bit arithmetic. Type `char` is 8 bits, `short` and `int`
  are 16 bits and `long` is 32 bits. Each of these types can be `signed`
  or `unsigned`.

- Pointers, including pointers to pointers, etc.

- Structs, including a struct in a struct. Struct initializers.
  A struct must be declared at the global level.
  A reference to an undefined struct is accepted if the declaration
  or statement only refers to a pointer to such a struct.

- Anonymous structs.

- Assigning a struct value to another (e.g., `struct S a, b; a = b;`).
  Initializing a struct from another one is also supported (e.g.,
  `struct S a; struct S b = a;`).

- Passing a struct or union by value to a function (since version 0.1.40).
  (If the size of the struct or union is 1 byte, then it is passed as a
  16-bit word whose least significant byte is the struct or union.)

- Returning a struct or union by value from a function (since version 0.1.40).

- Declaring a variable after the function's code
  has started, as in [C99](https://en.wikipedia.org/wiki/C99).

- while, do, for, switch, continue, break. The switch statement supports
  long integers in the switch expression and in case values, as of CMOC 0.1.89.

- Declaring a for() loop's control variable in the for() itself as in C99,
  e.g., for (int i = 0; ...) {}.

- Declaring more than one variable on the same line, e.g., int a = 0, b = 1;

- [Variadic functions](https://en.wikipedia.org/wiki/Variadic_function),
  e.g., void foo(char \*format, ...). There must be at least one named
  argument before the ellipsis (...), as in
  [Standard C](https://en.wikipedia.org/wiki/ANSI_C).

- [K&R](https://en.wikipedia.org/wiki/C_%28programming_language%29#K.26R_C)
  function definitions, e.g., f() int a; { ... }

- Ending an initializer list with a comma.

- Use of the [C preprocessor](https://en.wikipedia.org/wiki/C_preprocessor)
  (the system's cpp is invoked): #include, #define, #ifdef, etc.

- Unions.

- Enumerations.

- [Type-safe](https://en.wikipedia.org/wiki/Type_safety)
  [function pointers](https://en.wikipedia.org/wiki/Function_pointer).

- The `static` and `extern` keywords. However, an extern declaration must be
  at the global level, not inside a function. Local static variables are
  supported since version 0.1.76. As in C, the initializer, if any, must be constant.

- The `const` keyword. (Const-correctness issues are diagnosed as warnings,
  not as errors, to avoid breaking code written before version 0.1.50.)

- Comma expressions (e.g., x = 1, y = 2, z = 3;).

- [Binary](https://en.wikipedia.org/wiki/Binary_code) literals
  (e.g., `0b101010` for 42). Note that this feature is not part of
  Standard C.

- Function-local [function prototypes](https://en.wikipedia.org/wiki/Function_prototype),
  since version 0.1.85.

- Arrays of function pointers (and typedefs thereof).

- Zero-element arrays.

- [Goto](https://en.wikipedia.org/wiki/Goto) and non-case labeled statements.

- Implementing [Duff's device](https://en.wikipedia.org/wiki/Duff%27s_device)
  in a switch(), since CMOC 0.1.97.

- Modular compilation and linking (see the [section on this](#Modular_compilation_and_linking)).

Installing CMOC
---------------

The following instructions on compiling the source code assume a Unix-like system.

### Requisites

* A C++ compiler, like GNU C++ (g++)
* A C preprocessor (named "cpp"), like the GNU C preprocessor (also needed at run time)
* GNU Make (build tool) (BSD Make lacks some needed features)
* GNU Flex (lexical analyzer generator)
* GNU Bison (parser generator)
* LWTOOLS (6809 assembler, also needed at run time)

### The compiler

Generic installation instructions are available in the INSTALL file.
The short version is to give the following commands,
where _X.Y.Z_ is the CMOC version number (e.g., 0.1.72):

<blockquote><pre>
tar -xzf cmoc-X.Y.Z.tar.gz
cd cmoc-X.Y.Z
./configure --prefix=/some/installation/dir
make
make install
</pre></blockquote>

On **FreeBSD** (and possibly other BSD systems), the `gmake` command should be
given instead of `make`. This ensures that GNU Make is used.

The argument to --prefix is often something like /usr/local or /usr/local/cmoc-X.Y.Z.

The sudo prefix is typically needed to do a `make install`
to a system directory like /usr/local.

The compiler executable is called "cmoc".  After installation, the
directory where it was installed must be in the shell's search path to
allow the shell to run it when typing _cmoc_.

The `make check` command runs several self tests.

To generate the HTML documentation (this document), do `make html`, which
will create the file `doc/cmoc-manual.html`.


Running CMOC
------------

### Compiling a C program

The following must be in the shell's search path:

- The _cmoc_ executable.

- A C preprocessor callable by the name "cpp".

- The [LWTOOLS](http://lwtools.projects.l-w.ca/) lwasm assembler,
  and lwlink linker. If libraries are to be created, then the
  lwar archiver must also be in the search path.

To compile a program written in the file foo.c, run this command:

> cmoc foo.c

By default, the resulting .bin file will load at address $2800 on a CoCo
(see the `#pragma org` directive elsewhere in this manual).
The generated machine language is **position-independent**, so the .bin file
can be loaded with an offset, by passing a second argument to Disk Basic's
LOADM command.

Pass `--help` to cmoc to see the available options.

CMOC comes with a `writecocofile` command that can copy a .bin file
to a 35-track Disk Basic diskette image file. For example:

> writecocofile testing.dsk foo.bin

Pass `--help` to writecocofile for the available options.
The `-r` option reads a file from the image and the `-d` option lists
the files in the image.

For more information on running a CoCo emulator on a GNU/Linux system, see
<http://sarrazip.com/emul-coco.html>.

### Disabling some optimizations

By default, all peephole optimizations are enabled.

The [peephole optimizer](https://en.wikipedia.org/wiki/Peephole_optimization)
can be disabled by passing the `-O0` option to the compiler.
This can be useful when one suspects that the compiler may have generated
invalid code.

Several peephole optimizations were contributed by Jamie Cho in early 2016
for version 0.1.20. They can be disabled by passing `-O1`.

Option `-O2` is equivalent to using the default (full) optimization level.

### Generated files

By default, compiling a C file gives a **.o** object file when option -c is passed,
or a .bin executable if -c is not passed and the C file is a complete program.

When the --intermediate (or -i) option is passed, the following intermediate
files are also generated:

- **.s**: The assembler file written by CMOC.

- **.lst**: The listing generated by the 6809 assembler (lwasm) from a .s file.

When linking, these intermediate files are also generated:

- **.map**: The linking map generated by lwlink. It lists the sections
  and symbols that appear in the final executable.

- **.link**: The linking script for lwlink. It lists the sections that
  must be gathered by the linker to produce the final executable.

Sine CMOC 0.1.62, the `--intdir=`_D_ option can be used to have these
intermediate files generated in directory _D_.

The executable in Disk Basic **.bin** format can be
transferred to a CoCo or emulator and loaded with the LOADM command.
(If compiling for OS-9&mdash;with the `--os9` command-line switch&mdash;the
executable has the name of the C file without the extension.)

When distributing the .bin file to CoCo users, it is not
necessary or useful to accompany it with the other generated files.

### The Motorola S-record (SREC) format

Pass the `--srec` option to cmoc to make
it generate an executable in
[Motorola S-record format](https://en.wikipedia.org/wiki/SREC_%28file_format%29).
The executable file will have the .srec extension.

Since version 0.1.43 of the compiler, this format is the default one
when targeting the USim 6809 simulator (with the `--usim` option).

### The Vectrex video game console

The doc subdirectory of the source distribution contains
cmoc-vectrex.markdown, which gives Vectrex-specific instructions.
Giving the `make html` command will generate cmoc-vectrex.html,
which can be viewed in a browser.

Note that questions regarding Vectrex-specific issues should be
addressed to **Johan Van den Brande** at `johan@vandenbrande.com`.

### The Dragon computer

The [Dragon](https://en.wikipedia.org/wiki/Dragon_32/64)
was a clone of the Color Computer.
It ran a Basic interpreter and a disk operating system that were
simitar to the CoCo's.
The .bin file format was different however.

CMOC will generate a Dragon .bin file when it is given the
`--dragon` command-line switch.

If a Dragon program is separated in several C files, each of these
files must be compiled with the `--dragon` switch, and the linking
invocation must also be given `--dragon`.

As of CMOC 0.1.79, floating point types are supported when
targeting the DragonDOS environment.

The `<disk.h>` library is also not usable on the Dragon.


<a name="Modular_compilation_and_linking" />
Modular compilation and linking
-------------------------------

Since version 0.1.43, CMOC has supported modular (separate) compilation
as well as linking several modules (object files) together.

In other words, the user can now separate a large program into several
.c files, compile each of these files to a .o object file, then have
CMOC link them together into a final executable.

LWTOOLS' lwasm and lwlink commands must be in the search path, otherwise,
the user can specify the `--lwasm=` and `--lwlink=` command-line options
to specify the full path of these commands.


### Target specification

When a program is separated in multiple C files, each of these files
must be compiled with the same target command-line switch (e.g.,
`--coco`, `--dragon`, etc.).
The linking invocation
&mdash; the one that generates the final executable &mdash; must also be given the
same switch.
This ensures that all parts of the program are compiled consistently,
i.e., they all assume the same underlying environment.


### Creating libraries

Object files (.o) can be grouped into a library by using LWTOOLS'
lwar command:

    lwar -c libstuff.a foo.o bar.o baz.o

Library filenames should have the `lib` prefix and be given the `.a`
extension, so that they will be compatible with the `-l` option that
is used to specify a library to link with, e.g.:

    cmoc -o prog.bin prog.c -lstuff

In this case, only the .o files of libstuff.a that are actually needed
will be copied to the executable.

It is possible to specify `libstuff.a` on the CMOC command line, but
then all of the library's object files get copied to the executable.

The `-l` option must be specified after the source or object file(s).

The `-L` option can be used to specify a directory in which to search
for libraries specified by `-l`. The `-L` option can be specified
before or after the source or object file(s).

An object file is indivisible as far as the linker is concerned,
and therefore, so is a C file. This means that either all of the
functions and globals in a C file will end up in the executable, or
none of them will.  When designing a library, it may be desirable to
put each function in its own C file, so that only the functions used
by the parent program will be copied into the executable.


### User library constructors and destructors

#### Defining the constructor and/or destructor

A library that does not come with CMOC might need to run some code
before and/or after the main() function. Some of the uses for this include
initializing some global variables or releasing some resources
when the program quits.

To have code executed before main(), the library author must create
a .asm file that defines a `constructors` section. For example:

        IMPORT  _prepare_stuff
        SECTION constructors
        lbsr    _prepare_stuff
        ENDSECTION

(Each line in the previous file must be indented.)

The code above calls a C function called prepare\_stuff()
(without an initial underscore). Some .c file in the library must
define that function, which must execute whatever initialization
is needed by the library.

In the example above, the LBSR instruction is used instead of JSR
to maintain the relocatability of the library.

To have code executed after main(), the same .asm file can be used
to create a `destructors` section.

        IMPORT  _destroy_stuff
        SECTION destructors
        lbsr    _destroy_stuff
        ENDSECTION

Here, a C function called destroy\_stuff() is called.

The code must NOT end with an RTS or PULS PC instruction, because
all the _constructors_ and _destructors_ sections get concatenated
in the executable, and an RTS gets generated after.

The .asm file may define only one of these two sections.
It does not have to define both.

#### Compiling the library

If the .asm file is called prepostmain.asm for example, then it must
be assembled into an object file with a command like the following:

    lwasm -fobj -o prepostmain.o prepostmain.asm

This object file must _not_ be included in the .a file that comprises
the other object files of the library.
(A .a file can be created with a `lwar -c` command.
See the "Creating libraries" section elsewhere in this manual.)

#### Installing the library

When the library is installed somewhere, the .o file that contains
the constructor and destructor (e.g., prepostmain.o) must be installed
along with the library's .a file.

#### Using the library

Once the library has been installed in a directory, a program that uses
this library must add some arguments to the CMOC invocation that links the program.
If for example the library is named "toolkit" and it was installed in /usr/local/tools,
then this directory contains both prepostmain.o and libtoolkit.a,
and the following arguments must be passed to CMOC when linking the program:

    /usr/local/tools/prepostmain.o -L /usr/local/tools -ltoolkit

The prepostmain.o file must be specified explicitly to force the linker
to include all of its contents in the executable. If this .o file were only
included in the .a file, then it would not be used by the linker because
no code refers explicitly to the contents of prepostmain.asm.


### Specifying code and data addresses

In a modular program, the address at which code and data must be
positioned must _not_ be specify with the `#pragma org`
and `#pragma data` directives. Those addresses must be specified
with the `--org` and `--data` command-line options when invoking
CMOC to perform linking:

<blockquote><pre>
cmoc -c mod1.c
cmoc -c mod2.asm
cmoc -o prog.bin --org=C000 --data=3000 mod1.o mod2.o
</pre></blockquote>

The same goes for `#pragma limit`: the `--limit` option must be
used instead.

Since version 0.1.43, these pragmas can only be used when
compiling directly from a C file to an executable,
i.e., the `-c` option is not used.

#### Default code addresses

* Coco Disk Basic (default): $2800
* Track 34 for CoCo Disk Basic floppy: $2602 (preceded by "OS")
* OS-9: $000D
* Thomson MO/TO: $2800
* Vectrex: $0000
* FLEX: $0000


### Assembly language modules

An object file typically comes from a C file, but an assembly language
file can also be passed to CMOC, which will invoke the assembler on it.
That file's extension must be `.s` or `.asm`.

There are conventions to be observed however:

* Most of the code must be in an assembler section named `code`:

<blockquote><pre>
        SECTION code  
_func   LDD     #42  
        RTS  
        ENDSECTION  
</pre></blockquote>

* The exception is code that initializes global variables.
That code must be in a section called initgl.
Such a section must **not** end with an RTS instruction:

<blockquote><pre>
        SECTION initgl
        LDD     #1000
        STD     _someGlobal,PCR
        ENDSECTION
</pre></blockquote>

* Functions and global variables that are to be made available to
  other modules must be exported with an EXPORT directive,
  e.g., `_func EXPORT`.

* Functions and global variables that are expected to be provided
  by _other_ modules must be imported with an IMPORT directive,
  e.g., `_printf IMPORT`.

* The code should preferably be position-independent, but that is
  not a requirement if the executable will always be loaded at the
  address it was compiled for.

* See the *Calling convention* section elsewhere in this manual
for the rules to observe.

* Read-only global variables and values must be in a section named
`rodata`. This includes string literals.
String literals must end with a null byte.
If the `\n` sequence is used in the literal, it must be encoded
as byte 10 ($0A).

* Writable global variables that have static initializers (typically
arrays) must be in a section named `rwdata`.

* Writable global variables that do not have initializers,
or that are initialized by executing code in an `initgl` section,
must be in a section named `bss`. This section must only give `RMB`
directives, and no `FCC`, `FDB` or `FCB` directives. The `bss`
section follows that rule so that it does not take any space in
the executable file (at least with the Disk Basic BIN format).

* Function names and global variable names must start with
an underscore. In other words, if the C name is `foo`, then the
assembly language name must be `_foo`.


### Merging a binary file with the executable

To include an arbitrary file in the final executable, LWASM's
`INCLUDEBIN` directive can be used.

As an example, assume the binary file is called `blob.dat` on the
filesystem of the computer the program is being developed on.
Create a `blob.asm` file that contains this:

            SECTION rodata
            EXPORT _blob
            EXPORT _blob_end
    _blob
            INCLUDEBIN blob.dat
    _blob_end
            ENDSECTION

Specifying `rodata` will put the data in the read-only section
of the program. To put it in the read-write section, specify
`rwdata` instead.

Prefixing the symbols with an underscore is necessary to
comply with CMOC's naming convention. This avoids conflicts with
symbols generated by CMOC.

Invoke LWASM to have it generate an object file:

    lwasm -fobj --output=blob.o blob.asm

In the C program, access the data this way:

        unsigned char *start, *end;
        asm
        {
    _blob     IMPORT  
    _blob_end IMPORT  
            leax    _blob,pcr
            stx     :start
            leax    _blob_end,pcr
            stx     :end
        }

After this code, the `start` and `end` pointers delimit the data
as loaded in memory on the target machine. (Note that there is no
way to force the data to be loaded at a specific address. The position
of the data is decided by the linker, the origin address set by `--org`
and the offset passed to `LOADM` on a CoCo.)

When invoking CMOC to perform the linking phase, add `blob.o` to
the command line.

See the [LWTOOLS](http://lwtools.projects.l-w.ca/) site for details
on assembler directives.


### Importing symbols used by inline assembly

If an inline assembly block uses a global variable that is provided
by another module, an import directive must be included:

<blockquote><pre>
void f()
{
    asm
    {
_foo    IMPORT
        ldd     #42
        std     _foo
    }
}
</pre></blockquote>


### Generating Prerequisites Automatically

A multi-module project typically uses a makefile to manage the build
process.

CMOC can automatically generate a dependencies file, with a `.d`
extension, that lists the source files that were encountered during
the compilation of a C file. The dependency file will be compatible
with GNU Make.

This is done by passing either the `--deps-only` or `--deps` option
to the compiler. A `.d` file will be generated with a makefile rule
like this:

<blockquote><pre>
main.o main.d : main.c foo.h bar.h baz.h
</pre></blockquote>

Option `--deps-only` stops right after generating this dependency file.
Option `--deps` can be used with `-c` to have the compiler both compile
the C file and generate the dependency file.

The [Generating Prerequisites Automatically](https://www.gnu.org/software/make/manual/html_node/Automatic-Prerequisites.html)
section of the [GNU Make](https://www.gnu.org/software/make/manual/)
manual should be consulted for details on how to write a makefile that
uses this mechanism. GNU Make's `-M` is similar to `--deps-only`, while
`-MMD` is similar to `--deps`.  Note that the `sed` command mentioned in
that manual is not necessary with CMOC, which automatically includes the
`.d` filename in the generated rule.

The name of the `.d` file is formed from the name of the `.o` file to
be generated.

This option has no effect
when also specifying the `-E` option (which prints the preprocessor
output and stops).


Programming for CMOC
--------------------

### Binary operations on bytes

Binary operations on bytes give a byte under CMOC, whereas they give
an integer under Standard C. To get a warning for such operations,
pass the `-Wgives-byte` command-line option. This can be useful when
porting an existing C program to CMOC.

This warning is not given if either side of the operation has a cast,
e.g., `(char) x + (char) y`, or `x + (char) y`.


### Signedness of integers

The compiler generally produces more efficient code for unsigned
arithmetic. Therefore, types `unsigned` and `unsigned char` should
be preferred to `int` and `char` where possible.

CMOC considers that the signedness of an additive or multiplicative
operation is the signedness of the left operand.

CMOC can issue a warning when comparison operators `<`, `<=`, `>` or `>=`
are used with operands of different signedness. This is obtained by
passing the `-Wsign-compare` command-line option.


### Pre-increment vs. post-increment

The pre-increment (++i) typically generates one instruction less
than the post-increment (i++). This is because the post-increment
must yield the *initial* value of the variable. When this initial
value is not needed, it is advisable to use the pre-increment.

The same situation holds for the decrement operators.

### Origin address

To specify the origin address of the program, use the --org option
when invoking CMOC to link your executable.  For example:

> cmoc --org=0x5C00 prog.bin prog.o foo.o bar.o

By default, the origin address is 0x2800. (Under Disk Basic, the
Basic program normally starts at 0x2601, because four PMODE pages
are reserved by default.)

### End address and length of the executable

To determine the effective start and end address of the executable
(assuming no relocation by LOADM), one can look up the `program_start`
and `program_end` symbols in the .lst listing file generated by the
assembler.

`program_end` is useful to check that the executable fits in the
available RAM. On a 32k CoCo, the RAM ends at $8000. By default,
Color Basic reserves 200 bytes for Basic strings, and before that,
the system stack should be given 512 bytes for a typical C program.
These assumptions mean that an executable should not go beyond $7D38.

**If a program crashes the CoCo just by getting loaded**, it could
be because it is too long and overwrites the system stack. The
`parse-coco-bin` Perl script, available on the CMOC home page,
can be useful to confirm that.

The following example program prints the start and end of the memory
it uses initially:

    #include <cmoc.h>
    int main()
    {
        char *s, *e;
        asm
        {
            leax    program_start,pcr
            stx     :s
            leax    program_end,pcr
            stx     :e
        }
        printf("START AT %p, END AT %p.\n", s, e);
        return 0;
    }

Note that the label addresses are taken relatively to the
program counter (`,pcr`), so that this program will report
the correct addresses even if the program is loaded at an
address other than the one specified in the .bin file.
(This can be done by passing a second argument to Disk Basic's
LOADM command.)

This section assumes that the code and data sections form a single
block.  When using the `#pragma data` directive (see elsewhere
in this manual), the *writable* globals will not be between
`program_start` and `program_end`.

### Enforcing a limit address on the end of the program

CMOC accepts the `--limit=X` option.  When it
is passed, CMOC will fail if the end of the program, as indicated
by the `program_end` listing symbol, exceeds address X (specified
in _hexadecimal_).

For example, `--limit=7800` will keep the program from finishing
too close to the system stack under Disk Basic, which is not far
below $8000. A limit at $7800 leaves two kilobytes for the stack
and for Basic's string space.

### Fitting a large program in memory when loading with LOADM

The memory layout under DECB is the following:

*   Various stuff at $0000.
*   PMODE graphics memory.
*   Basic program and its variables, starting at $2600.
*   Free memory (variable size).
*   Stack space (variable size).
*   String space (200 bytes by default).
*   Memory reserved by CLEAR, if any.
*   Basic interpret ROM, starting at $8000.

CMOC generates .BIN files that, by default, load at $2800.
This is intended to leave 512 bytes for a small Basic loader
like `10 LOADM"FOO":EXEC`.

If the .BIN file loaded by LOADM is too large, it will
extend into the string space, which by default occupies
the $7Fxx region, and possibly addresses below that.
The sectors loaded by LOADM will overwrite the stack,
and as soon as LOADM has finished, Basic is likely to crash
because it uses a corrupted stack space.

To detect that problem at compile time rather than run time,
the `--limit` option can be used, as described in the previous
section.

**To create more space for the .BIN file**, the following
steps may be possible:

*   **Reducing the string space.**
    If the Basic loader does nothing else than LOADM and EXEC,
    then the only string space needed is a number of bytes
    equal to the length of the filename passed to LOADM.
    If for example the filename is 8 characters long, then
    `CLEAR8` can be put before the LOADM command, e.g.,
    `10 CLEAR8:LOADM"ABCDEFGH":EXEC`. This will in turn
    move the _stack_ space up by 192 bytes and provide
    that much more space to the .BIN file.

*   **Allocating only one PMODE graphics page**, if the program
    does not use those pages, or if it only uses the first page
    (1.5k). This will save 4608 bytes (3 times 1536 bytes).
    This is done by starting the Basic loader with `PCLEAR1`.
    This moves the Basic program to $1400.
    The .BIN file can then be positioned at $1600,
    to allow 512 bytes for a Basic loader, like before.
    This is done by passing `--org=1600` to cmoc.

If these steps are still not enough, and a CoCo with at least 64k
is targeted, then getting rid of the Basic ROMs, in order to use
the full 64k, can be considered.
This involves developing a small .BIN that acts as the loader
of the actual, larger application. This loader performs these steps:
    
*   masks the interrupts;
*   possibly moves itself to low memory (requires
    relocatable code; do not use `--no-relocate`);
*   puts the CoCo in all RAM mode (by clearing $FFDF);
*   moves the S register to the bottom of a new stack space,
    e.g., at $FE00;
*   jumps to the next phase in the moved code, if the loader
    moved itself to low memory;
*   redirects the IRQ and NMI interrupt to its own handlers;
*   reenables the interrupts;
*   loads the actual application, compiled as a large
    raw binary with `--raw`;
*   masks the interrupts again;
*   jumps to the start address of the loaded raw binary,
    which typically sets up its own interrupt handlers,
    then reenables the interrupts.

The `decbfile_runDOSLoader()` function of the decbfile library
can help with this. This library is available on the CMOC page.

The loader and the raw binary must be shipped together.

The .BIN loader can possibly be replaced with a track 34
loader that gets launched by the DOS command.
See the "Building an executable to be launched by the DOS command"
section for details on this.


### Determining that CMOC is the compiler

Since version 0.1.48, CMOC automatically defines `_CMOC_VERSION_`
to be a 32-bit number of the form XXYYZZZ when the version
is X.Y.Z. For example, version 0.1.48 defines `_CMOC_VERSION_`
as 1048. This can be useful when some C code needs to do something
under CMOC but something else under another compiler.

### Specifying the target platform

By default, the compiler defines the `_COCO_BASIC_` preprocessor
identifier.  This identifier can be used to adapt a program to
make it use alternative code depending on whether it will run under
Disk Extended Color Basic or not.

To target **OS-9**, pass the `--os9` option. The compiler will define `OS9`.

To target the **Vectrex**, pass `--vectrex`. The compiler will define `VECTREX`.

To target the **Dragon**, pass `--dragon`. The compiler will define `DRAGON`.

To target the **Thomson MO**, pass `--thommo`. The compiler will define `THOMMO`.

To target the **Thomson TO**, pass `--thomto`. The compiler will define `THOMTO`.

To target the **FLEX** operating system, pass `--flex`. The compiler will define `FLEX`.


In the Thomson cases, the executable will have the `.bin` extension and be
in the CoCo Disk Basic BIN format.
This format is documented in the _Disk Basic Unravelled_ book.
CMOC does not provide the tools needed to convert this format to the Thomson
computers' native format.

When passing `--usim`, the compiler targets the [USim 6809 simulator](https://github.com/raybellis/usim),
which CMOC is shipped with.
The `USIM` identifier will be defined.
No .bin file is produced in this case.
The .srec file can be executed by passing its path to
`src/usim-0.91-cmoc/usim`.

### The standard library

CMOC's standard library is small.
The program must `#include <cmoc.h>` to use functions like printf().
See that file for a list of implemented C functions.
Many are C functions while others are CMOC extensions.
("Standard" here means that those functions come with CMOC, not that
CMOC aims to provide a complete [C standard library](https://en.wikipedia.org/wiki/C_standard_library).)

#### Provided header files

The following header files are provided with CMOC.
They should be used by a directive of the form `#include <_____.h>`.

* `<assert.h>`: Defines assert(), a macro that aborts the program if the assertion it is given is false.
* `<cmoc.h>`: Declares printf() and several other functions from C.
* `<coco.h>`: Declares several CoCo-specific functions, including initCoCoSupport(),
              which should be called first by a program that targets the CoCo.
* `<dskcon-standalone.h>`: Declares dskcon\_init(), dskcon\_processSector(), etc. to read and write
                           Disk Extended Color Basic floppy sectors without the presence of DECB.
* `<setjmp.h>`: Declares C functions setjmp() and longjmp(), which can be used to perform
                a nonlocal goto.
* `<stdarg.h>`: Declares C macros va\_start(), va\_arg() and va\_end(), which facilitate
                the implementation of functions that take a variable argument list, like printf().
* `<vectrex.h>`, `<vectrex/bios.h>`, `<vectrex/stdlib.h>`: Various definitions for the Vectrex console.

#### printf()

CMOC's [printf()](https://en.wikipedia.org/wiki/Printf_format_string)
function supports %u, %d, %x, %X, %p, %s, %c, %f and %%.
Specifying a field width is allowed, except for %f.
Left justification is supported.
Zero padding for an integer is supported (e.g., `%04x`, `%012ld`).

The `l` modifier is supported for 32-bit longs (e.g., "%012ld");

%p always precedes the hexadecimal digits with `$`, as per
the CoCo assembly language notation. %x and %X do not generate
such a prefix. %p, %x and %X always print letter digits as
capital letters (`A` to `F`, not `a` to `f`).

printf(), like putchar() and putstr(), sends its output one
character at a time to Color Basic's PUTCHR routine, whose
address is taken from $A002.

#### Redirecting the output of printf()

The standard library's printf() writes the characters of the formatted
string by calling the routine whose address is stored in the library's
CHROUT global variable (not to be confused with Color Basic's CHROUT
vector at $A002).
The same applies to functions sprintf(), putchar() and putstr().

By default, under Color Basic,
that routine is the one found in that $A002 vector.
To designate a C function as the new character output routine,
first define the new routine:

    void newOutputRoutine(void)
    {
        char ch;
        asm
        {
            pshs    x,b  // preserve registers used by this routine, except A
            sta     :ch
        }

        // Insert C statements that print or otherwise process 'ch'.

        asm
        {
            puls    b,x
        }
    }

This routine will receive the character to be printed in register A.
It **must** preserve registers B, X and U.
It does not have to preserve A.
A normal CMOC function will automatically preserve U, but an assembly-only
function will not, so it must preserve U itself.

When targeting for the Color Basic environment, the routine will receive
character code 13 to represent a new line, rather than code 10.

Install the routine at the appropriate time with this call:

    ConsoleOutHook oldCHROOT;

    oldCHROOT = setConsoleOutHook(newOutputRoutine).

To restore the original output routine, do this:

    setConsoleOutHook(oldCHROOT);

#### sprintf()

This function is like printf(), but it writes into a memory buffer
whose address is passed as the first argument, before the format string.
For example:

    char greeting[32];
    void f(char *name)
    {
        sprintf(greeting, "Hello, %s.", name);
    }

Calling f("Lonnie") will write "Hello, Lonnie." in the greeting[] array,
including a terminating '\\0' character. A total of 15 bytes
get written to the start of that array. 

The **caller is responsible for providing a buffer long enough** to
receive all the text resulting from the format string and its arguments,
*including the terminating '\\0' character*.

In this example, the longest "name" that can be safely passed to f()
would be a 23-character name.

The standard C language offers snprintf(), which is safer because it requires
passing the length of the destination buffer. But checking for
this limit would have a performance hit that is not necessarily
acceptable on a CoCo. If such a function is needed, it can be
implemented using the technique described in the previous section.

#### printf() and sprintf() return value

CMOC's versions of printf() and sprintf() always return 0, unlike
what is specified by Standard C.

A version of sprintf() can be coded in the following way to obtain
the number of characters effectively written.

    int sprintf_with_return_value(char *dest, const char *format, ...)
    {
        va_list ap;
        va_start(ap, format);
        vsprintf(dest, format, ap);
        va_end(ap);
        return (int) strlen(dest);
    }

#### readline()

readline(), declared by `#include <cmoc.h>`, acts like Basic's LINE INPUT
command and returns the address of the (NUL-terminated) string entered.
This address is a global buffer.
The next call to readline() will overwrite that buffer.

#### Redefining a standard library function

Defining a function using the name of a standard library function
is allowed.  For example, a program could redefine `rand()` and
`srand()` to implement an alternative random number generator.
In the final assembly program, such functions replace the ones
provided by the standard library.

#### Dynamic memory allocation with sbrk()

sbrk() is a function that dynamically allocates a region of memory
of the size (in bytes) passed to it as an argument. It returns a
void pointer. If the quantity of memory requested is not available,
sbrk() returns `(void *) -1`. For example:

    void *p = sbrk(100);
    if (p != (void *) -1)
        memset(p, 'X', 100);

In the case of the CoCo and the Dragon, the assumption is that the program is
loaded after the Basic program and variables. This means the space that
`sbrk()` can allocate from goes from there to the top of the stack,
which is around $3F00 on a 16K CoCo and $7F00 on a CoCo with 32K or more.
Do not use `sbrk()` if these assumptions do not apply, e.g., when
using `--data` to position the writable globals elsewhere than
right after the code and read-only data.

On the CoCo and the Dragon, use the `--stack-space` option or the
`#pragma stack_space` directive (documented elsewhere in this manual)
when the program needs more than 1024 bytes of stack space.

To determine how much of that memory is available for sbrk(),
call sbrkmax(), which returns the number of bytes as a size\_t
(unsigned).

sbrk() and sbrkmax() are declared by cmoc.h.

### Inline assembly

Inline assembly text can be specified by surrounding it with `asm {`
and `}`.

In the text, one can refer to C variables (global, local and parameters)
as well as functions.

Labels can be used for branch instructions, but a label must either be
unique to the whole program or comply with what lwasm considers a "local" label.
A label must appear at the very beginning of the line, without spaces or tabs
in front of it.

One way of using lwasm **local labels** is to prefix the label name with
the @ character. Such a label will be local to the current block, which
will begin at the previous blank line (or start of the `asm` block) and
end at the next blank line (or end of the `asm` block). Refer to the
[LWASM manual about its symbols](http://lwtools.projects.l-w.ca/manual/manual.html#AEN237)
for details on using local labels.

The assembler may also support `$` as a local label marker, but it is not
recommended to use it that way in inline assembly because it may hinder
portability to OS-9, where `$` is used in labels but not as a local label marker.

The following example fills array `out` with `n` copies of character `ch`,
then returns the address that follows the region written to.
(References to C variables should be preceded by a colon to avoid ambiguities,
especially when a C variable has the same name as a processor register.)

    #include <cmoc.h>

    char *f(char *out, char ch, unsigned char n)
    {
        char *end;
        asm
        {
            ldx     :out        /* comments must be C style */
            lda     :ch         // or C++ style
            ldb     :n          ; or semi-colon comment (passed to assembler)
    @loop:
            sta     ,x+
            decb
            bne     @loop
            stx     :end
        }
        return end;
    }

    int main()
    {
        char a[10];
        a[9] = '\0';
        char *p = f(a, 'X', (unsigned char) sizeof(a) - 1);
        printf("a='%s', %p, %p\n", a, a, p);
        return 0;
    }

Since version 0.1.21, when referring to a C function, the function
name is replaced with its assembly label, possibly followed by the
`,pcr` suffix.  This suffix is omitted if the instruction is BSR,
LBSR or JSR, because these instructions do not support the `,pcr` suffix.
The following example calls the same C function three different ways:

    asm
    {
        jsr     someCFunction
        lbsr    someCFunction
        leax    someCFunction
        jsr     ,x
    }

Note that CMOC always converts C code to position independent code by default.
Inline assembly statements should be written the same way if the resulting program
is to be relocatable.

The BSR instruction is not recommended because it is a short branch
and if the called function is too far from the call, the assembly
step will fail.

Semi-colon comments are supported. They are passed verbatim to the assembler.

Note that using inline assembly is likely to make the program non portable
to other C compilers.

See the *Calling convention* section elsewhere in this manual
for the rules to observe. Namely, inline assembly must not modify
U or Y. It is allowed to modify D, X and CC.

#### Arrays and struct fields

Since 0.1.79, referring to a struct field is supported:

    asm
    {
        inc     :someObject.someField
    }

When using an index into a C array, the index is in array elements, not in bytes:

    int v[5];
    asm
    {
        ldd     :v[4]       ; refers to last int in v[],
                            ; i.e., bytes 8 and 9 of the 10-byte array
    }

When using an index into a non-array, the index is in bytes:

    long n;
    int i;
    asm
    {
        clr     :n[3]       ; clears the least significant byte of n
        ldb     :i[1]       ; loads the least significant byte of i
    }


#### Preprocessor identifiers in inline assembly

The GNU C preprocessor can add spaces in surprising ways, which makes its
use slightly problematic in inline assembly. For example:

    #define PIA0 0xFF00

    asm
    {
        stb     PIA0+2
    }

One would expect this code to generate an `stb 0xFF02` instruction,
but cpp will actually expand this to `stb 0xFF00 +2`, because it
apparently adds a space after the expansion of the PIA0 identifier.

The assembler takes this space as the start of the comment, so it ignores
the +2 and assembles `stb $FF00`.

A workaround appears to be to reverse the addition and write `stb 2+PIA0`.
No space gets added before the identifier.

Therefore, preprocessor identifiers should be used with caution in
inline assembly.


#### Referring to variables whose name is that of a register

Before version 0.1.31, bad assembly language text could be emitted
if inline assembly referred to a C variable that has the name of a
register. To help resolve this ambiguity, version 0.1.31 introduced
a _C variable escape character_, which is a colon that is put in
front of the C variable name.

For example:

    char b;
    asm
    {
        inc     :b
        ldb     :b
        leax    b,x
    }

Here, `:b` refers to variable _b_ of the C program, while the _b_
in _b,x_ refers to the register.

This change may break inline assembly code in programs that were
written for versions of CMOC preceding 0.1.31. Adding a colon at
the right places will resolve the issue.

Note that the escape character is not necessary on variable names
that are not also register names.


#### Assembly-only functions

When a function is written entirely in assembly language for
performance reasons, the stack frame may not be necessary.
CMOC will not generate a stack frame for a function defined
with the `asm` modifier, as in this example:

    asm int f(int m, int n)
    {
        // U not pushed, so 1st argument is at 2,s
        asm
        {
            ldd     2,s     // load m
            addd    4,s     // add n, leave return value in D
        }
    }

Only `asm { ... }` statements are allowed in such a function.
Typically, only one is needed. Local variables cannot be defined
in that function and the function's parameters cannot be accessed
by name.  The assembly code is allowed to refer to global
variables however.

By default, the compiler ends the function with the RTS instruction
(or RTI if the function is declared with the `interrupt` modifier).
To keep the compiler from emitting that instruction (RTI as well as RTS),
add the `__norts__` keyword:

    asm __norts__ int f(int m, int n) { ... }

See the *Calling convention* section elsewhere in this manual
for the rules to observe. In particular, note that byte arguments
are promoted to words, which are pushed onto the stack in the big
endian byte ordering.


#### Hitachi 6309 instructions

Inline assembly can use [Hitachi 6309](https://en.wikipedia.org/wiki/Hitachi_6309)
instructions like TFM, LDQ, etc.
The underlying assembler (lwasm) accepts them by default.

The code generated by CMOC does not use the 6309-specific registers or instructions.


#### At global scope

Since CMOC 0.1.84, verbatim assembly block of the form `asm { ... }`
are allowed at global scope. For example:

    asm
    {
    foo:
    }

    void bar() {}

CMOC guarantees that this assembly block will be emitted
just before `bar()`, with no other intervening instructions or
assembly labels or directives, except for the EXPORT directive
that exports `bar()`, since it has external linkage.

The above example defines a `foo` label whose value will be equal
to the address of `bar()`.

If a sequence of more than one assembly blocks precedes a certain
C function, those blocks are all emitted, in order, just before
that function's code.

Inline assembly blocks that are not followed by a C function are
emitted, in order, after the functions have been emitted.


### Interrupt Service Routines

CMOC supports the `interrupt` function modifier, which tells the
compiler to end the function with an RTI instruction instead of
an RTS. For example, the following function handles the VSYNC
60 hertz interrupt:

    interrupt void newCoCoIRQRoutine(void)
    {
        asm
        {
            lda     $FF03           // check for 60 Hz interrupt
            lbpl    irqISR_end      // return if 63.5 us interrupt
            lda     $FF02           // reset PIA0, port B interrupt flag
        }

        // Do something in C.

        asm
        {
    irqISR_end:
        }
        // Nothing here, so that next instruction is RTI.
    }

This routine could be hooked to the IRQ vector this way on a CoCo:

    disableInterrupts();
    char *irqVector = * (char **) 0xFFF8;
    *irqVector = 0x7E;  // extended JMP instruction
    * (void **) (irqVector + 1) = (void *) newCoCoIRQRoutine;
    enableInterrupts();

Header `<coco.h>` defines macros disableInterrupts() and enableInterrupts().
They set and reset the F and I condition codes.

The FIRQ vector is at $FFF6.

Note that using the `interrupt` keyword is likely to make
the program non portable to other C compilers.


### Function pointers

The address of a function can be taken and stored in order to be
called through that pointer later.

The following example shows that the two syntaxes used in C to call
a function through a pointer are supported by CMOC:

    unsigned char f(int a, char b) { ... }
    int main()
    {
        unsigned char (*pf)(int a, char b) = f;
        unsigned char c0 = (*pf)(1000, 'x');
        unsigned char c1 = pf(1001, 'y');
        return 0;
    }

A member of a struct can point to a function. For example:

    struct S
    {
        void (*fp)();
    };
    void g() { ... }
    int main()
    {
        struct S s = { g };  // member 'fp' points to g()
        s.fp();  // call g()
        (*s.fp)();  // call g()
        return 0;
    }


### Array initializers

#### Local vs. global array

When only a single function needs to use a read-only array of
integers, this array could be defined locally to that function,
but it is more efficient, as of CMOC 0.1.10 to define the array as
global. This is because the global array will be emitted as a
sequence of FCB or FDB directives, while a local array will be
initialized with a series of load and store instructions.

In the following example, array `g` will be initialized more
efficiently than array `l`:

    int g[] = { 55, 66, 77 };
    void f()
    {
        int l[] = { 22, 33, 44 };
        /* ... */
    }


#### Execution of global array initializers

A global array initializer containing only integer values (not
string literals) is treated specially. It is initialized
at compile-time, not at run-time. This means that if the program
modifies the array, quits, and then is reexecuted, the modified
contents will still appear as is. The array will not be reinitialized
to the values in the initializer.

For example:

    #include <cmoc.h>
    int a[2] = { 13, 17 };
    int main() { a[0]++; printf("%d\n", a[0]); return 0; }

The first time this program is executed, a[0] starts at 13,
then is incremented to 14, which is the number that is printed.

The second time this program is executed, a[0] starts at 14
because array `a` is _not_ reinitialized upon entry.

This is meant to save memory by not including a second copy
of the initializer just for run-time initialization purposes.


### Constant initializers for globals

CMOC considers a global variable's initializer to be constant if
it is made of:

- numerical literals;
- constant numerical expressions;
- string literals;
- the address of a variable;
- a function name;
- an array name
- an arithmetic expression (using +, -, *, / or %) involving only
  one variable and one or more constant numerical expressions;
- a sequence of constant initializers between braces.


### Union initializers

An initializer used to initialize a union variable must match the
type of the first member of the union:

    union Word
    {
        unsigned u;
        char b[2];
    };
    union Word w0 = { 1000 };  // ok: 1000 matches unsigned u
    union Word w1 = { { 'x','y' } };  // error: does not match unsigned u


### Array sizes

One must be careful when specifying an array size using an
arithmetic expression, like this:

    char a[256 * 192 / 8];

This will generate an "invalid dimensions" error message.  This is
because the three numerical constants are of type `int`, which means
they are signed integers.  In 16-bit signed arithmetic, 256 \* 192 is
-16384, which gets divided by 8, which is -2048. This size is rejected
by the compiler, which only accepts array sizes between 1 and 32767
bytes inclusively.

The fix is to force the expression to be evaluated with
unsigned integers:

    char a[256U * 192U / 8U];

This will be 6144 bytes, as intended.


### Compiling a ROM image for a cartridge

To help support the ROM cartridge format, CMOC supports directives
that allow telling it to assemble the code, the string, long and real literals,
and the read-only global variables at the typical CoCo cartridge ROM
address of $C000, while mapping the writable global variables
at a RAM address like $3800.

This is achieved by using the `--org` and `--data` command-line options.
If the following program is in prog.c:

    int f() { return 42; }

    const int g = 100;
    const unsigned char byteArray[3] = { 11, 22, 33 };
    const char text[] = "hello";

    int anotherWritableGlobal;

    int main()
    {
        anotherWritableGlobal = 99;
        return 0;
    }

Then it can be compiled for a cartridge with this command, which
specifies hexadecimal addresses:

    cmoc --raw --org=C000 --data=3800 prog.c

The `--raw` option tells CMOC to generate only the machine code,
without any of the headers that appears in the .bin format.
The execute will be named `prog.raw`.

When developing a program made of multiple C files, `--org` and
`--data` should only be passed to the compiler invocation that links
the executable from the object files.

In the code above, variable `g` is read-only because it is of a
constant type.  `byteArray` and `text` are read-only because they are
arrays whose elements are of a constant type.

These three variables are thus automatically put in the read-only
section, next to the code. This means they will be part of the
cartridge ROM.

In the case of `text`, the use of the empty brackets is necessary for
that variable to be seen as read-only.
Declaring this variable as `const char *text` would lead the compiler
to see it as writable: the `text` pointer itself can be modified,
although the characters it points to cannot.

In the case of a cartridge program, no writable global variable can have
an initializer.  That is because the ROM file format has no support for
mapping a value into RAM, like the .bin format has.

After compiling the program, it can be tested with the XRoar emulator,
but first it must be padded at the end with enough bytes so that the
total file length is a multiple of 256. The following Perl script
does this:

    #!/usr/bin/perl
    sysread(STDIN, $rom, 0xFFFF) > 0 or die;
    my $romLen = length($rom);
    binmode STDOUT or die;
    print $rom;
    my $extra = $romLen % 256;
    print chr(0) x (256 - $extra) if $extra;

This script, in a file called bin2cart.pl, can be used this way:

    perl bin2cart.pl < foo.raw > foo.rom

This ROM image can be tested in the XRoar emulator this way:

    xroar -machine cocous -cart-autorun -cart foo.rom

Note that XRoar requires the image file to have the .rom extension.

In a cartridge-based program written as above, the CoCo's 60 Hz IRQ
interrupt is not enabled.  To enable the IRQ in such a program, put
this at the beginning of the `main()` function:

    asm
    {
        // We come here from a JMP $C000 in Color Basic (normally at
        // $A10A in v1.2). At this point, the 60 Hz interrupt has
        // not been enabled yet, so enable it.
        lda     $FF03   // get control register of PIA0, port B
        ora     #1
        sta     $FF03   // enable 60 Hz interrupt

        // Unmask interrupts to allow the timer IRQ to be processed.
        andcc   #$AF
    }

Function `sbrk()`, which allocates memory dynamically, should be avoided
because it expects to run in the default Disk Basic environment.


### Enumerations (enum)

Enumerations are supported, with the restriction that an enum must
be declared at the global scope. In particular, an enum with an
enumerator list (e.g., `enum { A, B }`) cannot be used in the formal
parameter of a function, nor as its return type.

An enum can be anonymous (e.g., `enum { A, B }`) or be given a name
(e.g., `enum E { A, B }`).

Each enumerated name can have a specified value, e.g.,
`enum { SCREEN = 0x0400, BIT5 = 1 << 5 }`. Such a value must be
a constant expression.


### Floating-point arithmetic

[Floating-point arithmetic](https://en.wikipedia.org/wiki/Floating-point_arithmetic)
is supported in some situations:

* when targeting the CoCo's with Extended Color Basic present;
* when targeting the Dragon with its Basic present;
* (since CMOC 0.1.86) when compiling with **--mc6839**, which involves adding a copy
  of Motorola's MC6839 floating-point ROM.

With **--mc6839**:

* At least **8k gets added** to the program code, because
  the whole ROM image gets added as is. Also, a 32-bit float format is
  used instead of the 40-bit format used by CoCo and Dragon Basic.
  This means that the numbers have about 7 significant digits instead of 9.
* The **`main()` function must first call `enable_printf_float()`**
  if the program wants to use the `%f` placeholder of CMOC's `printf()` function.
  (If this is not done, `printf()` will print an exclamation point (`!`)
  instead of the value of the number.)
* A trap function can be set by calling `setMC6839Trap()` (declared by `cmoc.h`).
  This function will be called when an MC6839 operation fails.

In all cases,
a warning is issued when the `double` keyword is encountered, so the
user knows not to expect double-precision. There is also a warning
when a numeric literal does not use the `f` suffix, which specifies
that the literal is single-precision. There too, double-precision
must not be expected. It is recommended to code programs using the
`float` type and the `f` suffix.

The compiler will fail to compile a program that uses floating-point
arithmetic when a platform other than the ones supported is targeted.
Using `--mc6839` may then be an option.

CMOC's `printf()` function supports the `%f` placeholder, but
it does not support the width or precision parameters of `%f`
(e.g., `"%7.3f"`).

The `<cmoc.h>` header file provides functions `strtof()` and `atoff()`
to convert an ASCII decimal representation of a floating-pointer number
into a float value, as well as `ftoa()`, to convert a float value into
an ASCII decimal representation.

Note that CMOC's float support **must not be assumed to be thread-safe**.
This means that if a multitasking system is in place, other than OS-9,
then only one thread at a time can make use of float operations.
If an interrupt service routine is getting executed, it too must not
do float operations if the main program is making use of them.
(The reason for this restriction is that some floating point libraries
use software floating point accumulators, e.g., FPA0 and FPA1 in the
Extended Color Basic case.)

Under OS-9, multitasking is achieved by running more than one process,
and each process will have its own set of software floating point
accumulators.


### Function Names as Strings

The constants `__FUNCTION__` and `__func__` can be used to refer to
the current function's name:

    printf("Now executing %s().\n", __func__);

In the global namespace, these identifiers give the empty string.


### Detecting null pointer accesses at run time

Accessing a value through a null pointer is a common bug.
To help detect such accesses, CMOC has the --check-null command-line
option, which adds run-time checks before every use of a pointer and
every array element access.

By default, the handler that is invoked when a null pointer access
is detected is a no-op. The program should start by calling the
`set_null_ptr_handler()` function to set up a handler that will
receive the address of the failed check as an argument. For example:

    #include <cmoc.h>

    struct S { int n; };

    void nullPointerHandler(void *addressOfFailedCheck)
    {
        printf("[FAIL: %p]\n", addressOfFailedCheck);
        exit(1);
    }

    int main()
    {
        set_null_ptr_handler(nullPointerHandler);
        struct S *p = 0;
        p->n = 42;
        return 0;
    }

This program will fail and display an address.  One can then look up
this address in the .lst listing file generated by CMOC to determine
in which function that null pointer was detected.

Using this option incurs a performance cost and significantly adds
to the code size (about 9%), so it is only recommended
during debugging. An alternative is to define an `assert()` macro,
as in Standard C, that expands to nothing when `NDEBUG` is defined.


### Detecting stack overflows at run time

Runaway recursion or excessively long local arrays can cause corruption
that is difficult to investigate. To help detect stack overflows,
CMOC has the --check-stack command-line option, which adds run-time checks
at the beginning of each function body.

When passing --check-stack to CMOC, the program should start with
a call to `set_stack_overflow_handler()` that installs a handler.
This handler receives two arguments: the address of the failed check
and the out-of-range stack pointer. The handler must not return.
For example:

    #ifdef _CMOC_CHECK_STACK_OVERFLOW_
    void stackOverflowHandler(void *addressOfFailedCheck, void *stackRegister)
    {
        printf("[FAIL: %p, %p]\n", addressOfFailedCheck, stackRegister);
        exit(1);
    }
    #endif
    void recurse() { recurse(); }
    int main()
    {
        #ifdef _CMOC_CHECK_STACK_OVERFLOW_
        set_stack_overflow_handler(stackOverflowHandler);
        #endif
        recurse();
        return 0;
    }

This program will fail and display two addresses. One can look up
the first one in the .lst listing file generated by CMOC to determine
in which function that stack overflow was detected.

Using this option incurs a performance cost, so it is only recommended
during debugging.

The preprocessor identifier `_CMOC_CHECK_STACK_OVERFLOW_` is defined
by CMOC when --check-stack is used. This identifier can be used to
exclude stack check code when not needed.

By default, CMOC allocates 1024 bytes for the stack on a CoCo
(256 bytes on a Vectrex). In a program that is compiled with the
default layout, the stack is preceded by the memory managed by sbrk()
(see elsewhere in this manual).

Note that this feature is not usable under **OS-9**, where stack
checking is automatic and uses a different mechanism.

#### Specifying the space allocated to the system stack

Option `--stack-space=`_N_ (with _N_ > 0) can be used to specify
the number of bytes (in decimal) to be reserved to the stack. This
affects the quantity of memory available to sbrk() and the stack
checking done by `--check-stack`.  Use this option when the program
uses lots of stack space. Note that this option is not permitted when
targeting the Vectrex.

The stack space can also be specified with `#pragma stack_space` _N_.
The command-line option has precedence over this pragma.

Specifying the stack space must be done when compiling the C file
that defines main(). It has no effect when compiling a file that
does not define main(), nor when calling CMOC to perform linking
on a list of object files.

Note that this feature is not usable under **OS-9**, where stack
checking is automatic and uses a different mechanism.

#### OS-9 stack checking

When targeting OS-9, by default, a stack check is performed upon
entering each function, unless that function is assembly-only.

The stack check verifies that there will be at least 64 bytes of free
stack space once the function will have allocated stack space for its
local variables. If this check fails, a "stack overflow" error
message is printed in the standard output and the program exits.

To change this 64 to another (non-zero) value, use the command-line
`--function-stack` option. Pass 0 to this option to disable the
stack check.


### Calling convention

CMOC follows the C [convention](https://en.wikipedia.org/wiki/Calling_convention)
of passing the parameters in the stack in the reverse order.

The caller pops them off the stack after the call.

An argument of type `char`, which is signed, is promoted to `int`.

An argument of type `unsigned char` is promoted to `unsigned int`.

When a struct passed by value, it is pushed exactly as it is, without
any padding, except if its size is 1: then an additional dummy byte
is pushed afterwards. (The same thing is done when a byte-sized argument
is promoted to 16 bits. This is for the benefit of va_arg(), defined
in `<stdarg.h>`).

The return value must be left in B if it is byte-sized or
in D if it is 16 bits.
If the return value is a struct, a long, a float or a double,
then the return value must be stored at a location whose address
is received by the function as its first (hidden) parameter.

The body of a CMOC-generated function preserves registers U, Y, S and DP.
It is allowed to modify A, B, X and CC. 

**Under OS-9, CMOC uses Y to refer to the data section** of the current
process. Any code that needs to use Y must preserve its value and
restore it when finished. For portability, this rule should also
be observed on platforms other than OS-9.

The compiler's low-level optimizer may emit code that uses Y when
targeting a platform other than OS-9.

The called function does not have to return any particular condition
codes.

Register DP is not used or modified by CMOC-generated code.

A CMOC-generated function uses a [stack frame](https://en.wikipedia.org/wiki/Call_stack#STACK-FRAME)
if the function receives
one or more parameters or has local variables, and is not defined
with the `asm` keyword (see the *Assembly-only functions* section
elsewhere in this manual).
Register U is used as the stack frame
pointer. Such a function starts with these instructions:

        PSHS    U       
        LEAU    ,S

an ends with these:

        LEAS    ,U
        PULS    U,PC    

(An `interrupt` function will end with `PULS U` and `RTI`.)


#### GCC calling convention

Since version 0.1.87, CMOC has also supported the GCC 6809 calling
convention, which presents these differences:

*   The first 8-bit function parameter is passed in register B
    (instead of being pushed on the stack).
*   The first 16-bit function parameter is passed in register X
    (instead of being pushed on the stack).
*   A 16-bit return value is placed in X instead of D.

To indicate that a function must be compiled using the GCC calling
convention, the `__gcccall` keyword must be specified on the function
prototype or the function definition, before the name of the function.
For example, `__gcccall int foo(char c, int w);`.

If the keyword is specified on a function's prototype but not on
the corresponding function definition, the function will be compiled
using the GCC convention.

To declare several functions as using the GCC convention, the pragma
directives used in the following example can be used:

    void foo(int x);

    #pragma push_calling_convention __gcccall

    int bar(char c, int w);
    int baz(int w, char c);
    char quux(char c);

    #pragma pop_calling_convention

    void waldo(int y);

In this example, functions `foo()` and `waldo()` use the default
CMOC convention, while the three other ones use the GCC convention.
These pragmas have been available since version 0.1.88.


### Calling a program as a DEF USR routine

Typically, such a program gets loaded at the end of Basic's RAM area,
e.g., `&H7000`.
Pass `--org=7000` to the compiler to position the program at such an address.
In the (Extended Color) Basic program, reserve some high RAM to your CMOC program,
e.g., `CLEAR 200,&H6FFF`.
The second argument should be one less than the starting address
of the CMOC program.

Then use `LOADM`, then `DEF USR` to define a user routine that points
to the CMOC program, e.g., `DEF USR5=&H7000`.
(The 5 in this example can be any number from 0 to 9.)

Finally, call the CMOC program using the routine number (5 here)
and an argument, e.g., `R=USR5(-1000)`.

The following Basic program is an example:

    10 CLEAR 200,&H6FFF 'RESERVE MEM FOR .BIN
    20 LOADM "DEFUSR" 'LOAD .BIN (CMOC GOT --ORG=7000)
    30 DEFUSR5=&H7000 '5 IS AN EXAMPLE, COULD BE 0-9
    40 R=USR5(-1000) 'CALLS $7000
    50 PRINT "GOT ";R;"FROM USR5()."

The argument passed to `USR*()` is stored by Color Basic in its FPA0
floating-point accumulator.
If this argument is a signed 16-bit integer, this argument can be
obtained in the CMOC program with this inline assembly code:

    signed short arg;  /* 16-bit integer variable */
    asm
    {
        pshs    u,y     ; protect registers against Basic
        jsr     $B3ED   ; convert FPA0 to signed integer in D
        puls    y,u
        std     :arg
    }

Then the C variable `arg` will contain the argument value.

To return a signed 16-bit integer value to the caller of USR*(),
use code like this:

    signed short ret = -2000;  /* value to return */
    asm
    {
        ldd     :ret    ; signed integer to be returned by USRn() call
        pshs    u,y     ; protect registers against Basic
        jsr     $B4F4   ; GIVABF routine stores (signed) D in FPA0
        puls    y,u
    }


### Value returned by main()

Since CMOC 0.1.58, the main() return value, and the exit() argument,
are guaranteed to be returned in D upon exiting the program, when compiled
for the CoCo Basic environment or for the Dragon.

This can be useful to return a 16-bit address from one program for use
by another program.
Note that registers U and Y must be preserved (typically with PSHS and PULS)
when calling another program.


### Building an executable to be launched by the DOS command

Disk Basic 1.1 has a DOS command that loads track 34 at $2600
and if the contents start with the bytes 'O' and 'S', the interpreter
jumps at $2602.

CMOC can accommodate this with its --dos option. It implies --coco,
it uses $2600 as the code origin, and it generates the "OS" marker
at that address.

The output is still a .bin file. CMOC comes with a command-line tool
called `install-coco-boot-loader` that takes this file and copies
it to track 34 of a DECB disk image. For example:

    cmoc --dos prog.c
    install-coco-boot-loader foo.dsk prog.bin

This tool will fail if track 34 is already occupied. It has a `--force`
option that will install the boot leader despite that.

To uninstall the boot loader, use the `--uninstall` option:

    install-coco-boot-loader --uninstall foo.dsk


### C Preprocessor portability issues

For maximum portability, a CMOC program should not use the C Preprocessor's
"[stringification](https://gcc.gnu.org/onlinedocs/gcc-4.7.0/cpp/Stringification.html)"
operator (#), e.g.:

    #define f(x) #x

When this feature is supported by cpp, the expression `f(foobar)` yields
`"foobar"`, including the double quotes.

When this feature is not supported, the pound sign is left as is, so CMOC
will see `#foobar`, which will typically give a syntax error. This has been
observed on the Macintosh (Darwin) as of May 2017.

Also to be avoided is the use of C++-style comments on a line to be
processed by cpp, e.g.:

    #define foo 42  // blargh

Some preprocessors may leave the comment instead of stripping it away,
so the following should be used instead:

    #define foo 42  /* blargh */


### Local variable hiding

Versions 0.1.66 and later have a `-Wlocal-var-hiding` command-line option
that makes the compiler issue a warning if a local variable hides another one
declared in the same function. In the following example, the second declaration
for `v` hides the first one, so the option will cause a warning to be issued
for the second declaration:

    void f(int c)
    {
        int v = 0;
        g(&v);
        if (c)
        {
            int v = 1;
            g(&v);
        }
    }


### Relocatability

By default, the code generated by CMOC is relocatable. The `--no-relocate`
command-line option can be passed to the compiler to tell it that
relocatability does not have to be supported. This allows the compiler
to do some optimizations.

When `--no-relocate` is passed, the C preprocessor identifier `_CMOC_NO_RELOCATE_`
is automatically defined.


### Avoiding CMOC's default C library

The `-nodefaultlibs` option tells CMOC to avoid linking the program with `libcmoc-std-*.a`,
which contains the small C library that comes with the compiler.
This option can be useful when targeting a platform that is not supported
by CMOC's library.

Note that assigning a struct variable to another will cause the compiler to generate
code that calls memcpy() to copy the struct.
If -nodefaultlibs is given, the program will not link,
because memcpy() is provided by `libcmoc-std-*.a`.
The program must thus provide its own implementation of memcpy().


### Making OS-9 system calls

As of CMOC 0.1.83, the way to make systems calls to OS-9 is to use
inline assembly. This section shows an example program, called `os9load.c`,
that loads the first few bytes of a file into a memory buffer.

    #include <cmoc.h>

    // filePath: Must end with '\r'.
    // Returns the number of bytes read from the file (may be lower than bufferLength).
    //
    size_t loadFileToBuffer(const char *filePath,
                            void *buffer,
                            size_t bufferLength)
    {
        // Open the file in read mode.
        unsigned char pathNumber, errorCode;
        asm
        {
            lda     #$01            ; read mode
            ldx     :filePath
            os9     $84             ; I$Open
            bcs     @error
            clrb
            sta     :pathNumber
    @error
            stb     :errorCode
        }
        if (errorCode != 0)
        {
            printf("I$Open failed: error #%u\n", errorCode);
            return 0;
        }

        // Load bytes from the file to the buffer.
        size_t numBytesRead;
        asm
        {
            lda     :pathNumber
            ldx     :buffer         ; address where to store the bytes
            pshs    y               ; save global data ptr; cannot refer to C globals for now
            ldy     :bufferLength   ; number of bytes to read
            os9     $89             ; I$Read
            bcs     @error
            clrb
            sty     :numBytesRead
    @error
            stb     :errorCode
            puls    y               ; restore global data ptr; can refer to C globals again
        }
        if (errorCode != 0)
        {
            printf("I$Read failed: error #%u\n", errorCode);
            // Do not return because we need to close the file.
        }

        // Close the file.
        unsigned char closeErrorCode;
        asm
        {
            lda     :pathNumber
            os9     $8F             ; I$Close
            bcs     @error
            clrb
    @error
            stb     :closeErrorCode
        }
        if (errorCode != 0)  // if read error
            return 0;  // do not report close error if any; read error already reported
        if (closeErrorCode != 0)
        {
            printf("I$Close failed: error #%u\n", errorCode);
            return 0;
        }
        return numBytesRead;  // success
    }

    int main() 
    {
        char contents[10];
        size_t numBytesRead = loadFileToBuffer("foo.txt\r", contents, sizeof(contents));
        printf("First %u bytes: [", numBytesRead);
        putstr(contents, numBytesRead);
        printf("]\n");
        return 0;
    }

It can be compiled by giving this command: `cmoc --os9 os9load.c`

Once the `os9load` executable has been transferred to NitrOS-9, it can
be used by first creating a `foo.txt` file (e.g., with `build foo.txt`).

If for example this file contains "`This is the foo.txt file.`",
then when `os9load` is run, it will print this:

    First 10 bytes: [This is th]

Details on the systems calls can be found in the OS-9 System Programmer's Manual.

Whenever the Y register is used in an OS-9 system call, it must be
preserved, typically with PSHS and PULS, as done above, because a CMOC program
uses that register to point to the process's data segment.

That segment contains the program's global variables. As long as the Y register
is used for something else, the CMOC program must refrain from referring to
global variables. (Local variables are accessed through the U or S registers.)


### Command line arguments received at run time

Under OS-9, the arguments passed to the command can be accessed by reading
the `argv` array passed to main() as in Standard&nbsp;C. No `#include` directive
is needed to access that array. CMOC does not come with a command-line
processing library like Unix's _getopt_.

Under the other targets supported by CMOC, there is no way to pass arguments
to the program in this Unix-like manner. Color Basic offers DEF&nbsp;USR as a
way to point to the executable and to pass it a numerical argument.
This argument can optionally be a VARPTR value, which allows passing
a refernce to a Basic numerical or string variable.
See the "Calling a program as a DEF USR routine" section for details.


### Troubleshooting

*   Compilation failures

    *   It is not recommended to run `cmoc` from the directory tree where
        it was built. It is recommended to run `make install` and to put
        `cmoc` in the executable path (`$PATH`).
        Command `which cmoc` can be used to confirm that `cmoc` is found
        in the expected path directory.
        If it displays nothing, the command is not in the path.

    *   Option `--verbose` can be passed to make it show exactly
        which options and arguments it passes to `lwasm` and `lwlink`.

    *   Option `-E` can be passed to make it output the result
        of the C preprocessor pass (to standard output).
        This can be used to see exactly how the `#defines` and `#includes`
        have been resolved.

    *   Option `-i` (or `--intermediate`) can be passed to make the
        compiler leave intermediate files instead of deleting them.
        This includes:
        
        *   the `.s` file that contains the **generated assembly language code**,
        *   the `.lst` file that contains the listing generated by `lwasm`,
        *   the `.link` script used by `lwlink`,
        *   the `.map` **linking map** file that shows the effective
            values of the assembly language labels and symbols.

*   Crash on startup

    *   The **program may be too long** and loading it in memory may have
        overwritten the memory used by the system stack. Under Color Basic,
        the stack is typically at $7Fxx on a CoCo with at least 32k of RAM.
        To detect this at build time, pass `--limit=7800` (in hex)
        to `cmoc` to have issue an error message with the program end
        exceeds $7800. No executable will be generated, which will avoid
        running an excessively long one.

    *   The program may have **tuned out memory** that is supposed to contain
        the code of **interrupt service routines** (ISRs). This can happen if the
        program switches to a CoCo's all-RAM mode, or it is changes the MMU
        blocks on a CoCo 3.

        *   The program should disable the interrupts before making such changes.
            Header `<coco.h>` provides `disableInterrupts()` and `enableInterrupts()`.
        
        *   Any redirection of interrupts should be made while interrupts
            are masked. In such a case, it is recommended to use self-sufficient
            ISRs, i.e., they do not rely on Color Basic's existing ISRs.

*   Frozen execution

    *   The progress of a program may be relying on the timer interrupt
        and its ISR.
        The expected interrupts will not happen if that **interrupt
        has inadvertently been left masked**, or if the interrupt
        has **not been redirected** to the correct ISR.

*   Other bugs
    
    *   **Binary operations on bytes give a byte under CMOC**, whereas
        they give an integer under Standard C. This may affect computations
        like `screenByte = screenStart + row * 32 + column`.
        If `row` is a byte variable, then `row * 32` will overflow a byte
        when `row` is greater than 7.
        Option `-Wgives-byte` can help detect such cases.
        The fix when using CMOC is to cast `row` to `unsigned short`
        to force a 16-bit multiplication.

    *   The **order of evaluation of the operands of binary operators**,
        like `+`, `&`, etc. is generally not guaranteed by Standard C,
        or by CMOC.
        This can be a problem in a case like `f() & g()`, if it is
        important for `f()` to be called before `g()`.
        As of CMOC 0.1.96, `g()` is called before `f()`.
        This behavior is not guaranteed to stay the same as CMOC evolves.
        The solution to ensure portability is to define `h()` to do
        `{ ReturnType a = f(); return a & g(); }`, then use `h()` instead
        of the original expression.

    *   **Inconsistent `#ifdef` directives** may generate invalid programs.
        If conditional compilation is used in a header file to determine
        the contents of a function or macro, care must be taken to ensure that
        the same condition is used for all C files involves in the program,
        so that they all agree.
    
    *   The **stack space may have been exceeded** by the depth of the
        C function calls. Excessive use of `sbrk()` may have over-allocated
        memory, especially if `--stack-space=N` was not called with the
        correct value (to inform `sbrk()` of the limit).
        Option `--check-stack` can be used to insert systematic checks.
        See section _Detecting stack overflows at run time_ elsewhere
        in this manual.

    *   To **rule out the possibility of a CMOC low-level optimizer bug**,
        option `-O0` can be passed to avoid that optimizer.
        Note that this option is likely to increase program size to a
        point where it might exceed the desired limits.

License
-------

**EXCEPT FOR THE FILES UNDER src/usim-0.91-cmoc in the CMOC source archive,
CMOC is distributed under the following license:**

This program is free software: you can redistribute it and/or modify
it under the terms of the **GNU General Public License** as published by
the Free Software Foundation, **either version 3 of the License, or
(at your option) any later version.**

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

For more details on USim, see <https://github.com/raybellis/usim>.

</body>
