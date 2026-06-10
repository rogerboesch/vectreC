        INCLUDE float.inc

        SECTION code

unpackSingleAndConvertToASCII EXPORT


; Input: X => Packed single-precision float.
;        U => Address of a 38-byte buffer that receives an ASCIIZ.
; Trashes D, X.
; Preserves U.
;
unpackSingleAndConvertToASCII
	flt_unpackFromXToFPA0
	flt_convertFPA0ToASCII
        rts


	ENDSECTION
