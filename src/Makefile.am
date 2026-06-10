# The dot forces CMOC to be compiled before processing the stdlib and float
# subdirectories, whose processing requires CMOC itself.
#
# On Windows (MinGW/MSYS2), exclude the usim 6809 simulator: it requires
# termios and is only needed for the self-tests.
#
if WINDOWS_BUILD
SUBDIRS = . stdlib float
else
SUBDIRS = usim-0.91-cmoc . stdlib float
endif

bin_SCRIPTS = install-coco-boot-loader

bin_PROGRAMS = cmoc

if BUILD_WRITECOCOFILE
bin_PROGRAMS += writecocofile
endif

cmoc_SOURCES = \
	TypeDesc.cpp \
	TypeDesc.h \
	TypeManager.cpp \
	TypeManager.h \
	DeclarationSpecifierList.cpp \
	DeclarationSpecifierList.h \
	util.cpp \
	util.h \
	Tree.cpp \
	Tree.h \
	ASMText.cpp \
	ASMText2.cpp \
	ASMText.h \
	Pseudo6809.cpp \
	Pseudo6809.h \
	parser.yy \
	lexer.ll \
	Declarator.cpp \
	Declarator.h \
	DeclarationSequence.cpp \
	DeclarationSequence.h \
	Declaration.cpp \
	Declaration.h \
	InitializerCodeEmitter.cpp \
	InitializerCodeEmitter.h \
	BinaryOpExpr.cpp \
	BinaryOpExpr.h \
	UnaryOpExpr.cpp \
	UnaryOpExpr.h \
	CastExpr.cpp \
	CastExpr.h \
	IfStmt.cpp \
	IfStmt.h \
	SwitchStmt.cpp \
	SwitchStmt.h \
	LabeledStmt.cpp \
	LabeledStmt.h \
	WhileStmt.cpp \
	WhileStmt.h \
	ForStmt.cpp \
	ForStmt.h \
	JumpStmt.cpp \
	JumpStmt.h \
	StringLiteralExpr.cpp \
	StringLiteralExpr.h \
	Scope.cpp \
	Scope.h \
	FunctionDef.cpp \
	FunctionDef.h \
	SemanticsChecker.cpp \
	SemanticsChecker.h \
	AncestorFunctor.cpp \
	AncestorFunctor.h \
	ExpressionTypeSetter.cpp \
	ExpressionTypeSetter.h \
	TranslationUnit.cpp \
	TranslationUnit.h \
	TreeSequence.cpp \
	TreeSequence.h \
	CommaExpr.cpp \
	CommaExpr.h \
	FunctionCallExpr.cpp \
	FunctionCallExpr.h \
	IdentifierExpr.cpp \
	IdentifierExpr.h \
	VariableExpr.cpp \
	VariableExpr.h \
	WordConstantExpr.cpp \
	WordConstantExpr.h \
	DWordConstantExpr.cpp \
	DWordConstantExpr.h \
	RealConstantExpr.cpp \
	RealConstantExpr.h \
	CompoundStmt.h \
	CodeStatus.cpp \
	CodeStatus.h \
	FormalParameter.cpp \
	FormalParameter.h \
	FormalParamList.cpp \
	FormalParamList.h \
	FunctionPointerCast.h \
	AssemblerStmt.cpp \
	AssemblerStmt.h \
	ObjectMemberExpr.cpp \
	ObjectMemberExpr.h \
	ClassDef.cpp \
	ClassDef.h \
	ConditionalExpr.cpp \
	ConditionalExpr.h \
	ScopeCreator.cpp \
	ScopeCreator.h \
	Pragma.cpp \
	Pragma.h \
	Parameters.cpp \
	Parameters.h \
	main.cpp

AM_YFLAGS = -d -Wno-conflicts-sr -Werror
# -d produces a .h file
# -Wno-conflicts-sr silences a shift/reduce warning.

# W_SUGGEST_OVERRIDE is -Wsuggest-override if configure.ac saw the compiler accept that option.
# W_ERROR is normally empty. Passing W_ERROR=-Werror to make launches a build that errors
# on warnings.
#
cmoc_CXXFLAGS = -DPKGDATADIR=\"$(pkgdatadir)\" $(W_SUGGEST_OVERRIDE) $(W_ERROR)


if BUILD_WRITECOCOFILE

writecocofile_SOURCES = writecocofile.cpp

endif


LOCAL_CMOC = $(ENV) PATH="$(srcdir):$$PATH" CMOCFLOATLIBDIR=float ./cmoc -I $(srcdir)/stdlib/ -Lstdlib -Lfloat

# Pass STOPONFAIL="--stop --nocleanup" on the command-line
# to force the unit testing to stop at the first failed test.
# Example: make STOPONFAIL="--stop --nocleanup" check
# The --nocleanup option avoids deleting the tested program
# (,check-prog.c) and the .s, etc. files after a failure.
#
STOPONFAIL =

# We use perl -e 'print "foo\n"' to print a string that uses escape sequences (e.g., \n)
# because the system's echo command may not support echo -n -e (especially -e).
#
PERLEXEC = $(PERL) -e

