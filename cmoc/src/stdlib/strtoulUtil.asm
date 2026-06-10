	SECTION code


strtoul_init    EXPORT
strtoul_done    EXPORT

_isspace        IMPORT
negateDWord     IMPORT

; Routine to be called by _strtoul10 and _strtoul16.
; See those functions for the layout of the stack frame pointed to by U.
;
strtoul_init:
	ldx	4,u		address of return value slot
	clra
	clrb
	std	,x		clear return value (accumulator)
	std	2,x

	ldx	6,u		nptr

; Pass any leading white spaces.
; Note that A is 0 upon entering this loop.
@skipWhiteSpaces
        ldb     ,x+
        pshs    b,a                 ; pass character as int to C function isspace()
        lbsr    _isspace            ; CMOC's implementation of isspace() does not modify X
        leas    2,s                 ; does not affect Z, which reflects value returned by CMOC's isspace()
        bne     @skipWhiteSpaces    ; if character was white space, go check for more; also, A is 0 at this point

	ldb	,-x             ; load first character that follows white space
	cmpb	#'-		negative number?
	bne	@checkPlus	no
	inc	-1,u            ; set sign boolean to true
        bra     @passSignAndReturn
@checkPlus
        cmpb    #'+
        bne     @return
@passSignAndReturn
        leax    1,x
@return
        rts


; Routine that finishes _strtoul10 and _strtoul16.
; Must be jumped to with LBRA, not LBSR.
;
strtoul_done:
	leax	-1,x		go back to last non-digit char
	stx	[8,U]		return that address in *endptr

	ldx	4,u		address of return value slot

; Negate the result if minus sign was read.
	tst	-1,u
	beq	@notNeg
	lbsr	negateDWord
@notNeg
	leas	,u
	puls	u,pc


	ENDSECTION
