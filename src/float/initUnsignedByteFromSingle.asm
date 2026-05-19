        INCLUDE float.inc

	SECTION code

initUnsignedByteFromSingle	EXPORT


; Based on Color Basic's routine at $B3ED.
;
initUnsignedByteFromSingle
	pshs	u,y,x		; save X that points to destination
	tfr	d,x		; point X to source real
	jsr	$BC14		; load FPA0 from X
;
	tst	FP0SGN		; check sign of FPA0
	bmi	@tooLow		; if < 0
	lda	FP0EXP
	beq	@tooLow		; FPA0 is 0.0, so result is 0
	cmpa	#$80+8		; is FPA0 >= 256?
	bhi	@tooHigh	; if yes
; Denormalize FPA0 until exponent is 8.
	beq	@denormDone	; if exponent is 8, denorm done
@shiftBits			; exponent is in 9..15
	ldb	FP0MAN		; load high 8 bits of mantissa
@shiftLoop
	lsrb
	inca			; increment exponent
	cmpa	#$80+8
	blo	@shiftLoop	; loop if exponent not yet 8
	bra	@store		; go store D as result
@denormDone
	ldb	FP0MAN
	bra	@store
@tooHigh
	ldb	#255
	bra	@store
@tooLow
	clrb
@store
	stb	[,s]		; get dest address from stack, store byte there
	puls	x,y,u,pc




	ENDSECTION
