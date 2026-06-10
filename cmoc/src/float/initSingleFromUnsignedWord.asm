	INCLUDE float.inc

	SECTION code

initSingleFromUnsignedWord	EXPORT

loadUnsignedDInFPA0             IMPORT


; Initializes the single-precision float at X with the unsigned word in D.
;
initSingleFromUnsignedWord

        IFDEF _CMOC_MC6839_

        pshs    u,y,d
        clr     ,-s
        clr     ,-s                     ; ,s is now a signed dword (big endian) equal to the initial D
        leay    FPCB_SINGLE,pcr
        tfr     y,d
        leay    ,s                      ; point Y to the signed dword in the stack
        lbsr    FPREG
        fcb     FFLTD                   ; page A-14 of MC6839 manual
        leas    4,s                     ; discard the signed dword, including the saved D
        puls    y,u,pc

        ELSE

	pshs	u,y,x
	lbsr	loadUnsignedDInFPA0	; load D (unsigned) into FPA0
	puls	x
	flt_packFPA0ToX
	puls	y,u,pc

        ENDC


	ENDSECTION
