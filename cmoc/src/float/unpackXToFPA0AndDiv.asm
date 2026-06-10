	INCLUDE float.inc

	SECTION code

unpackXToFPA0AndDiv	EXPORT
unpackXToFPA1AndDiv	EXPORT

unpackXtoFPA0AndPrep    IMPORT
unpackXtoFPA1AndPrep    IMPORT


; To be called by binOpSingleSingle.
; Input: X => packed single to be unpacked to FPA1.
;        FPA0 => factor of the division to be done.
; Output: FPA0 = FPA0 / single at X.
;
unpackXToFPA0AndDiv
	lbsr	unpackXtoFPA0AndPrep
	flt_divFPA1ByFPA0
	rts

unpackXToFPA1AndDiv
	lbsr	unpackXtoFPA1AndPrep
	flt_divFPA1ByFPA0
	rts



	ENDSECTION
