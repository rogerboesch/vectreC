        SECTION code

_strrchr        EXPORT

_strlen         IMPORT

* char *strrchr(const char *s, int c);
*
_strrchr:
        ldx     2,s             string
@findEnd
	tst	,x+
	bne	@findEnd
        leax    -1,x            go back to terminating NUL
        tst     5,s             searching for NUL?
        beq     @found
@findChar
        cmpx    2,s             does X point to start of string?
        beq     @notFound       if yes, 'c' not found
        lda     ,-x             go back and load char
        cmpa    5,s             compare with 'c' (ignore MSB)
        bne     @findChar
@found
        tfr     x,d             return address where char found
        rts
@notFound
        clra                    not found: return NULL
        clrb
        rts


        ENDSECTION
