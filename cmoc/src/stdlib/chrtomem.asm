        SECTION code

chrtomem        EXPORT

* Writes register A at the address given by chrtomem_writer,
* which gets incremented by 1.
*
chrtomem:
        pshs    x
        ldx     chrtomem_writer,pcr
        sta     ,x+
        stx     chrtomem_writer,pcr
        puls    x,pc

        ENDSECTION

        SECTION bss

chrtomem_writer EXPORT

chrtomem_writer RMB     2       used by chrtomem

        ENDSECTION
