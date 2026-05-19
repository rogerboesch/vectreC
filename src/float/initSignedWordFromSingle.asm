        INCLUDE float.inc

	SECTION code

initSignedWordFromSingle	EXPORT


; Based on Color Basic's routine at $B3ED.
; Input: D => address of source number. X => address of destination number.
;
initSignedWordFromSingle
	pshs	u,y,x		; save X that points to destination
	tfr	d,x		; point X to source real
	jsr	$BC14		; load FPA0 from X
;
	lda	FP0EXP
	cmpa	#$80+16		; is FPA0 >= 32768?
	bhs	@tooHigh
	ldx	#$B3DF		; packed -32768
	jsr	$BC96		; compare FPA0 to -32768
	blt	@tooLow
;
; Shift the mantissa right until the binary point is 16 bits from the left of the mantissa.
; We do not use Color Basic's $BCC8 routine because it is off by one on negative values, for C.
        lda     FP0EXP
        suba    #$80            ; real exponent in A (0..15); we want to increase it to 16
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
        tst     FP0SGN
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
        tst     FP0SGN
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




	ENDSECTION
