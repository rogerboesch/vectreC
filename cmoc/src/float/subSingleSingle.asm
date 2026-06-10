	INCLUDE float.inc

	SECTION code

subSingleSingle	EXPORT

binOpSingleSingle       IMPORT
unpackXtoFPA1AndPrep    IMPORT


; Subtracts two numbers and writes the result at a third location.
; Synopsis:
;	pshs	rightOpAddr
;	pshs	leftOpAddr
;	leax	result,PCR
;	lbsr	subSingleSingle
;	leas	4,s
; Preserves X.
;
subSingleSingle
	pshs	u,y,x
	leau	@unpackXToFPA1AndSub,PCR
	lbsr	binOpSingleSingle
	puls	x,y,u,pc
;
@unpackXToFPA1AndSub
	lbsr	unpackXtoFPA1AndPrep
	flt_subFPA0FromFPA1
	rts


	ENDSECTION
