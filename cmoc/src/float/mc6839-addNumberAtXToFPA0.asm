        INCLUDE float.inc


addNumberAtXToFPA0 EXPORT


        SECTION code


addNumberAtXToFPA0
        pshs    u,y
        leau    ,x                      ; point U to 1st number
        leax    FPCB_SINGLE,pcr
        tfr     x,d                     ; point D to single-precision type
        leax    FP0ADDR,pcr             ; point X to result number
        leay    FP0ADDR,pcr             ; point Y to 2nd number
        lbsr    FPREG
        fcb     FADD                    ; see page A-4 of MC6839 manual
        puls    y,u,pc


        ENDSECTION
