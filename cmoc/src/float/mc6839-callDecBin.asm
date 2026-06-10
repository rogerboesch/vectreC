	INCLUDE float.inc


__CMOC_callMC6839DecBin EXPORT

MC6839_ROM      IMPORT
FPCB_SINGLE     IMPORT


        SECTION code


; void __CMOC_callMC6839DecBin(float *result, const mc6839_bcd *bcd);
; 
__CMOC_callMC6839DecBin
        pshs    u                   ; preserve stack frame pointer
; Stack here: saved U, return address; 4,s = result parameter; 6,s = bcd parameter.
        leax    FPCB_SINGLE,pcr     ; control block that describes the single-precision float type
                                    ; (this ,pcr gets changed to ,y on OS-9 by os9fixup.pl)
        tfr     x,d
        ldx     4,s                 ; result address
        ldu     6,s                 ; BCD input string
        lbsr    FPREG               ; call MC6839 routine that receives its arguments in registers
        fcb     DECBIN
        puls    u,pc


        ENDSECTION
