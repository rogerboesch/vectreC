	SECTION code

checkWhiteSpace EXPORT

_isspace        IMPORT


; Preserves A.
; Returns Z=0 if A contains a white space character.
;
checkWhiteSpace
        pshs    a
        clr     ,-s             ; promote char as int
        lbsr    _isspace
        leas    1,s
        puls    a,pc            ; restore A, return Z


	ENDSECTION
