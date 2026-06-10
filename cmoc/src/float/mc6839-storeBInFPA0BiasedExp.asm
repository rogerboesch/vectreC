        INCLUDE float.inc

	SECTION code

storeBInFPA0BiasedExp EXPORT

; Does not change B. Affects CC.
;
storeBInFPA0BiasedExp
        pshs    b,a
        clra
        exg     a,b             ; biased exponent now in A
        lsra
        rorb                    ; biased exponent in D now shifted one bit right
        pshs    b,a             ; preserve D has a mask
        ldd     FP0ADDR,pcr     ; load high word of accumulator
        anda    #$80            ; erase existing biased exponent
        andb    #$7F
        ora     ,s+             ; OR in the new biased exponent
        orb     ,s+
        std     FP0ADDR,pcr
        puls    a,b,pc


	ENDSECTION
