# $Id: Makefile.am,v 1.100 2025/09/25 00:21:46 sarrazip Exp $

# N.B.: CMOC must already have been compiled before this directory can be processed.

# CAUTION: In this library, references to writable data must be specified with lowercase ",pcr",
#          while references to code or read-only data must be specified with uppercase ",PCR".
#          This is for the benefit of OS-9, where ",pcr" gets converted to ",Y".

# Flags to be passed to all invocations of lwasm.
LWASMFLAGS =

# The header files typically go in /usr/share/cmoc/include.
#
cmocincdir = $(pkgdatadir)/include
cmocinc_HEADERS = \
	assert.h \
	assert-impl.h \
	cmoc.h \
	coco.h \
	disk.h \
	stdarg.h \
	setjmp.h \
	vectrex.h \
	dskcon-standalone.h \
	cmoc-stdlib-private.h

vectrexincdir = $(cmocincdir)/vectrex
vectrexinc_HEADERS = \
	vectrex/stdlib.h \
	vectrex/bios.h \
	vectrex/compatibility.h \
	vectrex/types.h

# The ecb, usim, os9, vec, dgn, etc. keywords in these library names
# are specified in invokeLinker() [main.cpp].
#
cmoclibdir = $(pkgdatadir)/lib
cmoclib_DATA = \
	libcmoc-crt-ecb.a \
	libcmoc-crt-usim.a \
	libcmoc-crt-os9.a \
	libcmoc-crt-flex.a \
	libcmoc-crt-vec.a \
	libcmoc-crt-dgn.a \
	libcmoc-crt-void.a \
	libcmoc-crt-thm.a \
	libcmoc-crt-tht.a \
	libcmoc-std-ecb.a \
	libcmoc-std-usim.a \
	libcmoc-std-os9.a \
	libcmoc-std-flex.a \
	libcmoc-std-vec.a \
	libcmoc-std-dgn.a \
	libcmoc-std-void.a \
	libcmoc-std-thm.a \
	libcmoc-std-tht.a

# Essential library that contains routines called by CMOC-generated code,
# e.g., main() caller, operations on longs, etc.
#
CRT_ASM = \
	crt.asm \
	addDWordDWord.asm \
	addDWordInt.asm \
	addDWordUnsignedInt.asm \
	adddww.asm \
	addIntDWord.asm \
	addUnsignedIntDWord.asm \
	andDWordDWord.asm \
	andDWordWord.asm \
	cmpDWordDWord.asm \
	cmpDWordSignedInt.asm \
	cmpDWordUnsignedInt.asm \
	cmpDWordSwitchExpr.asm \
	cmpdww.asm \
	cmpSignedIntDWord.asm \
	cmpUnsignedIntDWord.asm \
	copyDWord.asm \
	copyDWordFromXToD.asm \
	copyMem.asm \
	copyWordToDWord.asm \
	decrementDWord.asm \
	DIV16.asm \
	DIV16BY10.asm \
	DIV32.asm \
	DIV8.asm \
	DIV8BY7.asm \
	DIV8BY3.asm \
	divDBy16.asm \
	divdwb.asm \
	divDWordDWord.asm \
	divDWordUnsignedInt.asm \
	divdww.asm \
	divModDWordDWord.asm \
	divModDWordUnsignedInt.asm \
	divModUnsignedIntDWord.asm \
	divUnsignedIntDWord.asm \
	dwordJumpTableSwitch.asm \
	signedDWordJumpTableSwitch.asm \
	unsignedDWordJumpTableSwitch.asm \
	incrementDWord.asm \
	initByteFromDWord.asm \
	initDWordFromSignedWord.asm \
	initDWordFromUnsignedWord.asm \
	initWordFromDWord.asm \
	isDWordZero.asm \
	jumpTableSwitch.asm \
	leftShiftDWord.asm \
	leftShiftDWordAtXByB.asm \
	modDWordDWord.asm \
	modDWordUnsignedInt.asm \
	modUnsignedIntDWord.asm \
	MUL168_TO24.asm \
	MUL168_TO32.asm \
	MUL16.asm \
	MUL16BY10.asm \
	MUL32.asm \
	mulDWordDWord.asm \
	mulDWordInt.asm \
	mulDWordUnsignedInt.asm \
	mulIntDWord.asm \
	mulUnsignedIntDWord.asm \
	negateDWord.asm \
	orDWordDWord.asm \
	orDWordWord.asm \
	push4ByteStruct.asm \
	push5ByteStruct.asm \
	pushStruct.asm \
	pushSmallStruct.asm \
	resetBBytes.asm \
	resetDBytes.asm \
	rightShiftDWord.asm \
	rightShiftDWordAtXByB.asm \
	SDIV16.asm \
	SDIV8.asm \
	shiftByteLeft.asm \
	shiftByteRightSigned.asm \
	shiftByteRightUnsigned.asm \
	shiftLeft.asm \
	shiftRightSigned.asm \
	shiftRightUnsigned.asm \
	signedDivDWordDWord.asm \
	signExtWordToDWord.asm \
	sub32.asm \
	sub32xu.asm \
	subDWordDWord.asm \
	subDWordInt.asm \
	subDWordUnsignedInt.asm \
	subdww.asm \
	subIntDWord.asm \
	subUnsignedIntDWord.asm \
	testAndRemoveSignOnDWord.asm \
	testAndRemoveSignOnWord.asm \
	tfrZtoB.asm \
	xorDWordDWord.asm \
	xorDWordWord.asm

