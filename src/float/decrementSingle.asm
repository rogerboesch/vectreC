	INCLUDE float.inc

	SECTION code

decrementSingle	EXPORT


; Subtracts one 1 from the packed single-precision float at X.
; Preserves X.
;
decrementSingle
	pshs	u,y,x
	flt_unpackFromXToFPA0
	leax	@packedMinus1,PCR
	flt_addNumberAtXToFPA0
	ldx	,s		; retrieve original number address
	flt_packFPA0ToX
	puls	x,y,u,pc
@packedMinus1
        IFDEF _CMOC_MC6839_
        fqb     $BF800000
        ELSE
	fcb	$81		; packed -1.0
	fdb	$8000
	fdb	0
        ENDC


	ENDSECTION
