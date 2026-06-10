#include <cmoc.h>


asm int
isdigit(int c)
{
    //return c >= '0' && c <= '9';
    asm
    {
        ldd     2,s         ; c 
        cmpd    #'0'
        blt     @false
        cmpd    #'9'
        bgt     @false
        ldb     #1          ; A is already 0
        rts
@false
        clra
        clrb
    }
}