CREATE_TEST_PROG = $(PERLEXEC) 'print "int main() { return 0; }\n"' >,check-prog.c

USIM = usim-0.91-cmoc/usim
HD = $(HEXDUMP) -C
MKDIR = mkdir
RMDIR = rmdir

check-base:
	./cmoc --version >/dev/null
	./cmoc --version | $(GREP) -q "cmoc 0"
	./cmoc --help >/dev/null
	./cmoc --help | $(PERL) -ne '/GNU General Public License/ and $$ok=1; END { exit !$$ok }'
	./cmoc --help | $(PERL) -ne '/default: [0-9A-F]{4}/ and $$ok=1; END { exit !$$ok }'  # check default code address
	./cmoc --help | $(PERL) -ne '/Compiler data directory: .*?\/cmoc/ and $$ok=1; END { exit !$$ok }'
	rm -f ,check-prog.c ,check-prog.s ,check-prog.asm ,check-prog.i ,check-prog.lst ,check-prog.hex ,check-prog.bin
	$(PERLEXEC) 'print "char g; int main() { return 0; }\n"' >,check-prog.c
	$(LOCAL_CMOC) -o ,check-prog.xyz ,check-prog.c
	test ! -f ,check-prog.bin  # fail if -o not applied
	rm ,check-prog.xyz  # fail if file not there
	$(LOCAL_CMOC) --lwasm=`which lwasm` --lwlink=`which lwlink` ,check-prog.c
	rm ,check-prog.bin  # fail if file not there

check-srec:
	$(CREATE_TEST_PROG)
	$(LOCAL_CMOC) --srec --verbose ,check-prog.c | $(GREP) ",check-prog.srec"
	$(GREP) -q ^S1 ,check-prog.srec  # data record
	test `$(GREP) -c ^S9 ,check-prog.srec` = 1  # starting address record (only one)
	test `$(GREP) -c ^: ,check-prog.srec` = 0  # not Intel HEX format

# Checks --raw.
# Check that a raw format executable for DECB starts with LDD #-1024 and an LBSR instruction,
# as emitted by TranslationUnit::emitAssembler().
#
check-raw:
	$(CREATE_TEST_PROG)
	$(LOCAL_CMOC) --raw --verbose ,check-prog.c | $(GREP) ",check-prog.raw"
	$(HD) ",check-prog.raw" | $(HEAD) -1 | $(GREP) -q '^00000000  cc fc 00 17'

# Checks --org, --data.
#
check-org:
	$(CREATE_TEST_PROG)
	$(LOCAL_CMOC) --org=4321 --data=6000 --intermediate --verbose ,check-prog.c 2>&1
	$(GREP) -q "Symbol: program_start (,check-prog.o) = 4321" ,check-prog.map
	$(GREP) -q "Section: rwdata .* load at 6000," ,check-prog.map
	rm -f ,check-prog.*

# Checks --limit.
# Checks that no executable is left when --limit fails.
#
check-limit:
	$(CREATE_TEST_PROG)
	rm -f ,check-prog.bin
	$(LOCAL_CMOC) --org=2800 --limit=2810 ,check-prog.c 2>&1 | $(GREP) -q "program_end exceeds it"
	test ! -e ,check-prog.bin
	rm -f ,check-prog.*

# Checks --dos.
# Checks that the resulting binary is in BIN format, that it starts
# with a header that positions the contents at $2600, and that the
# first two bytes are "OS".
#
check-dos:
	$(CREATE_TEST_PROG)
	$(LOCAL_CMOC) --dos ,check-prog.c
	$(HD) ,check-prog.bin | $(HEAD) -1 | $(GREP) -q '^00000000  00 .. .. 26 00 4f 53'
	rm -f ,check-prog.*

check-long: check-long.srec
	$(USIM) check-long.srec | $(PERL) -pe '/\bSUCCESS\b/ and $$s = 1; END { exit !$$s }'
	rm -f check-long.{s,lst,o,map,link,srec}

check-long.srec: $(srcdir)/check-long.c
	$(LOCAL_CMOC) --usim --org=1600 --limit=F600 -DPART0 -DPART1 -DPART2 -O2 --no-relocate -w --intermediate $(srcdir)/check-long.c

check-float: check-coco-float check-dragon-float
	$(PERLEXEC) 'print "int main() { return 18.44f == 0; }\n"' >,check-prog.c
	$(LOCAL_CMOC) --decb -c --intermediate -O0 ,check-prog.c
	$(GREP) -q 'FCB.$$85,$$13,$$85,$$1E,$$B8$$' ,check-prog.s
	$(LOCAL_CMOC) --dragon -c --intermediate -O0 ,check-prog.c
	$(GREP) -q 'FCB.$$85,$$13,$$85,$$1E,$$B8$$' ,check-prog.s
	rm -f ,check-prog.*

