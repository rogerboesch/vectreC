	INCLUDE float.inc


__CMOC_callMC6839BinDec EXPORT

MC6839_ROM      IMPORT
FPCB_SINGLE     IMPORT


        SECTION code


; void _CMOC_callMC6839BinDec(mc6839_bcd *bcd, const float *f);
; 
__CMOC_callMC6839BinDec
        pshs    y,u                     ; preserve data segment pointer and stack frame pointer
; Stack here: saved U, saved Y, return address; 6,s = bcd parameter; 8,s = f parameter.
        leax    FPCB_SINGLE,pcr         ; control block that describes the single-precision float type
                                        ; (this ,pcr gets changed to ,y on OS-9 by os9fixup.pl)
        tfr     x,d
        ldx     6,s                     ; mc6839_bcd struct to write the result to
        ldy     8,s                     ; address of the float to convert
        ldu     #19                     ; precision (number of digits in MC6839 mantissa)
        lbsr    FPREG                   ; call MC6839 routine that receives its arguments in registers
        fcb     BINDEC
        puls    u,y,pc


        ENDSECTION
