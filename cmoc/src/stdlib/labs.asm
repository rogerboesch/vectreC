        SECTION code

_labs   EXPORT

negateDWord     IMPORT


; long int abs(long int j);
;
; Address of return value is at 2..3,s.
; j is at 4..7,s upon entry.
;
_labs
        tst     4,s             ; test sign bit of MSB of j (big endian)
        bpl     @done
        leax    4,s             ; point to j
        lbsr    negateDWord     ; negate long at X (preserves X)
@done
; Return long at 4,s.
        ldx     2,s             ; point to long return value
        ldd     4,s             ; MSW of result
        std     ,x
        ldd     6,s             ; LSW of result
        std     2,x
        rts


        ENDSECTION
