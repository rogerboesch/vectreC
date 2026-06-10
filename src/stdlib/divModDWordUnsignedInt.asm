	SECTION code

divModDWordUnsignedInt	EXPORT

DIV32                   IMPORT


; Input: Pushed arguments: address of dword, value of unsigned int.
;        X => destination dword.
;		 B => 0 = caller wants quotient, non-zero = caller wants remainder.
; Preserves X.
;
divModDWordUnsignedInt
	pshs	u,y,x,b
	leas	-16,s
;
; 0,s: Copy of dividend (to avoid modifying the caller's).
; 4,s: Extension of divisor to 32 bits.
; 8,s: Quotient received from DIV32.
; 12,s: Remainder received from DIV32.
; 16,s: Pushed B. Boolean: 0 = caller wants quotient, non-zero = caller wants remainder.
; 17,s: Pushed X. Address of dword that receives the quotient.
; 19,s: Pushed Y.
; 21,s: Pushed U.
; 23,s: Return address of this subroutine.
; 25,s: Address of the 32-bit unsigned dividend.
; 27,s: 16-bit unsigned divisor.
;
	ldx	25,s		; address of dividend
	ldd	,x		; copy to 0,s
	std	,s
	ldd	2,x
	std	2,s
	leax	,s		; point to dividend
	ldd	27,s		; value of divisor
	std	6,s		; extend divisor to 32 bits
	clr	5,s
	clr	4,s
	leau	4,s		; point to 32-bit divisor
	leay	8,s		; point to quotient and remainder (8 bytes)
	lbsr	DIV32
	ldx	17,s		; retrieve destination address for quotient (pushed X)
	tst	16,s		; caller wants quotient or remainder? (pushed B)
	bne	@remainder
	leau	8,s
	bra	@copy
@remainder
	leau	12,s
@copy
	ldd	,u		; copy quotient to X
	std	,x
	ldd	2,u
	std	2,x
	leas	16,s
	puls	b,x,y,u,pc




	ENDSECTION
