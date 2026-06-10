	SECTION code

subDWordDWord	EXPORT


; Input: Stacked arguments: address of left dword, address of right dword.
;        X => Result dword.
; Output: DWord *U - *Y written to *X.
;         Sets N, Z, V, C.
; Preserves X, Y, U. Trashes D.
; Does not modify the stacked arguments.
;
subDWordDWord
	pshs	u,y
	ldu	6,s		; left
	ldy	8,s		; right

	ldd	2,u		; low word
	subd	2,y
	std	2,x

	ldd	,u		; high word
	sbcb	1,y
	sbca	,y
	std	,x

	puls	y,u,pc


	ENDSECTION
