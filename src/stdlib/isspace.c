#include <cmoc.h>


// This implementation does not modify X.
// It does not modify 'c' in the stack.
// It returns 0 or 1 in B, and CC reflects this, and A is guaranteed to be 0.
//
asm int
isspace(int c)
{
    asm
    {
        ldd     2,s         ; argument 'c'
        cmpd    #9
        blo     @notSpace
        cmpd    #13
        bls     @space
        cmpd    #32
        beq     @space
@notSpace
        clrb
        clra                ; leave with Z=1
        rts
@space
        clra
        ldb     #1          ; leave with Z=0
    }
}
