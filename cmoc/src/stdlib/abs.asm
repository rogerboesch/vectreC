        SECTION code

_abs    EXPORT


; int abs(int j);
;
_abs
        ldd     2,s         ; j
        bge     @done       ; if j >= 0
        clra
        clrb
        subd    2,s         ; 0 - j
@done
        rts


        ENDSECTION
