	SECTION code

passLeadingWhiteSpaceChars      EXPORT

checkWhiteSpace                 IMPORT

; Input: X => ASCIIZ.
; Output: X advanced to the first non white space character,
;         as per isspace().
;
passLeadingWhiteSpaceChars
@loop
        lda     ,x+
        lbsr    checkWhiteSpace
        bne     @loop                   ; branch if A is white space
        leax    -1,x
        rts


	ENDSECTION
