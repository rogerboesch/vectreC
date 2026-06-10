	INCLUDE float.inc

	SECTION code

initSingleFromSignedDWord       EXPORT

initSingleFromDWord	        IMPORT


initSingleFromSignedDWord
        orcc    #1
        lbra    initSingleFromDWord


	ENDSECTION
