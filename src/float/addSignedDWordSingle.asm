	SECTION code

addSignedDWordSingle	EXPORT

loadSignedDWordInFPA0   IMPORT
addFPA0FPA1             IMPORT

addSignedDWordSingle
	pshs	u,y,x
	ldx	8,s		; left (unsigned dword)
	lbsr	loadSignedDWordInFPA0
	ldx	10,s		; right (single)
	jsr	$BB2F		; unpack from X to FPA1
	lbra	addFPA0FPA1




	ENDSECTION
