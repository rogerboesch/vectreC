        INCLUDE float.inc

	SECTION code

initUnsignedWordFromSingle	EXPORT


; Input: X => Address of the 16-bit result.
;        D => Address of the source real.
;
initUnsignedWordFromSingle

        IFDEF _CMOC_MC6839_

        pshs    u,y,x           ; save X that points to destination
        leas    -4,s            ; temporary 32-bit integer
        leax    ,s              ; have conversion write to it
        leay    FPCB_SINGLE,pcr
        exg     d,y             ; point Y to source real, point D to FPCB
        lbsr    FPREG
        fcb     FFIXD           ; convert single to signed long
        ldd     ,s              ; check sign and load top word of resulting long
        bmi     @tooLow         ; if long < 0
        bne     @tooHigh        ; if top word > 0, then long > 0xFFFF
        ldd     2,s             ; low word of long
        bra     @returnD
@tooLow
        clra                    ; store 0 at destination
        clrb
        bra     @returnD
@tooHigh
        ldd     #$FFFF          ; store max unsigned word at destination
@returnD
        leas    4,s             ; discard temporary 32-bit integer
        std     [,s]            ; store at address saved in stack by PSHS
        puls    x,y,u,pc

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
	cmpa	#EXPBIAS+16     ; is FPA0 >= 65536?
	bhi	@tooHigh	; if yes
; Denormalize FPA0 until exponent is 16.
	beq	@denormDone	; if exponent is 16, denorm done
	cmpa	#EXPBIAS+8      ; if exponent is in 9..15
	bhi	@shiftBits	; then go shift mantissa right by 1 to 7 bits
; Exponent is in 1..8. Shift mantissa right by 8 bits.
	ldb	FP0MAN		; load high byte of mantissa
	stb	FP0MAN+1	; shift it 8 bits right
	clr	FP0MAN		; clear high byte of mantissa
	adda	#8		; exponent is now 8 more than initially (now 9..16)
	cmpa	#EXPBIAS+16
	beq	@denormDone
@shiftBits			; exponent is in 9..15
	ldx	#0
	tfr	a,b
	abx			; X = biased exponent
	ldd	FP0MAN		; load high 16 bits of mantissa
@shiftLoop
	lsra			; shift D right one bit
	rorb
	leax	1,x		; increment exponent
	cmpx	#EXPBIAS+16
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

        ENDC


	ENDSECTION
