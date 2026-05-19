        INCLUDE float.inc

	SECTION code

initDWordFromSingle	EXPORT

negateDWord             IMPORT


; Based on Color Basic's routine at $B3ED.
; Input: Carry = signedness flag: 0 if input long is unsigned, non-zero if signed.
; 	 D => address of source number. X => address of destination number.
;
initDWordFromSingle
	pshs	u,y,x,cc
	tfr	d,x		; point X to source real
	jsr	$BC14		; load FPA0 from X
;
	puls	cc			; get signedness flag in C
	bcc	@checkUnsignedRange	; if unsigned
;
	lda	FP0EXP
	cmpa	#$80+32		; is FPA0 >= 2^32?
	bhs	@tooHighSigned
	leax	packedMinus2To31,pcr
	jsr	$BC96		; compare FPA0 to -2^31
	blt	@tooLowSigned
	bra	@inRange
;
@checkUnsignedRange
	tst	FP0SGN
	bmi	@zero		; if FPA0 negative, return 0UL
	lda	FP0EXP
	cmpa	#$80+32		; is FPA0 > 2^32?
	bhi	@tooHighUnsigned
;
@inRange
;
; Shift the mantissa right until the binary point is 32 bits from the left of the mantissa.
; We do not use Color Basic's $BCC8 routine because it is off by one on negative values, for C.
        lda     FP0EXP
        suba    #$80            ; real exponent in A (0..32); we want to increase it to 32
        bls     @zero
	bra	@byteShiftCond
@byteShiftLoop
; Shift 8 bits right.
        ldx     FP0MAN+1
        stx     FP0MAN+2
        ldb     FP0MAN
        stb     FP0MAN+1
        clr     FP0MAN
        adda    #8
@byteShiftCond
        cmpa    #32
	beq	@shiftDone
        cmpa    #24
        bls     @byteShiftLoop
@shiftLoop			; shift one bit at a time under A is 32
        lsr     FP0MAN
        ror     FP0MAN+1
        ror     FP0MAN+2
        ror     FP0MAN+3
        inca
@shiftCond
        cmpa    #32
        blo     @shiftLoop
@shiftDone
; Absolute value of result is in FP0MAN. Apply the sign.
        tst     FP0SGN
        bpl     @store
        ldx	#FP0MAN
	lbsr	negateDWord
        bra     @store
@zero
        clr	FP0MAN
        clr	FP0MAN+1
        clr	FP0MAN+2
        clr	FP0MAN+3
	bra	@store
@tooHighUnsigned
	ldd	#$FFFF
	std	FP0MAN
	std	FP0MAN+2
	bra	@store
@tooHighSigned
        tst     FP0SGN
        bpl     @maxSigned
@tooLowSigned
        ldd     #$8000
	std	FP0MAN
        clr	FP0MAN+2
        clr	FP0MAN+3
        bra     @store
@maxSigned
	ldd	#$7FFF
	std	FP0MAN
	lda	#$FF
	std	FP0MAN+2
@store				; FP0MAN now contains final dword
	puls	x		; get dest address from stack
	ldd	FP0MAN
	std	,x
	ldd	FP0MAN+2
	std	2,x
	puls	y,u,pc
packedMinus2To31
	fcb	$a0		; 128 + exponent 32
	fdb	$8000
	fdb	$0000



	ENDSECTION
