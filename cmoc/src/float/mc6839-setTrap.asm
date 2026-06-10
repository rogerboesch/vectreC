        INCLUDE float.inc

_setMC6839Trap EXPORT

FPCB_SINGLE IMPORT


        SECTION code


; void setMC6839Trap(void (*trapFunctionPointer)(void));
;
_setMC6839Trap
        ldd     2,s                     ; trapFunctionPointer
        std     FPCB_SINGLE+4,pcr
        rts


        ENDSECTION
