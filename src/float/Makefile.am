# $Id: Makefile.am,v 1.44 2024/02/25 19:58:21 sarrazip Exp $
#
# ecb = Extended Color Basic.
# dgn = Dragon.
# native = CMOC's own floating-point library (experimental, slow and bloated).
# mc6839 = External library based on the Motorola MC6839.
# mc6839_os9 = Same, but compiled for OS-9.

cmoclibdir = $(pkgdatadir)/lib
cmoclib_DATA = \
	float-ctor.ecb_o \
	float-ctor.dgn_o \
	float-ctor.mc6839_o \
	float-ctor.mc6839_os9_o \
	libcmoc-float-ecb.a \
	libcmoc-float-dgn.a \
	libcmoc-float-mc6839.a \
	libcmoc-float-mc6839_os9.a

if DISTRIBUTE_FLOATABLE

cmoclib_DATA += \
	float-ctor.native_o \
	libcmoc-float-native.a

endif

# Utility routines called by CMOC-generated code.
# These routines use flt_* assembly language macros that are defined
# for each supported float library.
#
# CAUTION: For portability to OS-9, these routines must assume that Y points
#          to the data segment of the current process. They must preserve Y.
#
ASM = \
	addFPA0FPA1.asm \
	addIntSingle.asm \
	addSignedDWordSingle.asm \
	addSingleInt.asm \
	addSingleSignedDWord.asm \
	addSingleSingle.asm \
	addSingleUnsignedDWord.asm \
	addSingleUnsignedInt.asm \
	addUnsignedDWordSingle.asm \
	addUnsignedIntSingle.asm \
	binOpSingleSingle.asm \
	cmpSignedIntSingle.asm \
	cmpSingleSignedInt.asm \
	cmpSingleSingle.asm \
	cmpSingleUnsignedInt.asm \
	cmpUnsignedIntSingle.asm \
	copySingle.asm \
	decrementSingle.asm \
	divByZeroSingle.asm \
	divByZeroSingle_singleDividend.asm \
	divIntSingle.asm \
	divSignedDWordSingle.asm \
	divSingleInt.asm \
	divSingleSignedDWord.asm \
	divSingleSingle.asm \
	divSingleUnsignedDWord.asm \
	divSingleUnsignedInt.asm \
	divUnsignedDWordSingle.asm \
	divUnsignedIntSingle.asm \
	enable_printf_float.asm \
	incrementSingle.asm \
	initSignedDWordFromSingle.asm \
	initUnsignedDWordFromSingle.asm \
	initSignedByteFromSingle.asm \
	initSignedWordFromSingle.asm \
	initSingleFromSingle.asm \
	initSingleFromDWord.asm \
	initSingleFromSignedWord.asm \
	initSingleFromUnsignedWord.asm \
	initSingleFromSignedDWord.asm \
	initSingleFromUnsignedDWord.asm \
	initUnsignedByteFromSingle.asm \
	initUnsignedWordFromSingle.asm \
	isDWordZeroSpecial.asm \
	isSingleZero.asm \
	loadSignedDWordInFPA0.asm \
	loadUnsignedDInFPA0.asm \
	loadUnsignedDWordInFPA0.asm \
	mulIntSingle.asm \
	mulSignedDWordSingle.asm \
	mulSingleInt.asm \
	mulSingleSignedDWord.asm \
	mulSingleSingle.asm \
	mulSingleUnsignedDWord.asm \
	mulSingleUnsignedInt.asm \
	mulUnsignedDWordSingle.asm \
	mulUnsignedIntSingle.asm \
	negateSingle.asm \
	subIntSingle.asm \
	subSignedDWordSingle.asm \
	subSingle_common.asm \
	subSingleInt.asm \
	subSingleSignedDWord.asm \
	subSingleSingle.asm \
	subSingleUnsignedDWord.asm \
	subSingleUnsignedInt.asm \
	subUnsignedDWordSingle.asm \
	subUnsignedIntSingle.asm \
	unpackSingleAndConvertToASCII.asm \
	unpackXToFPA0AndDiv.asm \
	unpackXtoFPA0AndPrep.asm \
	unpackXToFPA1AndMul.asm

ASM_NOT_MC6839 = \
	strtof.asm \
	ftoa.asm \
	initDWordFromSingle.asm

