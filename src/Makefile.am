# $Id: Makefile.am,v 1.76 2020/06/07 14:53:53 sarrazip Exp $

# The dot forces CMOC to be compiled before processing the stdlib and float
# subdirectories, whose processing requires CMOC itself.
#
SUBDIRS = usim-0.91-cmoc . stdlib float

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
	ASMText.h \
	Pseudo6809.h \
	parser.yy \
	lexer.ll \
	Declarator.cpp \
	Declarator.h \
	DeclarationSequence.cpp \
	DeclarationSequence.h \
	Declaration.cpp \
	Declaration.h \
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
	main.cpp

AM_YFLAGS = -d
# -d produces a .h file

cmoc_CXXFLAGS = -DPKGDATADIR=\"$(pkgdatadir)\"


if BUILD_WRITECOCOFILE

writecocofile_SOURCES = writecocofile.cpp

endif


LOCAL_CMOC = $(ENV) PATH="$(srcdir):$$PATH" ./cmoc -I $(srcdir)/stdlib/ -Lstdlib -Lfloat

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
	./cmoc --help >/dev/null
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

check-org:
	$(CREATE_TEST_PROG)
	$(LOCAL_CMOC) --org=4321 --data=6000 --intermediate --verbose ,check-prog.c 2>&1
	$(GREP) -q "Symbol: program_start (,check-prog.o) = 4321" ,check-prog.map
	$(GREP) -q "Section: rwdata .* load at 6000," ,check-prog.map
	$(LOCAL_CMOC) --org=2800 --limit=2810 ,check-prog.c 2>&1 | $(GREP) -q "program_end exceeds it"

check-dos:
	$(CREATE_TEST_PROG)
	$(LOCAL_CMOC) --dos ,check-prog.c
	$(HD) ,check-prog.bin | $(HEAD) -1 | $(GREP) -q '^00000000  00 .. .. 26 00 4f 53'
	rm -f ,check-prog

check-long:
	$(LOCAL_CMOC) --usim -DPART0 -DPART1 -DPART2 -O2 -Lstdlib -Lfloat --org=1600 $(srcdir)/check-long.c 2>&1 >/dev/null
	$(USIM) check-long.srec | $(PERL) -pe '/\bSUCCESS\b/ and $$s = 1; END { exit !$$s }'
	rm -f check-long.srec

check-float:
	$(PERLEXEC) 'print "int main() { return 18.44f == 0; }\n"' >,check-prog.c
	$(LOCAL_CMOC) --coco -c --intermediate -O0 ,check-prog.c
	$(GREP) -q 'FCB.$$85,$$13,$$85,$$1E,$$B8$$' ,check-prog.s
	$(LOCAL_CMOC) --os9 -c --intermediate -O0 ,check-prog.c
	$(GREP) -q 'FCB.$$13,$$85,$$1E,$$85$$' ,check-prog.s
	rm -f ,check-prog.*

check-os9: check-array-section
	$(CREATE_TEST_PROG)
	$(LOCAL_CMOC) --os9 --intermediate -nodefaultlibs ,check-prog.c  # work b/c main() calls nothing
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
	rm -f ,check-prog.*

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
	rm -f ,check-prog.d

check-vectrex:
	$(PERLEXEC) 'print "#include <vectrex/bios.h>\n#include <vectrex/stdlib.h>\nint main() { move(0, 0); wait_retrace(); return 0; }\n"' >,check-prog.c
	rm -f ,check-prog.bin
	$(LOCAL_CMOC) --vectrex --intermediate ,check-prog.c
	test -f ,check-prog.bin
	$(STRINGS) ,check-prog.bin | $(GREP) -q 'g GCE 2015'
	$(GREP) -q -w '^_move' ,check-prog.s
	$(GREP) -q -w '^_wait_retrace' ,check-prog.s

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

check-misc: check-base check-srec check-org check-long check-float check-os9 check-deps check-vectrex check-stack-space check-linking check-nbsp
	rm -f ,check-prog.c ,check-prog.asm ,check-prog.s ,check-prog.i \
		,check-prog.lst ,check-prog.hex ,check-prog.srec \
		,check-prog.link ,check-prog.map ,check-prog.bin ,check-prog

check-self-tests:
	$(PERL) -cw $(srcdir)/test-program-output.pl
	$(PERL) -cw $(srcdir)/test-bad-programs.pl
	$(PERL) $(srcdir)/test-program-output.pl $(STOPONFAIL) $(srcdir)
	$(PERL) $(srcdir)/test-bad-programs.pl $(STOPONFAIL) $(srcdir)

# Stack space must be specified when compiling the .c file that defines main().
check-stack-space:
	$(PERLEXEC) 'print qq[#include <cmoc.h>\nint main() { printf("\%u\\n", sbrkmax()); return 0; }\n]' >,check-prog.c
	$(LOCAL_CMOC) --usim -c -I $(srcdir)/stdlib ,check-prog.c
	$(LOCAL_CMOC) --usim --org=0 ,check-prog.o
	test `$(USIM) ,check-prog.srec` -ge 60000
	$(LOCAL_CMOC) --usim -c -I $(srcdir)/stdlib --stack-space=30000 ,check-prog.c
	$(LOCAL_CMOC) --usim --org=0 ,check-prog.o
	test `$(USIM) ,check-prog.srec` -lt 34000
	$(PERLEXEC) 'print qq[#pragma stack_space 15000\n#include <cmoc.h>\nint main() { printf("\%u\\n", sbrkmax()); return 0; }\n]' >,check-prog.c
	$(LOCAL_CMOC) --usim -c -I $(srcdir)/stdlib ,check-prog.c
	$(LOCAL_CMOC) --usim --org=0 ,check-prog.o
	test `$(USIM) ,check-prog.srec` -ge 48000
	test `$(USIM) ,check-prog.srec` -lt 49000
	rm -f ,check-prog.c ,check-prog.o ,check-prog.srec

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

# Checks if some files contain non-breakable spaces (character 160).
# The statement in the 'continue' clause resets the line counter ($.)
# at the end of each processed file.
#
check-nbsp:
	$(PERL) -e 'while (<>) { if (/\xA0/) { print "ERROR: non-breakable space at $$ARGV:$$.\n"; exit 1 } } continue { close ARGV if eof }' \
		$(cmoc_SOURCES) test-program-output.pl test-bad-programs.pl check-long.c $(bin_SCRIPTS) Makefile.am

check: check-writecocofile check-misc check-self-tests


EXTRA_DIST = $(bin_SCRIPTS) $(pkgdata_DATA) $(pkgdata_SCRIPTS) \
             test-program-output.pl test-bad-programs.pl check-long.c

.PHONY: cleanlib
cleanlib:
	$(MAKE) -C stdlib clean
	$(MAKE) -C float clean

CLEANFILES = parser.cc parser.h lexer.cc

MAINTAINERCLEANFILES = Makefile.in
