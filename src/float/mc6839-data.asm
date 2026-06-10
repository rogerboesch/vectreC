	INCLUDE float.inc

        IFDEF _CMOC_MC6839_

FPCB_SINGLE     EXPORT
FP0ADDR         EXPORT
FP1ADDR         EXPORT

nop_handler     IMPORT

        SECTION rwdata

; MC6839 floating point control block.
; Defined with an lwasm struct.
;
FPCB_SINGLE
        fcb     %00000010       ; control byte; bits 5-7: single-precision float type; bits 1-2: round toward 0 (needed for conversion to integer)
        fcb     $00             ; trap enable byte
        fcb     $00             ; status byte
        fcb     $00             ; secondary status byte
        fdb     nop_handler     ; pointer to default do-nothing trap routine

        ENDSECTION


        SECTION bss

; Unpacked float point accumulators.
FP0ADDR         rmb     4
FP1ADDR         rmb     4

        ENDSECTION

        ENDC
