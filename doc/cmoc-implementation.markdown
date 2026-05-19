<head>
<title>CMOC Implementation Guide</title>
</head>
<body style="margin-left: 50px; margin-right: 50px; margin-top: 30px; margin-bottom: 30px;">

CMOC Implementation Guide
=========================

**By Pierre Sarrazin**

`sarrazip@sarrazip.com`

Copyright &copy; 2003-2018

<http://sarrazip.com/dev/cmoc.html>

Distributed under the **GNU General Public License**,
**version 3 or later** (see the License section).

Date of this manual: 2018-11-14


Introduction
------------

This guide describes the implementation of the CMOC compiler. To
learn how to use it, refer to the CMOC Manual, distributed with
CMOC as cmoc-manual.markdown.

CMOC is written in [C++](https://en.wikipedia.org/wiki/C%2B%2B),
[GNU Bison](https://www.gnu.org/software/bison/) and
[Flex](https://en.wikipedia.org/wiki/Flex_%28lexical_analyser_generator%29),
with some support scripts written in [Perl](https://www.perl.org/).


Compiler structure
------------------

The main phases of this compiler are the following:

1. Lexical analysis.
1. Syntactic analysis.
1. Semantic checking.
1. Code generation.
1. Low-level optimization.


### Lexical analysis

This phase reads a file of characters and produces a stream of tokens.
The file `lexer.ll` is a GNU Flex source file that gets converted
into a C++ file named `lexer.cc`. The list of all token names
(e.g., INT, CHAR, etc.) is defined with `%token` directives in the
parser.yy, which defines the syntactic analyzer.


### Syntactic analysis

This phase reads the tokens produced by the lexical analyzer and
checks that those tokens conform to a grammar that is close to that
of a large subset of the C language. This grammar is defined in
`parser.yy`, which is a GNU Bison source file that gets converted
into a C++ file named `parser.cc` that defines a function called
yyparse().

For each grammar rule in `parser.yy`, there is a code segment in
curly braces that builds a syntax tree. When the input program is
considered valid by this parser, the end result is a single syntax
tree whose subtrees are mostly function definitions and global
variable declarations.

The main() function of the compiler calls yyparse(). If any errors
occurred, they have been counted by the global integer `numErrors`,
defined in parser.yy. If that number is zero, the compiler continues
with the subsequent phases.


### Semantic checking

This phase is implemented by TranslationUnit::checkSemantics(),
which is called by main() some time after having called yyparse()
successfully.

A major part of this phase is implemented by the SemanticsChecker
class, which is functor that gets called on most nodes of the
syntax tree. In particular, this triggers calls to the checkSemantics()
method of each defined FunctionDef object, i.e., on each C function
defined in the input program.

The main job of FunctionDef::checkSemantics() is to call the
ExpressionTypeSetter functor on all nodes in the function's body.
This functor examines each expression and sub-expression and
determines its type.

A type (like `char`, `int`, a struct, a pointer to another type, etc.)
is represented by a `TypeDesc` object.

#### Type representation

`BasicType` is an enumeration that represents the elementary types
supported by the compiler: `VOID_TYPE`, `BYTE_TYPE`, `WORD_TYPE`,
`POINTER_TYPE`, `ARRAY_TYPE`, `CLASS_TYPE` and the internal
`SIZELESS_TYPE`.

The `TypeDesc` class represents more complex types. It contains
a BasicType, a boolean indicating if the type is signed or unsigned
(when applicable), the pointed type (when the type itself is pointer),
the class name (when the type is a struct), and a boolean indicating
if the struct is actually a union. (A union is just a struct whose
members are all at offset zero.)

A pointer to a signed int is represented by two TypeDesc objects.
The first one represents the signed int type, using the `WORD_TYPE`
as its basic type. The second one uses `POINTER_TYPE` as its basic
type and points to the first TypeDesc as its pointed type.

The `TypeManager` class only holds one instance of TypeDesc to represent
a certain type. This applies to all basic C types (signed int,
unsigned int, signed char, unsigned char and void).

`TypeManager` is the only class that is allowed to create TypeDesc
objects. The rest of the compiler calls public methods of TypeManager
to obtain a pointer to the TypeDesc object that corresponds to the
type described.

For example, `getIntType(WORD_TYPE, true)` returns the instance
of TypeDesc that corresponds to `int`. To obtain a TypeDesc
representing `int *`, one would pass the previous TypeDesc pointer
to getPointerTo().

The `TypeManager` class also registers and handles typedefs.

To obtain the size in bytes of a type represented by a TypeDesc,
call `TranslationUnit::getTypeSize()`.


##### Classes

A TypeDesc that represents a struct (`CLASS_TYPE`) only holds the name
of the class. The contents of the class are described by a `ClassDef`
object. Method `TranslationUnit::getClassDef()` is the one to call,
with a struct name, to obtain the corresponding `ClassDef`. Each member
of a struct is described by class `ClassMember`, which is local to
`ClassDef`.

Since version 0.1.40, structs can be passed by value to a function
and a function can return a struct by value. The number of bytes passed
in the stack is the exact size in bytes of the struct, except for 1-byte
structs. In this case, a padding byte is added to enforce the rule that
1-byte arguments are always passes as two bytes.

C requires that an argument of a size smaller than `int` be promoted
to `int` before being passed. That is why `char` types are passed as
two bytes. If a function has a variable number of parameters (using the
`...` notation), that function uses a `va_arg()` macro to iterate through
its parameters (e.g., `va_arg(ap, int)`.  That macro's implementation
code only has access to the size of the requested parameter. This
means that in the cases of `va_arg(ap, char)` and `va_arg(ap, OneByteStruct)`,
there is no way for the implementation to determine if the parameter
is a character or a struct. Consequently, both types must be passed
to a function the same way, i.e., with a padding byte.


##### Real numbers (float and double)

Floating-point types `float` and `double` are represented as structs
which are respectively named `_Float` and `_Double`.

As of version 0.1.40 of the compiler, their sizes are hardcoded at
5 and 8 bytes (see `TypeManager::createInternalStructs()`). However,
the `double` keyword is actually parsed as an alias for `float` (see
terminals `FLOAT` and `DOUBLE` in `parser.yy`). In a future version
of the compiler that would come with an 8-byte floating-point library,
the `double` keyword would have to start designating `struct _Double`.

When code is emitted for an operation that involves `float` or `double`
(i.e., single- and double-precision types), the compiler emits calls
to assembly-language routines that implement the operation (e.g.,
`addSingleSingle`). As of version 0.1.40, these routines invoke Color
Basic routines that work on a 5-byte packed floating-point format. This
is why floating-point support is only available when targeting the Disk
Basic environment, and not the OS-9 or Vectrex platforms.

To support other platforms, a suitable floating-point library would
have to be available. The floating-point format would not have to be
the current 5-byte format, but the compiler would have to be adjusted
to allow for platform-dependent format sizes.
Using such a library, an equivalent of `float-ecb.inc` would have to
be coded and named according to the new platform (e.g., `float-os9.inc`).
Then `stdlib.inc` would need to \#include the new file in a way similar
to how `float-ecb.inc` is \#included.


##### 32-bit integers (long)

32-bit types `long`, both signed and unsigned, are represented as
structs which are respectively named `_Long` and `_ULong`.

Struct `_Long` is the only structure that is considered signed.

The utility routines (e.g. `initDWordFromUnsignedWord`, etc.)
are available to all targets supported by the compiler.


### Code generation

This phase creates an ASMText object (in the main() function)
and passes it to TranslationUnit::emitAssembler(), which writes
6809 instructions and assembler directives (e.g., `ORG`) to the
ASMText object.

An ASMText object is a vector of "elements," each element being an
instruction, a label, an inline assembly block, a text comment, a
separator comment, an include directive, a C function start marker
or a C function end marker.

Most classes that are part of the syntax tree have an emitCode()
method. It receives a reference to an ASMText object and a boolean
that indicates if the method must evaluate an _l-value_ or not
(an _r-value_).

An l-value is a value that is suitable to be used at the left of
an assignment operator, hence the _l_. An l-value must have an
address, and it is this address that must be computed, and left
in the X register.

If an r-value must be computed, it must be left in the D register
(if the type of the expression has 16 bits) or in B (in the 8-bit
case). An r-value does not have to have an address. Numerical
constants like 42 for example have no address and can only be used
as r-values, i.e., on the right side of assignments.

When emitting an assignment like `foo = bar`, the emitCode()
method will be called with the `lValue` parameter equal to false
in the case of `bar`, then equal to true in the case of `foo`. In
the general case, after the first call to emitCode(), a `PSHS B,A`
instruction is emitted to preserve the value of `bar`. Then the
second call to emitCode() will evaluate the address of `foo` and
leave it in register X. In the general case, this call is free to use
D for intermediate values, hence the PSHS instruction just before.
After this second call, the compiler emits instructions that pop
D off the stack (the value of `bar`) and store it at the address
contained in X (the address of `foo`).

The following pseudo-code summarizes this process:

    [evaluate right side of assignment in D]
    PSHS B,A
    [evaluate address of destination in X, possibly trashing D]
    PULS A,B
    STD ,X

#### Utility routines

The `crt` library contains several utility routines (e.g., DIV16)
that only exist to be called by code generated by CMOC.
To add a new utility routine, named _foobar_ for example, do this:

* Create a file called `foobar.asm` in `src/stdlib`.
* Start the file with `SECTION code` (indented).
* Add a line that says `foobar EXPORT` (not indented).
* Add a line that defines the `foobar` label, then write the code
  of the utility routine. Make sure it ends with an `RTS` instruction,
  or equivalent.
* End the file with `ENDSECTION` (indented).
* Add the name `foobar.asm` to the `CRT_ASM` list variable in
  `src/stdlib/Makefile.am`.

To invoke the utility routine from the CMOC code generation phase,
issue the statement `callUtility(out, "foobar")`, where `out` is
a reference to the `ASMText` object where the invocation is to be
emitted.

Giving the `make` command will trigger the regeneration of
`src/stdlib/Makefile`, then compile the routine and the code
that invokes it.


### Low-level optimization

After having called TranslationUnit::emitAssembler(), the main()
function calls ASMText::peepholeOptimize(), unless the user has
requested no optimizations. This method applies a series of ad
hoc optimizations directly on the assembly code, with the aim of
improving naive instruction sequences, therefore making the program
shorter and/or faster.

After this phase, the main() function tells the ASMText object
to write its contents to an actual .asm file on disk.


Self tests
----------

### Run-time unit tests

The Perl script `test-program-output.pl` contains many small C programs
that run assert statements and write results to standard output.
Each of these programs is a unit test that is run by the USim 6809
simulator. No assert is expected to fail. The output of a program
is checked against the expected output.

Each unit test is a reference to a hash that contains a single-line
_title_ field, a multi-line _program_ field and an _expected_ field
that indicates what output is expected of the program through its
printf() statements, if any.  The _expected_ field uses \n to
represent a newline.

An optional _compilerOptions_ field can be used to pass additional
options to the compiler.

The `--help` option lists several command-line switches. The most
useful during development are `--last`, `--only=`_X_ and `--stop`.


### Error and warning unit tests

The Perl script `test-bad-programs.pl` is similar to
`test-program-output.pl`, but its goal is not to run programs, it is
to check that the compiler issues the error and warning messages that
are expected when making several types of mistakes in a C program.

The _expected_ field of a unit test in this script is a reference
to an array of compiler messages. In these messages, the `error` and
`warning` keywords are preceded and followed by two underscores so
as to avoid having these lines be picked up by an automatic build
system's error detection facilities.


OS-9
----

An OS-9 process receives a pointer to its data segment in the U
register, the address of the top of the data segment in Y and
the address of the command line in X. This command line
ends with character $0D.

TranslationUnit::emitAssembler() emits instructions that make use
of those registers.

The very first thing that a CMOC-generated program does under OS-9
is to call routine named `OS9PREP`, which is defined in stdlib/crt.asm.
This routine:

1. Exchanges U and Y, so that Y points to the data segment, as per
   the CMOC convention for OS-9. At the end of this routine, the code
   will use U as the stack frame register.
1. Copies the initial values of the writable data to the data segment
   designated by Y.
   * Those initial values are read from memory that immediately follows 
     the code. (The sections that get copied are named `rwdata` and `bss`. 
     The link script prepared by createLinkScript() puts these two
     sections at the end of the executable file.)
   * This copy allows several simultaneous instances of the same
     program to start with their own independent copy of the program's
     global variables.
1. Parses the command-line.

### Global variables

A program's *writable* global variables are assigned offsets that start at zero.
(This is the purpose of the `load 0` that appears on the `section rwdata`
line of the OS-9 link script.) At run time, those offsets are relative
to the start of the data segment, designated by Y. Therefore, all
references to writable globals are made by instruction arguments of
the form `_someVariable,Y`.

*Read-only* globals, as well as string, long and floating point literals,
are handled differently. Because the process is assumed to refrain from
changing them, they are positioned after the code and are not copied
to the data segment designated by Y. Because that data is next to the
code, all references to it are made by instruction arguments of the form
`_someReadOnlyData,PCR`.

### No reference patching

Unlike the OS-9 C compiler, CMOC does not have to do any patching of
_data-text_ and _data-data_ references, because globals are initialized
dynamically, not statically. CMOC generates global variable
initialization routines in the `initgl` section. That code is
executed right before main(): TranslationUnit::emitAssembler() emits
a call to `INILIB` (which is defined in crt.asm) and that routine
branches to `INITGL`, which is defined by section `initgl_start`.

The link script places the `start` section, and there the `program_start`
symbol, at address $000D because the first 13 bytes of the code segment
are reserved.

### Command line parsing

The command line passed to the process by the OS-9 shell is parsed by
`parseCmdLine`, a routine defined in crt.asm that is part of `OS9PREP`.

`OS9PREP` returns the number of arguments in D
and the address of the array of argument strings is returned in X.
After the call to `OS9PREP`, this array of pointers is passed to main()
as standard arguments `argc` and `argv`.

An arbitrary maximum is set by the CMOC library on the number of
arguments that are passed to main(). This maximum is set by `maxargc`
in stdlib/std.inc. The command line parser fills an array called `argv`,
which is defined in stdlib/crt.asm. This array contains two bytes per
argument (maximum `maxargc`), plus 2 more bytes to contain the null pointer
that marks the end of the C array argv[].

argv[0] is an empty string. In theory, it would represent the name of
the current process. argv[1] is the first actual argument received by
the process. For example, `prog foo bar` will pass `foo` as argv[1]
and `bar` as argv[2]. argv[argc] is a null pointer.

### Low-Level Optimizer

The low-level optimizer implemented in class ASMText must be careful
to avoid changing register Y when the compilation targets OS-9.

### Compiling the CMOC library

`Makefile.am` in `stdlib` uses Perl script `os9fixup.pl` to change any
lower-case `,pcr` addressing suffix to `,Y`, because by CMOC convention,
these are writable data references. References to code and to read-only
data must be made with an upper-case `,PCR` suffix. On other platforms,
Y is not used as a data register and all data references are PC-relative.


Documentation
-------------

The documentation is written in Markdown format in files found in
the `doc` subdirectory. This subdirectory contains a Perl script
named `toc.pl` that adds a table of contents to the HTML produced
by the `markdown` command from a Markdown source file. That Perl
script is run by the makefile in the `doc` subdirectory.


License
-------

See the CMOC Manual.


</body>