ASM_MC6839 = \
	mc6839-rom.asm \
	mc6839-data.asm \
	mc6839-trap.asm \
	mc6839-loadAFromFPA0BiasedExp.asm \
	mc6839-storeBInFPA0BiasedExp.asm \
	mc6839-isBiasedExponentZero.asm \
	mc6839-addNumberAtXToFPA0.asm \
	mc6839-addFPA1ToFPA0.asm \
	mc6839-subFPA0FromFPA1.asm \
	mc6839-mulFPA0ByFPA1.asm \
	mc6839-divFPA1ByFPA0.asm \
	mc6839-compareFPA0ToX.asm \
	mc6839-fixMinusZeroAtX.asm \
	mc6839-callBinDec.asm \
	mc6839-callDecBin.asm \
	mc6839-setTrap.asm

C_FILES = \
	atoff.c \
	fabsf.c

C_FILES_MSBASIC = \
	msbasic-applyRealFunction.c \
	msbasic-atanf.c \
	msbasic-cosf.c \
	msbasic-expf.c \
	msbasic-floorf.c \
	msbasic-logf.c \
	msbasic-sinf.c \
	msbasic-sqrtf.c \
	msbasic-tanf.c \
	msbasic-truncf.c \
	roundf.c

H_FILES_MC6839 = mc6839-cmoc.h
C_FILES_MC6839 = \
	mc6839-strtof.c \
	mc6839-ftoa.c \
	mc6839-log2fOfPosValue.c \
	mc6839-log2f.c \
	mc6839-logf.c

ECB_OBJ  = $(ASM:asm=ecb_o) $(ASM_NOT_MC6839:asm=ecb_o) $(C_FILES:.c=.ecb_o) $(C_FILES_MSBASIC:.c=.ecb_o)
DGN_OBJ  = $(ASM:asm=dgn_o) $(ASM_NOT_MC6839:asm=dgn_o) $(C_FILES:.c=.dgn_o) $(C_FILES_MSBASIC:.c=.dgn_o)
NATIVE_OBJ = $(ASM:asm=native_o) $(ASM_NOT_MC6839:asm=native_o) $(C_FILES:.c=.native_o) native-data.native_o
MC6839_OBJ = $(ASM:asm=mc6839_o) $(ASM_MC6839:asm=mc6839_o) $(C_FILES:.c=.mc6839_o) $(C_FILES_MC6839:.c=.mc6839_o)
MC6839_OS9_OBJ = $(ASM:asm=mc6839_os9_o) $(ASM_MC6839:asm=mc6839_os9_o) $(C_FILES:.c=.mc6839_os9_o) $(C_FILES_MC6839:.c=.mc6839_os9_o)

libcmoc-float-ecb.a: $(ECB_OBJ)
	$(LWAR) -c $@ $(ECB_OBJ)

libcmoc-float-dgn.a: $(DGN_OBJ)
	$(LWAR) -c $@ $(DGN_OBJ)

libcmoc-float-mc6839.a: $(MC6839_OBJ)
	$(LWAR) -c $@ $(MC6839_OBJ)

libcmoc-float-mc6839_os9.a: $(MC6839_OS9_OBJ)
	$(LWAR) -c $@ $(MC6839_OS9_OBJ)

if DISTRIBUTE_FLOATABLE

NATIVE_OTHER = native-data.asm native/floatable.h native/floatable-private.h

# CMOC-independent 40-bit floating-point library.
FLOATABLE_ASM = \
	native/floatable.asm
FLOATABLE_C = \
	native/addUFloat40.c \
	native/assert.c \
	native/cmpUFloat40.c \
	native/compareAccWithDivisor.c \
	native/div40by32.c \
	native/fdiv.c \
	native/fmul.c \
	native/fromDecimal.c \
	native/normalizeUFloat40.c \
	native/pack.c \
	native/print.c \
	native/setUFloat40ToSignedInt.c \
	native/setUFloat40ToZero.c \
	native/shift5BytesLeft1Bit.c \
	native/subtract32From40.c \
	native/subUFloat40.c \
	native/toDecimal.c \
	native/unpack.c
FLOATABLE_OBJ = \
	$(FLOATABLE_ASM:.asm=.native_o) \
	$(FLOATABLE_C:.c=.native_o)

# Library that contains Floatable and CMOC-specific glue code.
libcmoc-float-native.a: $(NATIVE_OBJ) $(FLOATABLE_OBJ)
	$(LWAR) -c $@ $(NATIVE_OBJ) $(FLOATABLE_OBJ)

endif


