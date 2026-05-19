	SECTION code

addSingleSignedDWord	EXPORT

loadSignedDWordInFPA0   IMPORT
addFPA0FPA1             IMPORT


addSingleSignedDWord
	pshs	u,y,x
	ldx	10,s		; right (unsigned dword)
	lbsr	loadSignedDWordInFPA0
	ldx	8,s		; left (single)
	jsr	$BB2F		; unpack from X to FPA1
	lbra	addFPA0FPA1




	ENDSECTION
