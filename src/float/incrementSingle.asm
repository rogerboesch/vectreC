	SECTION code

incrementSingle	EXPORT


; Adds one 1 to the packed single-precision float at X.
; Preserves X.
;
incrementSingle
	pshs	u,y,x
	jsr	$BC14		; unpack into Basic's FPA0 (preserves X)
	leax	packed1,PCR
	jsr	$B9C2		; add number at X to FPA0 (trashes X)
	ldx	,s		; retrieve original number address
	jsr	$BC35		; pack FPA0 into X
	puls	x,y,u,pc
packed1
	fcb	$81		; packed 1.0
	fdb	0
	fdb	0




	ENDSECTION