check-os9: check-array-section check-add-os9-stack-space check-os9-no-float check-os9-isr
	$(CREATE_TEST_PROG)
	$(LOCAL_CMOC) --os9 --intermediate -nodefaultlibs ,check-prog.c  # works b/c main() calls nothing
	$(LOCAL_CMOC) --os9 --intermediate ,check-prog.c
	rm ,check-prog
	$(GREP) -q OS9PREP ,check-prog.s
	$(GREP) -q OS9PREP ,check-prog.lst
	$(GREP) -q OS9PREP ,check-prog.map
	$(GREP) -q 'section start load d' ,check-prog.link
	$(PERLEXEC) 'print "void putchar(int c); int main() { putchar(42); return 0; }\n"' >,check-prog.c
	$(LOCAL_CMOC) --os9 --intermediate -nodefaultlibs ,check-prog.c | grep -q '_putchar not found'
	rm -f ,check-prog.s
	$(LOCAL_CMOC) --os9 -i ,check-prog.c
	$(GREP) -q _stkcheck ,check-prog.s
	$(LOCAL_CMOC) --os9 --intermediate --function-stack=9876 ,check-prog.c
	$(GREP) -q -w 9876 ,check-prog.s
	$(LOCAL_CMOC) --os9 --intermediate --function-stack=0 ,check-prog.c
	test `$(GREP) -c _stkcheck ,check-prog.s` = 0
	rm -f ,check-prog.* ,check-prog

# Check OS-9 option --add-os9-stack-space.
# The expected "01 50" is the word affected by that option.
# It is the Permanent Storage Size, at offset 11 of the executable.
# The expected number can be affected when the size of the BSS section changes.
#
check-add-os9-stack-space:
	$(CREATE_TEST_PROG)
	$(LOCAL_CMOC) --os9 --intermediate --add-os9-stack-space=300 ,check-prog.c
	$(HD) ,check-prog | $(EGREP) -q '^00000000  .* 00 0d 01 51 '
	rm -f ,check-prog.* ,check-prog

# Check that an OS-9 executable does not contain the float constructor.
#
check-os9-no-float:
	$(CREATE_TEST_PROG)
	$(LOCAL_CMOC) --os9 --intermediate ,check-prog.c
	test `$(GREP) -c float ,check-prog.map` = 0
	rm -f ,check-prog.* ,check-prog

# Check that an signal handler can be implemented for OS-9.
# The _CMOC_fpir_ calling convention makes the function receive 's' in register B.
# We check that B gets "spilled" in the activation record's local variable space,
# i.e., STB -1,U in this case.
#
check-os9-isr:
	$(PERLEXEC) 'print "interrupt _CMOC_fpir_ void isr(unsigned char s) {}\n"' >,check-prog.c
	$(LOCAL_CMOC) -c --os9 --intermediate ,check-prog.c
	$(PERL) -ne '/^\s+STB\s+-\d+,U/ and $$f=1; END {exit 1-$$f}' ,check-prog.s
	rm -f ,check-prog.* ,check-prog

# Check that a DECB executable contain the float constructor.
#
check-coco-float:
	$(CREATE_TEST_PROG)
	$(LOCAL_CMOC) --coco --intermediate ,check-prog.c
	test `$(GREP) -c float ,check-prog.map` = 2
	rm -f ,check-prog.*

# Check that a Dragon executable contain the float constructor.
#
check-dragon-float:
	$(CREATE_TEST_PROG)
	$(LOCAL_CMOC) --dragon --intermediate ,check-prog.c
	test `$(GREP) -c float ,check-prog.map` = 2
	rm -f ,check-prog.*

# Check that the right Dragon file type is created when using --dragon
# with --raw or --srec or neither.
#
check-dragon:
	rm -f ,check-prog.* ,check-prog
	$(CREATE_TEST_PROG)
	$(LOCAL_CMOC) --dragon ,check-prog.c
	rm ,check-prog.bin
	$(LOCAL_CMOC) --dragon --raw ,check-prog.c
	rm ,check-prog.raw
	$(LOCAL_CMOC) --dragon --srec ,check-prog.c
	rm ,check-prog.srec
	rm -f ,check-prog.* ,check-prog

# Checks that both the C file that defines a constant array and the C file that
# imports it via "extern" see the array as part of the read-only data section.
# Under OS-9, this means using ,PCR to refer to the array, while ,Y is used to
# refer to writable data.
#
check-array-section:
	$(PERLEXEC) 'print "const int ro[] = { 1000 }; int w[] = { 2000 };\n"' >,array.c
	$(PERLEXEC) 'print "extern const int ro[]; extern int w[]; int main() { return ro[0] + w[0]; }\n"' >,main.c
	$(LOCAL_CMOC) --os9 -O0 --intermediate ,main.c ,array.c
	$(EGREP) -q 'LDD.*_ro\+0,PCR' ,main.s
	$(EGREP) -q 'LDD.*_w\+0,Y' ,main.s
	rm -f ,main ,main.* ,array.*

check-flex:
	$(CREATE_TEST_PROG)
	$(LOCAL_CMOC) --flex --intermediate -nodefaultlibs ,check-prog.c  # works b/c main() calls nothing
	$(LOCAL_CMOC) --flex --intermediate ,check-prog.c
	rm ,check-prog.cmd
	$(GREP) -q -w FLEXCL ,check-prog.map  # FLEX-specific command line parsing routine
	rm -f ,check-prog.*

