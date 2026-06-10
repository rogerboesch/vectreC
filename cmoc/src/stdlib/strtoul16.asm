	SECTION code

_strtoul16	EXPORT

strtoul_init    IMPORT
strtoul_done    IMPORT


; unsigned long strtoul16(char *nptr, char **endptr);
;
; A hidden first argument is passed to point to the 32-bit return value slot.
;
_strtoul16
	pshs	u
	leau	,s
	clr	,-s		-1,U: boolean: string contains negative number
        lbsr    strtoul_init
@loop
	LDB	,X+		read next char from ASCII buffer
	CMPB	#'0
	LBLO	strtoul_done    stop reading at non-digit char
	CMPB	#'9
	BLS	@convert0to9
        ANDB    #$DF            convert lower-case to upper-case ($61..$66 -> $41..$46)
        CMPB    #'A
        LBLO    strtoul_done
        CMPB    #'F
        LBHI    strtoul_done
        SUBB    #'A'-10         char -= 'A' - 10 -> char = char - 'A' + 10 -> 10..15
        BRA     @bRegReady
@convert0to9
	SUBB	#'0		convert from ASCII '0'..'9' to 0..9
@bRegReady
; B now contains the nybble to be shifted into the accumulator
        lda     #4              shift dword left by 4 bits
        pshs    x               preserve char reader
	ldx	4,u		address of return value slot
@shiftBy1Bit
	lsl	3,x
	rol	2,x
	rol	1,x
	rol	,x
	deca
	bne	@shiftBy1Bit
; Low bybble of dword is now ready to receive nybble in B.
        orb     3,x
        stb     3,x
        puls    x               restore char reader
	bra	@loop
 

	ENDSECTION
