; Function that gets called when an exception occurs in the 6839 ROM.
; This function calls printf().
; A program can call setConsoleOutHook() to redirect printf() output.

	INCLUDE float.inc


mc6839_trap     EXPORT

_printf         IMPORT
_exit           IMPORT


        SECTION code


mc6839_trap
        pshs    u,y,x,d
        ldd     4*2,s                   ; return address
        pshs    d
        ; Push the 6 bytes of the FPCB:
        leax    FPCB_SINGLE,pcr
        ldd     4,x
        pshs    d
        ldd     2,x
        pshs    d
        ldd     ,x
        pshs    d
        ;
        leax    @trapmsg,PCR
        pshs    x
        lbsr    _printf
        leas    9*2,s
        ldd     #1
        lbsr    _exit
@trapmsg
        fcc     "*** MC6839 TRAP (FPCB=$%04x%04x%04x, RA=%p, D=$%04x, X=%p, Y=%p, U=%p) ***"
        fcb     10,0


        ENDSECTION
