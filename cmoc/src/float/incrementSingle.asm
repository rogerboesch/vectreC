	INCLUDE float.inc

	SECTION code

incrementSingle	EXPORT


; Adds one 1 to the packed single-precision float at X.
; Preserves X.
;
incrementSingle
	pshs	u,y,x
	flt_unpackFromXToFPA0
	leax	@packed1,PCR
	flt_addNumberAtXToFPA0
	ldx	,s		; retrieve original number address
	flt_packFPA0ToX
	puls	x,y,u,pc
@packed1
        IFDEF _CMOC_MC6839_
        fqb     $3F800000
        ELSE
	fcb	$81		; packed 1.0
	fdb	0
	fdb	0
        ENDC


	ENDSECTION
