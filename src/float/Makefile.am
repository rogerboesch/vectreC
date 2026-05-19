# $Id: Makefile.am,v 1.7 2020/06/06 04:41:44 sarrazip Exp $

cmoclibdir = $(pkgdatadir)/lib
cmoclib_DATA = \
	libcmoc-float-ecb.a

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
	divIntSingle.asm \
	divSignedDWordSingle.asm \
	divSingleInt.asm \
	divSingleSignedDWord.asm \
	divSingleSingle.asm \
	divSingleUnsignedDWord.asm \
	divSingleUnsignedInt.asm \
	divUnsignedDWordSingle.asm \
	divUnsignedIntSingle.asm \
	ftoa.asm \
	incrementSingle.asm \
	initDWordFromSingle.asm \
	initSignedByteFromSingle.asm \
	initSignedWordFromSingle.asm \
	initSingleFromDWord.asm \
	initSingleFromSignedWord.asm \
	initSingleFromUnsignedWord.asm \
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
	strtof.asm \
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
	unpackXToFPA0AndDiv.asm \
	unpackXtoFPA0AndPrep.asm \
	unpackXToFPA1AndMul.asm

# Also part of 'float' library, but only under Color Basic:
C_COCO_BASIC = \
	atoff.c

ECB_OBJ  = $(ASM:asm=ecb_o) $(C_COCO_BASIC:.c=.ecb_o)

libcmoc-float-ecb.a: $(ECB_OBJ)
	$(LWAR) -c $@ $(ECB_OBJ)

.asm.ecb_o:
	$(LWASM) -fobj -D_COCO_BASIC_ --output=$@ $<

CMOC_LIB_FLAGS = -O2 -DNDEBUG -nostdinc -I $(srcdir)/../stdlib -I $(srcdir)

.c.ecb_o:
	../cmoc -c $(CMOC_LIB_FLAGS) -o $@ $<

# Checks if some files contain non-breakable spaces (character 160).
# The statement in the 'continue' clause resets the line counter ($.)
# at the end of each processed file.
#
check:
	$(PERL) -e 'while (<>) { if (/\xA0/) { print "ERROR: non-breakable space at $$ARGV:$$.\n"; exit 1 } } continue { close ARGV if eof }' \
		$(ASM) $(C_COCO_BASIC) Makefile.am

CLEANFILES = $(cmoclib_DATA) $(ECB_OBJ) $(ECB_OBJ:.ecb_o=.s) $(ECB_OBJ:.ecb_o=.lst)

EXTRA_DIST = float.inc $(ASM) $(C_COCO_BASIC)
