        SECTION code

_vsprintf       EXPORT

CHROUT          IMPORT
_vprintf        IMPORT
chrtomem        IMPORT
chrtomem_writer IMPORT


* int vsprintf(char *str, const char *format, va_list ap);
*
_vsprintf
        ldx     CHROUT,pcr
        stx     vsprintf_oldCHROUT,pcr  preserve initial output routine address
        leax    chrtomem,pcr            install chrtomem as destination of printf()
        stx     CHROUT,pcr

        ldx     ,s++                    remove return address in caller of sprintf()
        stx     vsprintf_retaddr,pcr    preserve it

        ldx     ,s++                    remove destination buffer
        stx     chrtomem_writer,pcr     make chrtomem write to destination buffer

        lbsr    _vprintf                print into destination buffer via chrtomem, using va_list

        clr     [chrtomem_writer,pcr]   terminate resulting string

        ldx     vsprintf_oldCHROUT,pcr  restore initial output routine address
        stx     CHROUT,pcr

        leas    -2,s                    restore stack slot for destination buffer pointer (b/c C calling convention)

        jmp     [vsprintf_retaddr,pcr]  return to caller of sprintf()


        ENDSECTION


        SECTION bss
                
vsprintf_retaddr        RMB     2       used by _vsprintf
vsprintf_oldCHROUT      RMB     2       used by _vsprintf

        ENDSECTION

