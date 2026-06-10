	SECTION code

_strtoul10	EXPORT

strtoul_init            IMPORT
strtoul_done            IMPORT
mulDWordUnsignedInt     IMPORT


; unsigned long strtoul10(char *nptr, char **endptr);
;
; A hidden first argument is passed to point to the 32-bit return value slot.
;
_strtoul10
	pshs	u
	leau	,s
	clr	,-s		-1,U: boolean: string contains negative number
        lbsr    strtoul_init
@loop
	LDB	,X+		read next char from ASCII buffer
	CMPB	#'0
	LBLO	strtoul_done    stop reading at non-digit char
	CMPB	#'9
	LBHI	strtoul_done	stop reading at non-digit char
	SUBB	#'0		convert from ASCII '0'..'9' to 0..9
	CLRA
	pshs	x,b		preserve char reader, new digit
; Call mulDWordUnsignedInt to multiply accumulator by 10.
	ldb	#10
	pshs	b,a
	ldx	4,u		address of return value slot
	pshs	x
	lbsr	mulDWordUnsignedInt	preserves X, trashes D
	leas	4,s
;
	puls	b		restore new digit
; Add B to accumulator.
	clra
	ldx	4,u		address of return value slot
	addd	2,x		add low word of acc, sets carry
	std	2,x
	ldd	,x		get high word of acc
	adcb	#0		add carry
	adca	#0
	std	,x
;
	puls	x		restore char reader
	bra	@loop


	ENDSECTION
