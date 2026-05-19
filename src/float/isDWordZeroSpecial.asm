	SECTION code

isDWordZeroSpecial	EXPORT


; Input: 4,S (before call) => dword.
; Output: Z is set iff divisor is zero.
; Preserves X. Trashes D.
;
isDWordZeroSpecial
	pshs	x
	ldx	8,s	; address of dword to check
	ldd	,x
	bne	@done
	ldd	2,x
@done
	puls	x,pc


	ENDSECTION
