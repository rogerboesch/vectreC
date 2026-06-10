        INCLUDE float.inc


subFPA0FromFPA1 EXPORT


        SECTION code


subFPA0FromFPA1
        pshs    u,y
        leax    FPCB_SINGLE,pcr
        tfr     x,d                     ; point D to single-precision type
        leax    FP0ADDR,pcr             ; point X to result number
        leau    FP1ADDR,pcr             ; point U to 1st number
        leay    FP0ADDR,pcr             ; point Y to 2nd number
        lbsr    FPREG
        fcb     FSUB
        puls    y,u,pc


        ENDSECTION
