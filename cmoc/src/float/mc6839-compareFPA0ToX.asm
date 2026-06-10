        INCLUDE float.inc


compareFPA0ToX EXPORT


        SECTION code

; Input: X => right-side number.
; N.B.: The Y register is used to global variables under OS-9.

compareFPA0ToX
        pshs    u,y,x                   ; save frame ptr, data segment ptr, caller's X
        leax    FPCB_SINGLE,pcr
        tfr     x,d                     ; point D to single-precision type
        ldx     #0                      ; compare single with single
        leau    FP0ADDR,pcr             ; point U to left number
        puls    y                       ; load Y with caller's X
        lbsr    FPREG
        fcb     FCMP                    ; page A-10 of MC6839 manual
        beq     @equal
        bgt     @greater
        ldb     #-1                     ; lower
        bra     @done
@greater
        ldb     #1
        bra     @done
@equal
        clrb
@done
        puls    y,u,pc


        ENDSECTION
