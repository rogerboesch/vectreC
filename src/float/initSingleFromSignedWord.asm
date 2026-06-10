	INCLUDE float.inc

	SECTION code

initSingleFromSignedWord	EXPORT


; Initializes the single-precision float at X with the signed word in D.
;
initSingleFromSignedWord

        IFDEF _CMOC_MC6839_

        pshs    u,y,d                   ; save signed word (D) in stack
        leay    FPCB_SINGLE,pcr
        tfr     y,d
        leay    ,s                      ; point to signed word
        lbsr    FPREG
        fcb     FFLTS                   ; page A-14 of MC6839 manual
        puls    d,y,u,pc

        ELSE

	pshs	u,y,x
	flt_loadSignedDIntoFPA0
	puls	x
	flt_packFPA0ToX
	puls	y,u,pc

        ENDC


	ENDSECTION
