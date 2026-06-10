        SECTION constructors


unpackSingleAndConvertToASCII IMPORT
unpackSingleAndConvertToASCII_hook IMPORT
singlePrecisionSize IMPORT

        IFDEF _CMOC_MS_FLOAT_
        leax    unpackSingleAndConvertToASCII,PCR
        stx     unpackSingleAndConvertToASCII_hook,pcr
        ldb     #5
        stb     singlePrecisionSize,pcr
        ENDC

        IFDEF _CMOC_MC6839_
        ldb     #4
        stb     singlePrecisionSize,pcr
        ENDC


        ENDSECTION