check-deps:
	$(CREATE_TEST_PROG)
	$(LOCAL_CMOC) -c ,check-prog.c
	test ! -f ,check-prog.d 
	$(LOCAL_CMOC) -c --deps ,check-prog.c
	$(GREP) -q '\.d : ' ,check-prog.d
	test -f ,check-prog.o 
	rm -f ,check-prog.d ,check-prog.o
	$(LOCAL_CMOC) --deps-only ,check-prog.c
	$(GREP) -q '\.d : ' ,check-prog.d
	test ! -f ,check-prog.o 
	rm -f ,check-prog.*

# Checks that the compiler exits with a failure code when the preprocessor
# used by --deps-only fails.
#
check-deps-only-exit:
	$(PERLEXEC) 'print "#error FOO\n"' >,check-prog.c
	$(LOCAL_CMOC) --deps-only ,check-prog.c 2>&1 | $(GREP) -q "err..: #err.. FOO"
	rm -f ,check-prog.*

check-only-deps-arg:
	mkdir ,subdir
	$(CREATE_TEST_PROG)
	mv ,check-prog.c ,subdir
	$(LOCAL_CMOC) --deps-only=,subdir/,other-name.d ,subdir/,check-prog.c
	test ! -f ,other-name.d
	$(GREP) -q ',check-prog.o ,subdir/,other-name.d : ,subdir/,check-prog.c' ,subdir/,other-name.d
	rm ,subdir/,other-name.d ,subdir/,check-prog.c
	rmdir ,subdir
	mkdir ,subdir
	$(CREATE_TEST_PROG)
	mv ,check-prog.c ,subdir
	$(LOCAL_CMOC) --deps-only=,subdir/,other-name.d -o ,subdir/,check-prog.o ,subdir/,check-prog.c
	test ! -f ,other-name.d
	test ! -f ,other-name.o
	$(GREP) -q ',subdir/,check-prog.o ,subdir/,other-name.d : ,subdir/,check-prog.c' ,subdir/,other-name.d
	rm ,subdir/,other-name.d ,subdir/,check-prog.c
	rmdir ,subdir

check-vectrex:
	$(PERLEXEC) 'print "#include <vectrex/bios.h>\n#include <vectrex/stdlib.h>\nint main() { move(0, 0); wait_retrace(); return 0; }\n"' >,check-prog.c
	rm -f ,check-prog.bin
	$(LOCAL_CMOC) --vectrex --intermediate ,check-prog.c
	test -f ,check-prog.bin
	$(STRINGS) ,check-prog.bin | $(GREP) -q 'g GCE 2015'
	$(GREP) -q -w '^_move' ,check-prog.s
	$(GREP) -q -w '^_wait_retrace' ,check-prog.s

check-void-target:
	$(PERLEXEC) 'print "int main() { return 0; }\n"' >,check-prog.c
	rm -f ,check-prog.srec ,check-prog.bin
	$(LOCAL_CMOC) --void-target ,check-prog.c
	test -f ,check-prog.srec
	test ! -f ,check-prog.bin

check-intdir:
	rm -f ,check-prog.{s,lst,o,map,link,bin} ,check-prog-out.{map,link,bin}
	$(CREATE_TEST_PROG)
	$(LOCAL_CMOC) -i -c ,check-prog.c
	test -f ,check-prog.s
	test -f ,check-prog.lst
	test -f ,check-prog.o
	rm ,check-prog.{s,lst,o}
	$(LOCAL_CMOC) -i ,check-prog.c
	test -f ,check-prog.s
	test -f ,check-prog.lst
	test -f ,check-prog.o
	test -f ,check-prog.map
	test -f ,check-prog.link
	test -f ,check-prog.bin
	rm ,check-prog.{s,lst,o,map,link,bin}
	$(MKDIR) ,int
	$(LOCAL_CMOC) -i -c --intdir=,int ,check-prog.c
	test -f ,int/,check-prog.s
	test -f ,int/,check-prog.lst
	test -f ,int/,check-prog.o
	rm ,int/,check-prog.{s,lst,o}
	$(LOCAL_CMOC) -i -c --intdir=,int -o ,check-prog.o ,check-prog.c
	test -f ,int/,check-prog.s
	test -f ,int/,check-prog.lst
	test -f ,check-prog.o
	rm ,int/,check-prog.{s,lst} ,check-prog.o
	$(LOCAL_CMOC) -i -c --intdir=,int -o ./,check-prog.o ,check-prog.c
	test -f ,int/,check-prog.s
	test -f ,int/,check-prog.lst
	test -f ,check-prog.o
	rm ,int/,check-prog.{s,lst} ,check-prog.o
	$(LOCAL_CMOC) -i --intdir=,int -o ./,check-prog-out.bin ,check-prog.c
	test -f ,int/,check-prog.s
	test -f ,int/,check-prog.lst
	test -f ,int/,check-prog.o
	test -f ,check-prog-out.map
	test -f ,check-prog-out.link
	test -f ,check-prog-out.bin
	rm ,int/,check-prog.{s,lst,o}
	rm ,check-prog-out.{map,link,bin}
	$(RMDIR) ,int

