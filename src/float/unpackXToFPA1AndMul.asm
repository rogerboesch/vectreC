	INCLUDE float.inc

	SECTION code

unpackXToFPA1AndMul	EXPORT

unpackXtoFPA1AndPrep    IMPORT


; To be called by binOpSingleSingle.
; Input: X => packed single to be unpacked to FPA1.
;        FPA0 => factor of the multiplication to be done.
; Output: FPA0 = FPA0 * single at X.
;
unpackXToFPA1AndMul
	lbsr	unpackXtoFPA1AndPrep
	flt_mulFPA0ByFPA1
	rts


	ENDSECTION
