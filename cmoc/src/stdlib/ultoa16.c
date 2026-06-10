// ultoa16.c - CMOC's standard library functions.
//
// By Pierre Sarrazin <http://sarrazip.com/>.
// This file is in the public domain.

#include <cmoc.h>


char *_FinishIntegerToASCII(char *firstDigit, char *endOfString);


char *
ultoa16(unsigned long value, char *str)
{
    if (value == 0)
    {
        str[0] = '0';
        str[1] = '\0';
        return str;
    }

    char *writer = str;

    // Write the digits in reverse order, then reverse them.

    while (value > 0)
    {
        char lowNybble = (char) value & 0x0F;
        char hexDigit = (lowNybble <= 9 ? '0' + lowNybble : (char) ('A' - 10) + lowNybble);
        *writer++ = hexDigit;
        value >>= 4;
    }
    
    return _FinishIntegerToASCII(str, writer);
}
