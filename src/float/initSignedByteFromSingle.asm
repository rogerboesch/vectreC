        INCLUDE float.inc

	SECTION code

initSignedByteFromSingle	EXPORT


; Input: D => address of source number. X => address of destination number.
;
initSignedByteFromSingle

        IFDEF _CMOC_MC6839_

initSignedWordFromSingle IMPORT
        pshs    x,b,a           ; save X that points to destination, create temp short with D
        leax    ,s              ; point X to temp short
        lbsr    initSignedWordFromSingle
        ldd     ,s++            ; pop resulting short
        cmpd    #-128
        blt     @tooLow
        cmpd    #127
        ble     @returnB        ; if not too high
        ldb     #127
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
	flt_loadAFromFPA0BiasedExp
        beq     @zero                   ; exponent is 0, so FPA0 is 0.0
	cmpa	#EXPBIAS+8		; is FPA0 >= 128?
	bhs	@tooHigh
	leax	packedMinus128,pcr
	flt_compareFPA0ToX		; compare FPA0 to -128
	blt	@tooLow
;
; Shift the mantissa right until the binary point is 8 bits from the left of the mantissa.
        flt_loadAFromFPA0BiasedExp
        suba    #EXPBIAS        ; real exponent in A (0..7); we want to increase it to 16
        bls     @zero
        flt_loadBFromTopFPA0MantissaByte
@shiftLoop
        lsrb
        inca
        cmpa    #8
        blo     @shiftLoop
; Absolute value of result is in FP0MAN. Apply the sign.
        flt_testFPA0Sign
        bpl     @store
        negb
        bra     @store
@zero
        clrb
	bra	@store
@tooHigh
        flt_testFPA0Sign
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

        ENDC


	ENDSECTION