%.ecb_o: %.asm float.inc
	$(LWASM) -fobj -D_COCO_BASIC_ -D_CMOC_MS_FLOAT_ --output=$@ $<

%.dgn_o: %.asm float.inc
	$(LWASM) -fobj -DDRAGON -D_CMOC_DGN_FLOAT_ -D_CMOC_MS_FLOAT_ --output=$@ $<

%.native_o: %.asm float.inc
	$(LWASM) -fobj -D_CMOC_NATIVE_FLOAT_ --output=$@ $<

%.mc6839_o: %.asm float.inc
	$(LWASM) -fobj -D_CMOC_MC6839_ --output=$@ $<

OS9TEMP = $(notdir $<.os9asm)

# See ../stdlib/os9fixup.pl re: ,PCR vs ,pcr vs Y.
# Lines with ";;OS9" are not copied to float-os9.inc.
#
float-os9.inc: float.inc
	$(PERL) $(srcdir)/../stdlib/os9fixup.pl < $< | $(PERL) -ne '/;;OS9/ or print' > $@

%.mc6839_os9_o: %.asm float-os9.inc
	$(PERL) $(srcdir)/../stdlib/os9fixup.pl < $< > $(OS9TEMP)
	$(LWASM) -fobj -D_CMOC_MC6839_ -DOS9 -I $(srcdir) -I . --pragma=dollarnotlocal --output=$@ $(OS9TEMP)
	rm $(OS9TEMP)

INTERMEDIATE = #-i
CMOC_LIB_OPTIM = -O2
CMOC_LIB_FLAGS = $(CMOC_LIB_OPTIM) -DNDEBUG -nostdinc -I $(srcdir)/../stdlib -I $(srcdir) -Werror -Wlocal-var-hiding $(INTERMEDIATE)

%.ecb_o: %.c float.inc
	../cmoc --coco -c $(CMOC_LIB_FLAGS) -D_CMOC_MS_FLOAT_ -o $@ $<

%.dgn_o: %.c float.inc
	../cmoc --dragon -c $(CMOC_LIB_FLAGS) -D_CMOC_DGN_FLOAT_ -D_CMOC_MS_FLOAT_ -o $@ $<

%.native_o: %.c float.inc native/floatable.h native/floatable-private.h
	../cmoc -c $(CMOC_LIB_FLAGS) -D_CMOC_NATIVE_FLOAT_ -o $@ $<

%.mc6839_o: %.c float.inc
	../cmoc -c --mc6839 $(CMOC_LIB_FLAGS) -D_CMOC_MC6839_ -o $@ $<

%.mc6839_os9_o: %.c float.inc
	../cmoc -c --os9 --mc6839 $(CMOC_LIB_FLAGS) -D_CMOC_MC6839_ -DOS9 -o $@ $<

# Checks if some files contain non-breakable spaces (character 160).
# The statement in the 'continue' clause resets the line counter ($.)
# at the end of each processed file.
#
check:
	$(PERL) -e 'while (<>) { if (/\xA0/) { print "Non-breakable space at $$ARGV:$$.\n"; exit 1 } } continue { close ARGV if eof }' \
		$(ASM) $(C_FILES) Makefile.am

CLEANFILES = $(cmoclib_DATA) \
	$(ECB_OBJ) $(ECB_OBJ:.ecb_o=.s) $(ECB_OBJ:.ecb_o=.lst) \
	$(DGN_OBJ) $(DGN_OBJ:.dgn_o=.s) $(DGN_OBJ:.dgn_o=.lst) \
	$(NATIVE_OBJ) $(NATIVE_OBJ:.native_o=.s) $(NATIVE_OBJ:.native_o=.lst) $(FLOATABLE_OBJ) \
	$(MC6839_OBJ) $(MC6839_OBJ:.mc6839_o=.s) $(MC6839_OBJ:.mc6839_o=.lst) \
	$(MC6839_OS9_OBJ) $(MC6839_OS9_OBJ:.mc6839_os9_o=.s) $(MC6839_OS9_OBJ:.mc6839_os9_o=.lst) float-os9.inc

EXTRA_DIST = \
    float.inc float-ctor.asm $(ASM) $(ASM_NOT_MC6839) $(ASM_MC6839) $(C_FILES) \
    $(NATIVE_OTHER) \
    $(FLOATABLE_ASM) $(FLOATABLE_C) \
    $(C_FILES_MSBASIC) $(H_FILES_MC6839) $(C_FILES_MC6839) Float09.bin

