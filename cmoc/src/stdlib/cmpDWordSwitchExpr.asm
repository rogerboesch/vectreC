	SECTION code

cmpDWordSwitchExpr      EXPORT


cmpDWordDWord           IMPORT


; Compare the 32-bit integer value at X with the one designated by D.
; Input: X => 32-bit value.
;        D => offset of a 32-bit value relative to the return address of the call to this routine.
; Output: Sets N, Z, V, C.
; Preserves X, Y, U. Trashes D.
; Example:
;       leax    someValue,pcr
;       ldd     #otherValue-(*+3+3)     ; first 3 is size of LDD, second 3 is size of LBSR;
                                        ; (*+3+3) is the address of the LBEQ instruction
;       lbsr    cmpDWordSwitchExpr      ; pushes a return address, which will be used with D
;       lbeq    somewhere
;
cmpDWordSwitchExpr
        pshs    x               ; pass to cmpDWordDWord
        ldx     2,s             ; return address of this call
        leax    d,x             ; pointer to other 32-bit value
        pshs    x               ; pass to cmpDWordDWord
        lbsr    cmpDWordDWord   ; preserves its stacked arguments
        leas    2,s             ; discard pointer to other 32-bit value
        puls    x,pc            ; restore original X, return


	ENDSECTION