# Check that 8x8 multiply uses MUL instruction, and that 16x8 uses MUL16 routine.
# 
check-mul:
	$(PERLEXEC) 'print "unsigned f(unsigned char a, unsigned char b) { return a * b; }\n"' >,check-prog.c
	$(LOCAL_CMOC) -c -i -O0 ,check-prog.c
	$(PERL) -ne '/^\s+MUL\b/ and $$f=1; END {exit !$$f}' ,check-prog.s

	$(PERLEXEC) 'print "unsigned f(unsigned char a, unsigned b) { return a * b; }\n"' >,check-prog.c
	$(LOCAL_CMOC) -c -i -O0 ,check-prog.c
	$(PERL) -ne '/^\s+LBSR\s+MUL16\b/ and $$f=1; END {exit !$$f}' ,check-prog.s

	$(PERLEXEC) 'print "unsigned f(unsigned a, unsigned char b) { return a * b; }\n"' >,check-prog.c
	$(LOCAL_CMOC) -c -i -O0 ,check-prog.c
	$(PERL) -ne '/^\s+LBSR\s+MUL16\b/ and $$f=1; END {exit !$$f}' ,check-prog.s

	rm -f ,check-prog.*

# Test the -Wlocal-var-hiding option.
# After calling cmoc, use a Perl command to invert cmoc's exit status: we want cmoc to fail.
#
check-local-var-hiding:
	echo 'void f() { int v; if (1) { int v; } }' >,check-prog.c
	$(LOCAL_CMOC) -c -Wlocal-var-hiding -Werror ,check-prog.c >/dev/null; $(PERLEXEC) 'exit !$$ARGV[0]' $$?
	rm -f ,check-prog.*


# Check that multiple definitions of an external linkage function or variable gives an error.
# The Perl statements check that the compiler output has 2 lines and that they contain the right error messages.
#
check-mult-def:
	echo 'void externFunc(void) {}; int externVar; int main() { return 0; }' > ,check-prog-1.c
	echo 'void externFunc(void) {}; int externVar;' > ,check-prog-2.c
	$(LOCAL_CMOC) ,check-prog-1.c ,check-prog-2.c > ,check-prog-out.txt || true
	$(PERLEXEC) '@a = <>; die 1 if @a != 2; die 2 unless $$a[0] =~ / error: multiple definitions of symbol _externFunc /' ,check-prog-out.txt
	$(PERLEXEC) '@a = <>; die 2 unless $$a[1] =~ / error: multiple definitions of symbol _externVar /' ,check-prog-out.txt
	rm -f ,check-prog*.*


# In the static case, the multiple definitions are not an error.
# The code contains calls to the static functions to avoid the "not called" warning.
#
check-mult-static-symbols:
	echo 'static void staticFunc(void) {}; static int staticVar; void f1() { staticFunc(); } int main() { return 0; }' > ,check-prog-1.c
	echo 'static void staticFunc(void) {}; static int staticVar; void f2() { staticFunc(); }' > ,check-prog-2.c
	$(LOCAL_CMOC) ,check-prog-1.c ,check-prog-2.c > ,check-prog-out.txt
	$(PERLEXEC) '@a = <>; die 1 if @a != 0' ,check-prog-out.txt
	rm -f ,check-prog*.*


check-misc: check-base check-srec check-raw check-org check-limit check-long check-float check-os9 check-flex \
				check-deps check-deps-only-exit check-only-deps-arg check-vectrex check-stack-space check-bad-stack-space-option check-linking check-nbsp \
				check-local-var-hiding check-void-target check-call-graph check-func-import check-no-hole \
				check-no-hidden-bool-for-global check-dash-x check-dash-E check-dash-w check-kandr-prototype check-local-prototype \
				check-assert check-assert-handler check-initial-s check-leas-optim
	rm -f ,check-prog.*

check-program-output:
	$(PERL) -cw $(srcdir)/test-program-output.pl
	$(PERL) $(srcdir)/test-program-output.pl --opt=0 $(STOPONFAIL) $(srcdir)
	$(PERL) $(srcdir)/test-program-output.pl $(STOPONFAIL) $(srcdir)
	$(PERL) $(srcdir)/test-program-output.pl --opt=0 --omit-frame-pointer $(STOPONFAIL) $(srcdir)
	$(PERL) $(srcdir)/test-program-output.pl --omit-frame-pointer $(STOPONFAIL) $(srcdir)
	$(PERL) $(srcdir)/test-program-output.pl --opt=0 --no-omit-frame-pointer $(STOPONFAIL) $(srcdir)
	$(PERL) $(srcdir)/test-program-output.pl --no-omit-frame-pointer $(STOPONFAIL) $(srcdir)
	$(PERL) $(srcdir)/test-program-output.pl --opt=0 --unsigned-char $(STOPONFAIL) $(srcdir)
	$(PERL) $(srcdir)/test-program-output.pl --unsigned-char $(STOPONFAIL) $(srcdir)
	$(PERL) $(srcdir)/test-program-output.pl --no-relocate $(STOPONFAIL) $(srcdir)

