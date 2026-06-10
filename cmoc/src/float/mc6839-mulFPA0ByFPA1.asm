        INCLUDE float.inc


mulFPA0ByFPA1 EXPORT


        SECTION code


fixMinusZeroAtX IMPORT


mulFPA0ByFPA1
        pshs    u,y
        leax    FPCB_SINGLE,pcr
        tfr     x,d                     ; point D to single-precision type
        leax    FP0ADDR,pcr             ; point X to result number
        leau    FP0ADDR,pcr             ; point U to 1st number
        leay    FP1ADDR,pcr             ; point Y to 2nd number
        lbsr    FPREG
        fcb     FMUL
        lbsr    fixMinusZeroAtX
        puls    y,u,pc


        ENDSECTION
