        SECTION code

_memcmp         EXPORT

memcmpimpl      IMPORT


_memcmp
        leax    compareBytes,PCR
        lbra    memcmpimpl


compareBytes
        lda     ,x+
        cmpa    ,u+
        rts


        ENDSECTION
