        INCLUDE float.inc

	SECTION code

initSignedDWordFromSingle     EXPORT

initDWordFromSingle	IMPORT


; Input: D => address of source number. X => address of destination number.
;
initSignedDWordFromSingle

        IFDEF _CMOC_MC6839_

        pshs    u,y
        leay    FPCB_SINGLE,pcr
        exg     d,y             ; point Y to source real, point D to FPCB
        lbsr    FPREG
        fcb     FFIXD
        puls    y,u,pc

        ELSE

        orcc    #1
        lbra    initDWordFromSingle

        ENDC


	ENDSECTION
