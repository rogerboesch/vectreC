        INCLUDE float.inc


divFPA1ByFPA0 EXPORT


        SECTION code


fixMinusZeroAtX IMPORT


divFPA1ByFPA0
        pshs    u,y
        leax    FPCB_SINGLE,pcr
        tfr     x,d                     ; point D to single-precision type
        leax    FP0ADDR,pcr             ; point X to result number
        leau    FP1ADDR,pcr             ; point U to dividend
        leay    FP0ADDR,pcr             ; point Y to divisor
        lbsr    FPREG
        fcb     FDIV                    ; page A-12 of MC6839 manual
        lbsr    fixMinusZeroAtX
        puls    y,u,pc


        ENDSECTION