# So-called "standard" CMOC library, which contains some Standard C functions
# and other useful C functions.
#
STD_ASM = \
	abs.asm \
	atoi.asm \
	atoui.asm \
	ATOW.asm \
	checkGuardVar.asm \
	checknull.asm \
	checkstack.asm \
	delay.asm \
	dwtoa.asm \
	hexdig.asm \
	labs.asm \
	LINBUF.asm \
	memcmp.asm \
	memicmp.asm \
	memcmpimpl.asm \
	memcpy.asm \
	memmove.asm \
	memset.asm \
	MUL168.asm \
	printf.asm \
	putchar_a.asm \
	putstr.asm \
	readline.asm \
	readword.asm \
	sbrk.asm \
	sbrkmax.asm \
	seed.asm \
	setConsoleOutHook.asm \
	sprintf.asm \
	vsprintf.asm \
	chrtomem.asm \
	strcat.asm \
	strchr.asm \
	strrchr.asm \
	strstr.asm \
	strcmp.asm \
	stricmp.asm \
	strcmpimpl.asm \
	strcpy.asm \
	strlen.asm \
	strlwr.asm \
	strncpy.asm \
	strtoul10.asm \
	strtoul16.asm \
	strtoulUtil.asm \
	strupr.asm \
	tolower.asm \
	toupper.asm \
	zerodw.asm \
	convertABToUpperCase.asm \
	passLeadingWhiteSpaceChars.asm \
	checkWhiteSpace.asm

# Also part of 'std' library:
#
C_COMMON = \
	atol.c \
	atol16.c \
	atoul.c \
	atoul16.c \
	strspn.c \
	strcspn.c \
	strtok.c \
	strpbrk.c \
	utoa10.c \
	itoa10.c \
	ultoa10.c \
	ltoa10.c \
	ultoa16.c \
	_FinishIntegerToASCII.c \
	stdarg.c \
	setjmp.c \
	sqrt16.c \
	sqrt32.c \
	divmod16.c \
	divmod8.c \
	rand.c \
	initCoCoSupport.c \
	qsort.c \
	bsearch.c \
	strncmp.c \
	memchr.c \
	memichr.c \
	memset16.c \
	isspace.c \
	isdigit.c \
	isalpha.c \
	isalnum.c \
	assert.c

