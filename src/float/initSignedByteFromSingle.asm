        INCLUDE float.inc

	SECTION code

initSignedByteFromSingle	EXPORT


; Based on Color Basic's routine at $B3ED.
;
initSignedByteFromSingle
	pshs	u,y,x		; save X that points to destination
	tfr	d,x		; point X to source real
	jsr	$BC14		; load FPA0 from X
;
	lda	FP0EXP
	cmpa	#$80+8		; is FPA0 >= 128?
	bhs	@tooHigh
	leax	packedMinus128,pcr
	jsr	$BC96		; compare FPA0 to -128
	blt	@tooLow
;
; Shift the mantissa right until the binary point is 8 bits from the left of the mantissa.
; We do not use Color Basic's $BCC8 routine because it is off by one on negative values, for C.
        lda     FP0EXP
        suba    #$80            ; real exponent in A (0..7); we want to increase it to 16
        bls     @zero
        ldb     FP0MAN
@shiftLoop
        lsrb
        inca
        cmpa    #8
        blo     @shiftLoop
; Absolute value of result is in FP0MAN. Apply the sign.
        tst     FP0SGN
        bpl     @store
        negb
        bra     @store
@zero
        clrb
	bra	@store
@tooHigh
        tst     FP0SGN
        bpl     @max
        ldb     #-128
        bra     @store
@max
	ldb	#127
	bra	@store
@tooLow
	ldb	#-128
@store
	stb	[,s]		; get dest address from stack, store byte there
	puls	x,y,u,pc
packedMinus128
	fdb	$8880
	fdb	$0000
	fcb	$00




	ENDSECTION
