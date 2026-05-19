        INCLUDE float.inc

	SECTION code

unpackXtoFPA0AndPrep	EXPORT
unpackXtoFPA1AndPrep    EXPORT


unpackXtoFPA0AndPrep
	jsr	$BC14		; unpack from X to FPA0
	bra	prepBinFloatOp

unpackXtoFPA1AndPrep
	jsr	$BB2F		; unpack from X to FPA1

prepBinFloatOp
        ; Compute sign of result, as in $BB2F.
        ldb     FP0SGN
        eorb    FP1SGN
        stb     RESSGN

        lda     FP1EXP
        ldb     FP0EXP          ; as in $BB2F; sets N and Z
	rts




	ENDSECTION