# Also part of 'std' library, but only under Color Basic and Dragon:
#
C_COCO_OR_DRAGON_BASIC = \
	attr.c \
	cls.c \
	cmp.c \
	coldStart.c \
	getTextMode.c \
	hscreen.c \
	hset.c \
	inkey.c \
	isKeyPressed.c \
	locate.c \
	palette.c \
	paletteRGB.c \
	pcls.c \
	pmode.c \
	pmodevars.c \
	readJoystickButtons.c \
	readJoystickPositions.c \
	resetPalette.c \
	rgb.c \
	screen.c \
	setBorderColor.c \
	setCaseFlag.c \
	setHighSpeed.c \
	setOutputDevice.c \
	setPmodeGraphicsAddressEx.c \
	setSAMRegisters.c \
	showGraphicsAddress.c \
	showLowResTextAddress.c \
	showPmode4.c \
	sleep.c \
	sound.c \
	waitkey.c \
	width.c

# Also part of 'std' library, but only under Color Basic:
#
C_COCO_BASIC = \
	disk.c \
	readDECBFile.c \
	dskcon-standalone.c \
	readDECBFileWithDECB.c \
	read-disk-file.c

# Also part of 'std' library, but only for OS-9:
#
C_OS9 =

H_OS9 =

# Also part of 'std' library, but only for the Vectrex:
#
C_VECTREX = vectrex_bios.c


CRT_ECB_OBJ  = $(CRT_ASM:.asm=.ecb_o)
CRT_USIM_OBJ = $(CRT_ASM:.asm=.usim_o)
CRT_OS9_OBJ  = $(CRT_ASM:.asm=.os9_o)
CRT_FLEX_OBJ  = $(CRT_ASM:.asm=.flex_o)
CRT_VECTREX_OBJ  = $(CRT_ASM:.asm=.vec_o)
CRT_DRAGON_OBJ  = $(CRT_ASM:.asm=.dgn_o)
CRT_VOID_OBJ  = $(CRT_ASM:.asm=.void_o)
CRT_THM_OBJ  = $(CRT_ASM:.asm=.thm_o)
CRT_THT_OBJ  = $(CRT_ASM:.asm=.tht_o)

libcmoc-crt-ecb.a: $(CRT_ECB_OBJ)
	$(LWAR) -c $@ $(CRT_ECB_OBJ)
libcmoc-crt-usim.a: $(CRT_USIM_OBJ)
	$(LWAR) -c $@ $(CRT_USIM_OBJ)
libcmoc-crt-os9.a: $(CRT_OS9_OBJ)
	$(LWAR) -c $@ $(CRT_OS9_OBJ)
libcmoc-crt-flex.a: $(CRT_FLEX_OBJ)
	$(LWAR) -c $@ $(CRT_FLEX_OBJ)
libcmoc-crt-vec.a: $(CRT_VECTREX_OBJ)
	$(LWAR) -c $@ $(CRT_VECTREX_OBJ)
libcmoc-crt-dgn.a: $(CRT_DRAGON_OBJ)
	$(LWAR) -c $@ $(CRT_DRAGON_OBJ)
libcmoc-crt-void.a: $(CRT_VOID_OBJ)
	$(LWAR) -c $@ $(CRT_VOID_OBJ)
libcmoc-crt-thm.a: $(CRT_THM_OBJ)
	$(LWAR) -c $@ $(CRT_THM_OBJ)
libcmoc-crt-tht.a: $(CRT_THT_OBJ)
	$(LWAR) -c $@ $(CRT_THT_OBJ)