check-bad-programs:
	$(PERL) -cw $(srcdir)/test-bad-programs.pl
	$(PERL) $(srcdir)/test-bad-programs.pl $(STOPONFAIL) $(srcdir)

# Stack space must be specified when compiling the .c file that defines main().
# If the stack space is made larger than the default 1k (with --stack-space or #pragma stack_space),
# then sbrkmax() is expected to report a smaller quantity of allocatable memory.
# --org=1 is used because as of CMOC 0.1.82, USim refuses to execute code
# at address 0, to help catch stray code.
#
check-stack-space:
	$(PERLEXEC) 'print qq[#include <cmoc.h>\nint main() { printf("\%u\\n", sbrkmax()); return 0; }\n]' >,check-prog.c
	$(LOCAL_CMOC) --usim -c -I $(srcdir)/stdlib ,check-prog.c
	$(LOCAL_CMOC) --usim --org=1 ,check-prog.o
	test `$(USIM) ,check-prog.srec` -ge 60000
	$(LOCAL_CMOC) --usim -c -I $(srcdir)/stdlib --stack-space=30000 ,check-prog.c
	$(LOCAL_CMOC) --usim --org=1 ,check-prog.o
	test `$(USIM) ,check-prog.srec` -lt 34000
	$(PERLEXEC) 'print qq[#pragma stack_space 15000\n#include <cmoc.h>\nint main() { printf("\%u\\n", sbrkmax()); return 0; }\n]' >,check-prog.c
	$(LOCAL_CMOC) --usim -c -I $(srcdir)/stdlib ,check-prog.c
	$(LOCAL_CMOC) --usim --org=1 ,check-prog.o
	test `$(USIM) ,check-prog.srec` -ge 48000
	test `$(USIM) ,check-prog.srec` -lt 49000
	rm -f ,check-prog.c ,check-prog.o ,check-prog.srec

# Checks that the compiler fails with no argument is passed to option --stack-space=.
#
check-bad-stack-space-option:
	$(CREATE_TEST_PROG)
	$(LOCAL_CMOC) --usim -c -I $(srcdir)/stdlib --stack-space= ,check-prog.c 2>&1 | $(GREP) -q "cmoc: Invalid argument for --stack-space:"
	rm -f ,check-prog.*

check-linking:
	$(PERLEXEC) 'print "int main() { return 0; }\n"' > ,mod0.c
	echo "static void in() {}  void ex() { void (*pf)() = in; }" > ,mod1.c
	$(LOCAL_CMOC) ,mod0.c ,mod1.c
	rm -f ,mod0.c ,mod1.c ,mod0.bin

check-writecocofile:
if BUILD_WRITECOCOFILE
	./writecocofile --version >/dev/null
	./writecocofile --help >/dev/null
	$(PERL) -e 'print chr(255) x (35*18*256)' > ,check.dsk
	./writecocofile -d ,check.dsk | $(GREP) -q '68 granule.s. free'
	echo FOOBAR > ,check.dat
	./writecocofile --verbose ,check.dsk ,check.dat > ,check.log
	$(GREP) -q 'Writing native file ,check.dat' ,check.log
	./writecocofile -d ,check.dsk | $(GREP) -q '67 granule.s. free'
	./writecocofile -d ,check.dsk | $(GREP) -q ',CHECK  .DAT  data     binary'
	./writecocofile -s ,check.dsk ,check.dat | $(GREP) -q 'FOOBAR'
	./writecocofile --verbose -k ,check.dsk ,check.dat | $(GREP) -q 'Freeing granule'
	./writecocofile -d ,check.dsk | $(GREP) -q '68 granule.s. free'
	rm -f ,check.dsk ,check.dat ,check.log
endif

# This checks that an initializer that involves 'f' is seen as a use of f(),
# which avoids the warning about a static function not being called.
# This test compiles with -c and without a main() function.
#
check-call-graph:
	echo "static void f(void) {}  void (*pf)(void) = f;" > ,check-prog.c
	$(LOCAL_CMOC) --usim -c ,check-prog.c
	rm -f ,check-prog.*

# Checks if some files contain non-breakable spaces (character 160).
# The statement in the 'continue' clause resets the line counter ($.)
# at the end of each processed file.
#
check-nbsp:
	cd $(srcdir) && $(PERL) -e 'while (<>) { if (/\xA0/) { print "Non-breakable space at $$ARGV:$$.\n"; exit 1 } } continue { close ARGV if eof }' \
		$(cmoc_SOURCES) test-program-output.pl test-bad-programs.pl check-long.c $(bin_SCRIPTS) Makefile.am

