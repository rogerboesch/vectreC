	INCLUDE float.inc

	SECTION code

unpackXtoFPA0AndPrep	EXPORT
unpackXtoFPA1AndPrep    EXPORT


unpackXtoFPA0AndPrep
	flt_unpackFromXToFPA0
	bra	prepBinFloatOp

unpackXtoFPA1AndPrep
	flt_unpackFromXToFPA1

prepBinFloatOp
        flt_computeResultSign
	rts


	ENDSECTION