STD_ECB_OBJ  = $(STD_ASM:.asm=.ecb_o)  $(C_COMMON:.c=.ecb_o) $(C_COCO_OR_DRAGON_BASIC:.c=.ecb_o) $(C_COCO_BASIC:.c=.ecb_o)
STD_USIM_OBJ = $(STD_ASM:.asm=.usim_o) $(C_COMMON:.c=.usim_o) disk.usim_o
STD_OS9_OBJ  = $(STD_ASM:.asm=.os9_o)  $(C_COMMON:.c=.os9_o) $(C_OS9:.c=.os9_o)
STD_FLEX_OBJ  = $(STD_ASM:.asm=.flex_o)  $(C_COMMON:.c=.flex_o)
STD_VECTREX_OBJ = $(STD_ASM:.asm=.vec_o) $(C_COMMON:.c=.vec_o) $(C_VECTREX:.c=.vec_o)
STD_DRAGON_OBJ  = $(STD_ASM:.asm=.dgn_o)  $(C_COMMON:.c=.dgn_o) $(C_COCO_OR_DRAGON_BASIC:.c=.dgn_o)
STD_VOID_OBJ  = $(STD_ASM:.asm=.void_o)  $(C_COMMON:.c=.void_o)
STD_THM_OBJ  = $(STD_ASM:.asm=.thm_o)  $(C_COMMON:.c=.thm_o)
STD_THT_OBJ  = $(STD_ASM:.asm=.tht_o)  $(C_COMMON:.c=.tht_o)

libcmoc-std-ecb.a: $(STD_ECB_OBJ)
	$(LWAR) -c $@ $(STD_ECB_OBJ)
libcmoc-std-usim.a: $(STD_USIM_OBJ)
	$(LWAR) -c $@ $(STD_USIM_OBJ)
libcmoc-std-os9.a: $(STD_OS9_OBJ)
	$(LWAR) -c $@ $(STD_OS9_OBJ)
libcmoc-std-flex.a: $(STD_FLEX_OBJ)
	$(LWAR) -c $@ $(STD_FLEX_OBJ)
libcmoc-std-vec.a: $(STD_VECTREX_OBJ)
	$(LWAR) -c $@ $(STD_VECTREX_OBJ)
libcmoc-std-dgn.a: $(STD_DRAGON_OBJ)
	$(LWAR) -c $@ $(STD_DRAGON_OBJ)
libcmoc-std-void.a: $(STD_VOID_OBJ)
	$(LWAR) -c $@ $(STD_VOID_OBJ)
libcmoc-std-thm.a: $(STD_THM_OBJ)
	$(LWAR) -c $@ $(STD_THM_OBJ)
libcmoc-std-tht.a: $(STD_THT_OBJ)
	$(LWAR) -c $@ $(STD_THT_OBJ)

%.ecb_o: %.asm std.inc ../float/float.inc
	$(LWASM) $(LWASMFLAGS) -fobj -D_COCO_BASIC_ -D_COCO_OR_DRAGON_BASIC_ --output=$@ $<
%.usim_o: %.asm std.inc ../float/float.inc
	$(LWASM) $(LWASMFLAGS) -fobj -DUSIM --output=$@ $<
OS9TEMP = $(notdir $<.os9asm)
%.os9_o: %.asm std.inc ../float/float.inc
	$(PERL) $(srcdir)/os9fixup.pl < $< > $(OS9TEMP)
	$(LWASM) $(LWASMFLAGS) -fobj -DOS9 -I$(srcdir) --pragma=dollarnotlocal --output=$@ $(OS9TEMP)
	rm $(OS9TEMP)
# OS9: dollarnotlocal is necessary to avoid errors on identifiers like "F$Exit".
%.flex_o: %.asm std.inc ../float/float.inc
	$(LWASM) $(LWASMFLAGS) -fobj -DFLEX --output=$@ $<
%.vec_o: %.asm std.inc ../float/float.inc
	$(LWASM) $(LWASMFLAGS) -fobj -DVECTREX --output=$@ $<
%.dgn_o: %.asm std.inc ../float/float.inc
	$(LWASM) $(LWASMFLAGS) -fobj -DDRAGON -D_COCO_OR_DRAGON_BASIC_ --output=$@ $<
