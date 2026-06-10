        INCLUDE float.inc

	SECTION code

initSignedWordFromSingle	EXPORT


; Location of 5 bytes that represent -32768.0 in the packed float format.
;
        IFDEF _COCO_BASIC_
packedMinus32768        EQU     $B3DF
        ENDC
        IFDEF DRAGON
packedMinus32768        EQU     $8B1C
        ENDC
        IFDEF _CMOC_NATIVE_FLOAT_
packedMinus32768:
        fcb     $90,$80,$00,$00,$00             ; ECB has this at $B3DF
        ENDC

        IFDEF _CMOC_MC6839_
packedMinus32768:
        fdb     $C700,$0000
        ENDC


; Input: D => address of source number. X => address of destination number.
;
initSignedWordFromSingle

        IFDEF _CMOC_MC6839_

        pshs    u,y
        leay    FPCB_SINGLE,pcr
        exg     d,y             ; point Y to source real, point D to FPCB
        lbsr    FPREG
        fcb     FFIXS
        puls    y,u,pc

        ELSE

; Based on Color Basic's routine at $B3ED.
	pshs	u,y,x		; save X that points to destination
	tfr	d,x		; point X to source real
	flt_unpackFromXToFPA0
;
	flt_loadAFromFPA0BiasedExp
        beq     @zero
	cmpa	#EXPBIAS+16     ; is |FPA0| >= 32768?
	bhs	@tooHigh
; Here, real exponent is in -127..15.
        IFDEF _CMOC_MS_FLOAT_
	ldx	#packedMinus32768
        ELSE
        leax    packedMinus32768,pcr
        ENDC
	flt_compareFPA0ToX	; compare FPA0 to -32768
	blt	@tooLow
;
; Shift the mantissa right until the binary point is 16 bits from the left of the mantissa.
; We do not use Color Basic's $BCC8 routine because it is off by one on negative values, for C.
        flt_loadAFromFPA0BiasedExp
        suba    #EXPBIAS        ; real exponent in A (0..15); we want to increase it to 16
        bls     @zero
        cmpa    #8
        bhi     @byteShiftDone
; Shift 8 bits right.
        ldb     FP0MAN
        stb     FP0MAN+1
        clr     FP0MAN
        adda    #8
        bra     @shiftCond
@byteShiftDone
@shiftLoop
        lsr     FP0MAN
        ror     FP0MAN+1
        inca
@shiftCond
        cmpa    #16
        blo     @shiftLoop
; Absolute value of result is in FP0MAN. Apply the sign.
        flt_testFPA0Sign
        bpl     @nonNeg
        clra
        clrb
        subd    FP0MAN
        bra     @store
@nonNeg
        ldd     FP0MAN
	bra	@store
@zero
        clra
        clrb
	bra	@store
@tooHigh
        flt_testFPA0Sign
        bpl     @max
        ldd     #-32768
        bra     @store
@max
	ldd	#32767
	bra	@store
@tooLow
	ldd	#-32768
@store
	std	[,s]		; get dest address from stack, store word there
	puls	x,y,u,pc

        ENDC


	ENDSECTION
