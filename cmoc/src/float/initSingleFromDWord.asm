	INCLUDE float.inc

	SECTION code

initSingleFromDWord	EXPORT


; Initializes the single-precision float at X from the dword whose address is in D.
; The carry must be the signedness flag (0 for unsigned dword, 1 for signed).
; Does not change the source dword.
; Trashes D, FPA0.
;
initSingleFromDWord

        IFDEF _CMOC_MC6839_

        bcs     @ffltd                  ; if from signed dword, just call FFLTD
        ; From unsigned dword.
        ; If dword <= $7FFFFFFF, then use FFLTD; else dword -= $80000000; use FFLTD; add 2^31 to float.
        pshs    u,y                     ; save stack frame ptr and data segment ptr
        tfr     d,y                     ; Y => dword
        ldd     ,y                      ; high word of dword
        bpl     @dwordIs31Bits          ; dword < $80000000
        ; Substract $80000000 from dword; store the result in a temporary dword in the stack.
        suba    #$80
        leas    -4,s                    ; create temp dword
        std     ,s
        ldd     2,y
        std     2,s
        tfr     s,d                     ; point D to temp dword
        ldy     4,s                     ; restore data segment ptr
        bsr     @ffltd                  ; converts temp dword to float at X
        leas    4,s                     ; discard temp dword
        ; Add 2^31 to float at X.
        leau    @twoTo31,PCR
        leay    ,x
        lbsr    FPREG
        fcb     FADD
        bra     @done
@dwordIs31Bits
        tfr     y,d                     ; point D to the caller's dword
        ldy     ,s                      ; restore data segment ptr
        bsr     @ffltd
@done
        puls    y,u,pc
@twoTo31
        fqb     $4F000000
;
; Converts the dword at D to a float at X.
; Y must point to the data segment on OS-9.
; Output: D => FPCB.
; Preserves Y.
;
@ffltd
        pshs    y                       ; preserve data segment ptr
        leay    FPCB_SINGLE,pcr         ; this ,pcr gets changed to ,y on OS-9 by os9fixup.pl
        exg     d,y                     ; point Y to signed dword, point D to FPCB
        lbsr    FPREG
        fcb     FFLTD                   ; page A-14 of MC6839 manual
        puls    y,pc

        ELSE

	pshs	u,y,x,cc
	flt_clrVALTYP
	tfr	d,x			; X => dword
	ldd	2,x			; copy dword to mantissa
	std	FP0MAN+2
	ldd	,x
	std	FP0MAN
; A is now high byte of mantissa, as expected by $BC84
	ldb	#EXPBIAS+32
	flt_storeBInFPA0BiasedExp
	flt_clrFPSBYT
	flt_clearFPA0Sign
	puls	cc			; test signedness flag
	bcc	@unsigned
	suba	#$80			; set carry if non-negative mantissa
	bra	@normalize
@unsigned
	orcc	#1			; consider mantissa non-negative
@normalize
	flt_normalizeFPA0
	puls	x
	flt_packFPA0ToX
	puls	y,u,pc

        ENDC


	ENDSECTION
