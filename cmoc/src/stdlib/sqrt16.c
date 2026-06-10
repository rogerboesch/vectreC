#include <cmoc.h>

#define PURE_ASM

#ifdef PURE_ASM
asm
#endif
unsigned char
sqrt16(unsigned short value)
{
#ifndef PURE_ASM
    if (value == 0)
        return 0;

    unsigned short mask = 1 << 14;
    unsigned short result = 0;

    while (mask > value)
        mask >>= 2;

    while (mask != 0)
    {
        if (value >= result + mask)
        {
            value -= result + mask;
            result = (result >> 1) + mask;
        }
        else
            result >>= 1;
        mask >>= 2;
    }

    return (unsigned char) result;
#else
    asm
    {
        ldd     2,s             ; value
        beq     @return         ; return 0 in D
        leas    -4,s
; 0,s (word): mask: bit that shifts right by 2 bits during loop
; 2,s (word): result
; 4,s (word): return address
; 6,s (word): value
        ldd     #$4000          ; mask = 1 << 14;
        std     ,s  
        clr     2,s             ; result = 0;
        clr     3,s
        bra     @condFirstWhile
@firstWhileLoop
; D contains mask here. Shift it 2 bits right.
        lsra
        rorb
        lsra
        rorb
@condFirstWhile
        cmpd    6,s             ; mask > value?
        bhi     @firstWhileLoop
;
        bra     @condSecondWhile
@secondWhileLoop
; D contains mask here.
        addd    2,s             ; result + mask
        cmpd    6,s             ; compare with value
        bhi     @else
; Do value = value - (result + mask),
; i.e., value = value - D,
; i.e., value = - (D - value):
        subd    6,s             ; (result + mask) - value
        coma
        comb
        addd    #1              ; negate D, so now D = value - (result + mask)
        std     6,s             ; assign to value
;
        ldd     2,s             ; result
        lsra
        rorb                    ; result >> 1
        addd    ,s              ; add mask
        bra     @afterIfElse
@else
        ldd     2,s             ; result
        lsra
        rorb
@afterIfElse
        std     2,s             ; result
; Do mask >>= 2:
        ldd     ,s              ; mask
        lsra
        rorb
        lsra
        rorb
@condSecondWhile
        std     ,s              ; mask
        bne     @secondWhileLoop
;
        ldb     3,s             ; return low byte of result
        leas    4,s             ; discard locals
@return                         ; return value is in B here
    }
#endif
}