# Check that a non-static function prototype generates an IMPORT directive
# in the asm file.
#
check-func-import:
	echo "int f(char); int main() { int (*pf)(char) = f; return pf(0); }" > ,check-prog.c
	$(LOCAL_CMOC) --usim -c ,check-prog.c
	rm -f ,check-prog.*

# Check that a DECB program that has something in its BSS section gives a .bin file
# containing single contiguous block.
# The Perl script reads the entire .bin file, gets the length of the first .bin block,
# and checks that this block is 10 bytes less than the whole file.
# A single-block .bin file has a 5-byte header, the contents, then the 5-byte footer.
# The dollar signs in the Perl script are doubled so that make passes a single
# dollar sign to the Perl command.
#
check-no-hole:
	echo "char bssArray[9]; int main() { return 0; }" > ,check-prog.c
	rm -f ,check-prog.bin
	$(LOCAL_CMOC) --coco ,check-prog.c
	$(PERL) -e '$$/ = undef; $$bin = <>; ($$z, $$blkLen) = unpack("Cn", $$bin); die unless $$blkLen == length($$bin) - 10' ,check-prog.bin
	rm -f ,check-prog.*


# Check that no ".initialized" hidden boolean is emitted for a global variable.
# Such a boolean is only emitted for some local static variables.
#
check-no-hidden-bool-for-global:
	echo "const char *str = \"foo\";" > ,check-prog.c
	rm -f ,check-prog.s ,check-prog.o ,check-prog.lst
	$(LOCAL_CMOC) -c -i ,check-prog.c
	test `$(GREP) -c _str.initialized ,check-prog.s` = 0
	rm -f ,check-prog.*

# Check that -E only generates the preprocessor output and does not generate an object file
# or an executable.
#
check-dash-E:
	rm -f ,check-prog.*
	$(CREATE_TEST_PROG)
	$(LOCAL_CMOC) -E ,check-prog.c >,check-prog.i
	$(GREP) -q ^# ,check-prog.i
	$(GREP) -q 'int main' ,check-prog.i
	test ! -f ,check-prog.bin
	test ! -f ,check-prog.o
	rm -f ,check-prog.*

# Check that a .c file is accepted by default, and with -x c and -x none.
# Check that a file with a non-standard extension (e.g., .foo) is only accepted with -x c.
#
check-dash-x:
	rm -f ,check-prog.c ,check-prog.foo ,check-prog.bin ,check-prog.err
	$(CREATE_TEST_PROG)
	$(LOCAL_CMOC) ,check-prog.c
	$(LOCAL_CMOC) -x none ,check-prog.c
	$(LOCAL_CMOC) -x c ,check-prog.c
	test -f ,check-prog.bin
	rm ,check-prog.bin
	mv ,check-prog.c ,check-prog.foo
	if $(LOCAL_CMOC) ,check-prog.foo 2>&1 > ,check-prog.err; then false; else true; fi
	$(GREP) -q "invalid input filename ,check-prog.foo" ,check-prog.err
	test ! -f ,check-prog.bin
	if $(LOCAL_CMOC) -x none ,check-prog.foo 2>&1 > ,check-prog.err; then false; else true; fi
	$(GREP) -q "invalid input filename ,check-prog.foo" ,check-prog.err
	test ! -f ,check-prog.bin
	$(LOCAL_CMOC) -x c ,check-prog.foo
	test -f ,check-prog.bin
	rm ,check-prog.foo ,check-prog.bin ,check-prog.err

# Checks that a K&R prototype for a function that has no return type is usable in a call.
#
check-kandr-prototype:
	$(PERLEXEC) 'print "f(); int main() { return f(); }\n"' >,check-prog.c
	$(PERLEXEC) 'print "f() { return 0xBEEF; }\n"' >,check-lib.c
	$(LOCAL_CMOC) -Werror ,check-prog.c ,check-lib.c
	rm -f ,check-prog.* ,check-lib.*

check-local-prototype:
	$(PERLEXEC) 'print "#include <cmoc.h>\nint main() { long f(long m, int n); printf(\x22%ld\x22, f(650000L, 4321)); return 0; }\n"' >,check-prog.c
	$(PERLEXEC) 'print "long f(long m, int n) { return m + n; }\n"' >,check-lib.c
	$(LOCAL_CMOC) --usim -Werror ,check-prog.c ,check-lib.c
	test "`$(USIM) ,check-prog.srec`" = "654321"
	rm -f ,check-prog.* ,check-lib.*

# Check that a failed assert() prints an error message and quits.
#
check-assert:
	$(PERLEXEC) 'print "#include <assert.h>\nint main() { assert(1 == 2); return 0; }\n"' >,check-prog.c
	$(LOCAL_CMOC) --usim -Werror ,check-prog.c
	test "`$(USIM) ,check-prog.srec`" = "***ASSERT FAILED: ,check-prog.c:2: 1 == 2"
	rm -f ,check-prog.*

