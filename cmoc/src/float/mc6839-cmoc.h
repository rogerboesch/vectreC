#ifndef _H_MC6839_CMOC_
#define _H_MC6839_CMOC_

#include <cmoc.h>


enum
{
    MAX_EXP_LENGTH = 4,
    MAX_FRACTION_LENGTH = 19,
    MAX_ASCII_OUTPUT_LENGTH = 38,
    DEFAULT_MAX_OUTPUT_DIGITS = 6,
};


typedef struct mc6839_bcd
{
    unsigned char exp_sign;
    unsigned char exp[MAX_EXP_LENGTH];
    unsigned char fraction_sign;
    unsigned char fraction[MAX_FRACTION_LENGTH];
    unsigned char fraction_digits;
} mc6839_bcd;


const unsigned char _CMOC_MINUS_HUGE_VALF[4] = { 0xFF, 0x80, 0x00, 0x00 };
const unsigned char _CMOC_NAN[4] = { 0xFF, 0xC0, 0x00, 0x00 };


void _CMOC_callMC6839BinDec(mc6839_bcd *bcd, const float *f);

char *_CMOC_convertBCDToASCIIDecimal(char out[MAX_ASCII_OUTPUT_LENGTH], mc6839_bcd *bcd, unsigned char maxOutputDigits);

float _CMOC_log2fOfPosValue(float x);



#endif  /* _H_MC6839_CMOC_ */
