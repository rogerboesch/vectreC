        INCLUDE float.inc

	SECTION code

initUnsignedWordFromSingle	EXPORT


; Based on Color Basic's routine at $B3ED.
;
initUnsignedWordFromSingle
	pshs	u,y,x		; save X that points to destination
	tfr	d,x		; point X to source real
	jsr	$BC14		; load FPA0 from X
;
	tst	FP0SGN		; check sign of FPA0
	bmi	@tooLow		; if < 0
	lda	FP0EXP
	beq	@tooLow		; FPA0 is 0.0, so result is 0
	cmpa	#$80+16		; is FPA0 >= 65536?
	bhi	@tooHigh	; if yes
; Denormalize FPA0 until exponent is 16.
	beq	@denormDone	; if exponent is 16, denorm done
	cmpa	#$80+8		; if exponent is in 9..15
	bhi	@shiftBits	; then go shift mantissa right by 1 to 7 bits
; Exponent is in 1..8. Shift mantissa right by 8 bits.
	ldb	FP0MAN		; load high byte of mantissa
	stb	FP0MAN+1	; shift it 8 bits right
	clr	FP0MAN		; clear high byte of mantissa
	adda	#8		; exponent is now 8 more than initially (now 9..16)
	cmpa	#$80+16
	beq	@denormDone
@shiftBits			; exponent is in 9..15
	ldx	#0
	tfr	a,b
	abx			; X = $80 + exponent
	ldd	FP0MAN		; load high 16 bits of mantissa
@shiftLoop
	lsra			; shift D right one bit
	rorb
	leax	1,x		; increment exponent
	cmpx	#$80+16
	blo	@shiftLoop	; loop if exponent not yet 16
	bra	@store		; go store D as result
@denormDone
	ldd	FP0MAN
	bra	@store
@tooHigh
	ldd	#65535
	bra	@store
@tooLow
	clra
	clrb
@store
	std	[,s]		; get dest address from stack, store word there
	puls	x,y,u,pc




	ENDSECTION