# Check that _SetFailedAssertHandler() causes a failed assert() to call a custom handler
# and that the handler is allowed to let the execution continue.
#
check-assert-handler:
	$(PERLEXEC) 'print q!#include <assert.h>!, "\n"; \
			print q!#include <cmoc.h>!, "\n"; \
			print q!void h(const char *f, int l, const char *c) { printf("[%s:%d:%s]", f, l, c); }!, "\n"; \
			print q!int main() { _SetFailedAssertHandler(h); assert(1 == 2); printf("[after]"); return 0; }!, "\n"' >,check-prog.c
	$(LOCAL_CMOC) --usim -Werror ,check-prog.c
	test "`$(USIM) ,check-prog.srec`" = "[,check-prog.c:4:1 == 2][after]"
	rm -f ,check-prog.*

# Check that --initial-s option.
# - No LDS by default w/ --coco or --os9.
# - LDS added when --initial-s used w/ --coco.
# - Not allowed when used w/ --os9.
#
check-initial-s:
	rm -f ,check-prog ,check-prog.*
	$(PERLEXEC) 'print q!int main() { return 0; }!, "\n";' >,check-prog.c

	$(LOCAL_CMOC) --coco -i ,check-prog.c
	$(PERLEXEC) '/\bLDS\b/ and exit 1' ,check-prog.lst
	$(HD) ,check-prog.bin | $(EGREP) -q '^00000000  00 .. .. 28 00 cc fc 00  17 '
	rm -f ,check-prog.bin ,check-prog.lst

	$(LOCAL_CMOC) --os9 -i ,check-prog.c
	$(PERLEXEC) '/\bLDS\b/ and exit 1' ,check-prog.lst
	rm -f ,check-prog ,check-prog.lst

	$(LOCAL_CMOC) --coco --initial-s=ABCD -i ,check-prog.c
	$(EGREP) -q -w LDS ,check-prog.lst
	$(HD) ,check-prog.bin | $(EGREP) -q '^00000000  00 .. .. 28 00 10 ce ab  cd cc fc 00 17 '
	rm -f ,check-prog.bin ,check-prog.lst

	$(LOCAL_CMOC) --os9 --initial-s=ABCD -i ,check-prog.c > ,check-prog.txt || true
	$(EGREP) -q 'cmoc: fatal error: --initial-s not compatible with OS-9' ,check-prog.txt

	rm -f ,check-prog ,check-prog.*


# Test ASMText::removeConsecutiveLEASInstructions() on a compilation for Color Basic,
# to test the case where LEAS 4,S is followed by LEAS -5,S.
# We expect this optimization to replace those two instructions with LEAS -1,S.
# The -5 comes from the 5-byte float format used by Color Basic.
#
# Testing with --mc6839 would involve a 4-byte float format, which would yield LEAS 4,S and LEAS -4,S,
# which would be optimized by removing both instructions.
# See optimization "Removal of two consecutive LEAS instructions that cancel each other"
# in test-program-output.pl for this case.
#
check-leas-optim:
	$(PERLEXEC) 'print q!void foo(float x) {}  int main() { float f, g; foo(f + g); return 0; }!, "\n";' >,check-prog.c
	$(LOCAL_CMOC) --coco -i -O0 ,check-prog.c
# Check that the unoptimized version has LEAS 4,S and LEAS -5,S, but not LEAS -1,S, which will be generated by the optimization.
	$(EGREP) -q 'LEAS.*4,S' ,check-prog.s
	$(EGREP) -q 'LEAS.*-5,S' ,check-prog.s
	$(EGREP) -q 'LEAS.*-1,S' ,check-prog.s; $(PERLEXEC) 'exit !$$ARGV[0]' $$?
# Recompile with optimizations.
	$(LOCAL_CMOC) --coco -i -O2 ,check-prog.c
# Check that the optimized version has LEAS -1,S instead of LEAS 4,S and LEAS -5,S.
	$(EGREP) -q 'LEAS.*4,S' ,check-prog.s; $(PERLEXEC) 'exit !$$ARGV[0]' $$?
	$(EGREP) -q 'LEAS.*-5,S' ,check-prog.s; $(PERLEXEC) 'exit !$$ARGV[0]' $$?
	$(EGREP) -q 'LEAS.*-1,S' ,check-prog.s

	rm -f ,check-prog ,check-prog.*

# Check the -w option, which inhibits all warnings.
#
check-dash-w:
	rm -f ,check-prog ,check-prog.*
	$(PERLEXEC) 'print q!int main() { undeclaredFunction(); return 0; }!, "\n";' >,check-prog.c
	$(LOCAL_CMOC) -c ,check-prog.c | $(GREP) -qw warning:
	$(LOCAL_CMOC) -c -w -Werror ,check-prog.c
	rm -f ,check-prog ,check-prog.*

check: check-writecocofile check-misc check-program-output check-bad-programs


EXTRA_DIST = $(bin_SCRIPTS) $(pkgdata_DATA) $(pkgdata_SCRIPTS) \
             test-program-output.pl test-bad-programs.pl check-long.c

.PHONY: cleanlib
cleanlib:
	$(MAKE) -C stdlib clean
	$(MAKE) -C float clean

CLEANFILES = parser.cc parser.hh lexer.cc

MAINTAINERCLEANFILES = Makefile.in
