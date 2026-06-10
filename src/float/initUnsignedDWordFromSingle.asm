        INCLUDE float.inc

	SECTION code

initUnsignedDWordFromSingle     EXPORT

initDWordFromSingle	IMPORT


; Input: D => address of source number. X => address of destination number.
;
initUnsignedDWordFromSingle

        IFDEF _CMOC_MC6839_

; The FFIXD operation only supports signed 32-bit integers.
; So we check if the single is too large for a 32-bit integer,
; and return 0xFFFFFFFF in that is the case.
; Then, if the single is larger than 0x7FFFFFFF, we subtract
; 0x80000000 from it, convert the rest, then add 0x80000000
; to the resulting 32-bit integer.
;
        pshs    u,y,x                   ; save X that points to destination ulong
        leas    -5,s                    ; 0,s = tempFloat; 4,s = add80000000 (bool); 5,s = addr of dest ulong (saved X)
        clr     4,s                     ; set add80000000 to false
        tfr     d,u                     ; point U to float (arg1 of FCMP)
        tst     ,u                      ; test sign of float
        bmi     @tooLow
; Compare the float with 0xFFFFFFFF.
        leax    FPCB_SINGLE,pcr
        tfr     x,d                     ; point D to FPCB
        ldx     #0                      ; compare single with single
        leay    @single_FFFFFFFF,PCR    ; arg2
        lbsr    FPREG
        fcb     FCMP
        bgt     @tooHigh                ; value does not fit 32 bits
; Compare the float (pointed to by U) with 0x80000000.
        leay    @single_80000000,PCR    ; arg2
        lbsr    FPREG
        fcb     FCMP
        blt     @convertF               ; if lt, float can be converted to ulong
; Float too high for FFIXD. Subtract $80000000 and convert rest.
; U still points to the caller's float.
; D still points to the FPCB.
        leax    ,s                      ; tempFloat: will receive result of substraction
        leay    @single_80000000,PCR    ; arg2
        lbsr    FPREG
        fcb     FSUB
        inc     4,s                     ; set add80000000 to true: remember to add $80000000 to conversion result
        leay    ,s                      ; convert tempFloat instead of caller's float
        bra     @convertY
@convertF
; U still points to the caller's float.
; D still points to the FPCB.
        leay    ,u
@convertY
        ldx     5,s                     ; caller's X, i.e., address of destination ulong
; Convert the float at Y to a signed long at X, knowing that the float is >= 0.
        lbsr    FPREG
        fcb     FFIXD
        bra     @return
@tooHigh
        ldx     5,s                     ; caller's X, i.e., address of destination ulong
        ldd     #$FFFF
        std     ,x
        std     2,x
        bra     @return
@tooLow
        clra
        clrb
        std     ,x
        std     2,x
@return
        tst     4,s                     ; set Z on add80000000
;
        leas    5,s                     ; discard local storage
        puls    x,y,u
;
        beq     @done                   ; if add80000000 was false (uses Z from TST instruction)
        ldb     ,x                      ; add $80 to MSB of destination ulong
        addb    #$80
        stb     ,x
@done
        rts
;
; Read-only MC6839 single-precision constants.
@single_80000000 fqb $4F000000
@single_FFFFFFFF fqb $4F800000

        ELSE

        andcc   #$FE
        lbra    initDWordFromSingle

        ENDC


	ENDSECTION
