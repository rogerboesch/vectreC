        SECTION code

checkGuardVar EXPORT

; Checks a local static's guard variable, to determine if this variable
; needs to be initialized.
; This check is atomic.
; The locking is done by masking the interrupts.
; The interrupts are masked upon exit if they were masked upon entry.
; Input: X => Boolean byte to check.
; Output: Z=0 if the variable needs to be initialized.
;         X is preserved.
;         B is trashed.
;
checkGuardVar:
        pshs    cc              ; preserve F and I
        orcc    #$50            ; mask interrupts if they were not already masked
        ldb     ,x              ; load guard variable
        bne     @alreadyTrue
        inc     ,x              ; set guard to true
@alreadyTrue
        puls    cc              ; restore original interrupt masking state
        tstb                    ; set Z according to original guard value
        rts


        ENDSECTION
