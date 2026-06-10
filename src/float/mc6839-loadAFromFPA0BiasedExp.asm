        INCLUDE float.inc

	SECTION code

loadAFromFPA0BiasedExp EXPORT

; Only changes A.
; Output: A is the biased exponent of FPA0, and Z reflects this byte value.
;
loadAFromFPA0BiasedExp
        lda     FP0ADDR,pcr             ; load 7 high bits of biased exponent
        lsla                            ; make room for 8th bit of exponent
        tst     FP0ADDR+1,pcr           ; test high bit of 2nd byte
        bpl     @loaded                 ; if 0
        inca
@loaded
        tsta
        rts


	ENDSECTION