%.void_o: %.asm std.inc ../float/float.inc
	$(LWASM) $(LWASMFLAGS) -fobj -D_CMOC_VOID_TARGET_ --output=$@ $<
%.thm_o: %.asm std.inc ../float/float.inc
	$(LWASM) $(LWASMFLAGS) -fobj -DTHOMMO --output=$@ $<
%.tht_o: %.asm std.inc ../float/float.inc
	$(LWASM) $(LWASMFLAGS) -fobj -DTHOMTO --output=$@ $<

INTERMEDIATE = #--intermediate
CMOC_LIB_FLAGS = -O2 -DNDEBUG -nostdinc -I $(srcdir) $(INTERMEDIATE) -Werror -Wno-inline-asm-array-indexes

%.ecb_o: %.c std.inc ../float/float.inc
	../cmoc -c $(CMOC_LIB_FLAGS) -o $@ $<
%.usim_o: %.c std.inc ../float/float.inc
	../cmoc -c $(CMOC_LIB_FLAGS) --usim -o $@ $<
%.os9_o: %.c std.inc ../float/float.inc
	../cmoc -c $(CMOC_LIB_FLAGS) --os9 -o $@ $<
%.flex_o: %.c std.inc ../float/float.inc
	../cmoc -c $(CMOC_LIB_FLAGS) --flex -o $@ $<
%.vec_o: %.c std.inc ../float/float.inc
	../cmoc -c $(CMOC_LIB_FLAGS) --vectrex -DSTD_INC_PATH='$(srcdir)/std.inc' -o $@ $<
%.dgn_o: %.c std.inc ../float/float.inc
	../cmoc -c $(CMOC_LIB_FLAGS) --dragon -o $@ $<
%.void_o: %.c std.inc ../float/float.inc
	../cmoc -c $(CMOC_LIB_FLAGS) --void-target -o $@ $<
%.thm_o: %.c std.inc ../float/float.inc
	../cmoc -c $(CMOC_LIB_FLAGS) --thommo -o $@ $<
%.tht_o: %.c std.inc ../float/float.inc
	../cmoc -c $(CMOC_LIB_FLAGS) --thomto -o $@ $<

# Checks if some files contain non-breakable spaces (character 160).
# The statement in the 'continue' clause resets the line counter ($.)
# at the end of each processed file.
#
check:
	$(PERL) -e 'while (<>) { if (/\xA0/) { print "Non-breakable space at $$ARGV:$$.\n"; exit 1 } } continue { close ARGV if eof }' \
		$(cmocinc_HEADERS) $(vectrexinc_HEADERS) \
		$(CRT_ASM) $(STD_ASM) \
		$(C_COMMON) $(C_COCO_OR_DRAGON_BASIC) $(C_COCO_BASIC) $(H_OS9) $(C_OS9) $(C_VECTREX) \
		Makefile.am

CLEANFILES = $(cmoclib_DATA) \
        $(CRT_ECB_OBJ) $(CRT_USIM_OBJ) $(CRT_OS9_OBJ) $(CRT_FLEX_OBJ) $(CRT_VECTREX_OBJ) $(CRT_DRAGON_OBJ) $(CRT_VOID_OBJ) \
        $(CRT_THM_OBJ) $(CRT_THT_OBJ) \
        $(STD_ECB_OBJ) $(STD_USIM_OBJ) $(STD_OS9_OBJ) $(STD_FLEX_OBJ) $(STD_VECTREX_OBJ) $(STD_DRAGON_OBJ) $(STD_VOID_OBJ) \
        $(STD_THM_OBJ) $(STD_THT_OBJ)

EXTRA_DIST = std.inc os9fixup.pl $(CRT_ASM) $(STD_ASM) \
		$(C_COMMON) $(C_COCO_OR_DRAGON_BASIC) $(C_COCO_BASIC) $(H_OS9) $(C_OS9) $(C_VECTREX)
