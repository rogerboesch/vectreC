        INCLUDE float.inc

	SECTION code

initUnsignedByteFromSingle	EXPORT


; Input: D => address of source number. X => address of destination number.
;
initUnsignedByteFromSingle

        IFDEF _CMOC_MC6839_

initUnsignedWordFromSingle IMPORT
        pshs    x,b,a           ; save X that points to destination, create temp unsigned short with D
        leax    ,s              ; point X to temp unsigned short
        lbsr    initUnsignedWordFromSingle
        ldd     ,s++            ; pop resulting unsigned short
        blo     @tooLow
        tsta
        beq     @returnB
        ldb     #255
        bra     @returnB
@tooLow
        ldb     #-128
@returnB
        stb     [,s]
        puls    x,pc

        ELSE

; Based on Color Basic's routine at $B3ED.
	pshs	u,y,x		; save X that points to destination
	tfr	d,x		; point X to source real
	flt_unpackFromXToFPA0
;
	flt_testFPA0Sign
	bmi	@tooLow		; if < 0
	flt_loadAFromFPA0BiasedExp
	beq	@tooLow		; FPA0 is 0.0, so result is 0
	cmpa	#EXPBIAS+8	; is FPA0 >= 256?
	bhi	@tooHigh	; if yes
; Denormalize FPA0 until exponent is 8.
	beq	@denormDone	; if exponent is 8, denorm done
@shiftBits			; exponent is in 9..15
	flt_loadBFromTopFPA0MantissaByte
@shiftLoop
	lsrb
	inca			; increment exponent
	cmpa	#EXPBIAS+8
	blo	@shiftLoop	; loop if exponent not yet 8
	bra	@store		; go store D as result
@denormDone
	flt_loadBFromTopFPA0MantissaByte
	bra	@store
@tooHigh
	ldb	#255
	bra	@store
@tooLow
	clrb
@store
	stb	[,s]		; get dest address from stack, store byte there
	puls	x,y,u,pc

        ENDC


	ENDSECTION
