#include "mc6839-cmoc.h"


#ifdef DUMP_BCD
static void
dumpBCD(const mc6839_bcd *bcd)
{
    printf("BCD: S=$%02x FD=%u ", bcd->fraction_sign, bcd->fraction_digits);
    printf("EXP: $%02x ", bcd->exp_sign);
    for (unsigned char j = 0; j < MAX_EXP_LENGTH; ++j)
        printf("%u", bcd->exp[j]);
    printf("\n     ");
    for (unsigned char j = 0; j < MAX_FRACTION_LENGTH; ++j)
        printf("%u", bcd->fraction[j]);
    printf("\n");
    //printf("     0123456789ABCDEF012\n");
}
#endif


static const unsigned char huge_valf[] = { 0x7F, 0xFF, 0xFF, 0xFF };


static float
getHugeVal(unsigned char negative)
{
    float ret = * (float *) huge_valf;
    return negative ? -ret : ret;
}


void _CMOC_callMC6839DecBin(float *result, const mc6839_bcd *bcd);


#if 0
static asm void *
get_FPCB_SINGLE(void)
{
    asm
    {
FPCB_SINGLE IMPORT
        #ifdef OS9
        leax    FPCB_SINGLE,y
        #else
        leax    FPCB_SINGLE,pcr
        #endif
        tfr     x,d
    }
}
#endif


// TODO: Support exponents.
//
float
strtof(_CMOC_CONST_ char *nptr, char **endptr)
{
    //printf("#strtof(nptr='%s' %p, endptr=%p)\n", nptr, nptr, endptr);

    // Pass leading white space characters.
    while (isspace(*nptr))
        ++nptr;

    mc6839_bcd bcd;
    memset(&bcd, 0, sizeof(bcd));

    if (nptr[0] == '-')
    {
        bcd.fraction_sign = 0x0F;
        ++nptr;
    }

    // Count the number of digits before the 'E' or the end.
    size_t numIntDigits = 0xFFFF;
    size_t numDigits = 0;
    const char *p;
    for (p = nptr; ; ++p)
    {
        if (*p == '.')
        {
            if (numIntDigits != 0xFFFF)  // already seen a point, this one ends the number
                break;
            numIntDigits = numDigits;
        }
        else if (isdigit(*p))
            ++numDigits;
        else
            break;
    }
    // p now points after the mantissa; an exponent may follow.

    if (numIntDigits == 0xFFFF)  // if point seen
        numIntDigits = numDigits;
    if (numIntDigits > MAX_FRACTION_LENGTH)
        return getHugeVal(bcd.fraction_sign != 0);
    if (numDigits > MAX_FRACTION_LENGTH)
        numDigits = MAX_FRACTION_LENGTH;  // ignore some decimal
    size_t numFracDigits = numDigits - numIntDigits;
    bcd.fraction_digits = (unsigned char) (numFracDigits > 255 ? 255u : numFracDigits);

    // Fill bcd.fraction[].
    for (size_t i = MAX_FRACTION_LENGTH - numDigits; i != MAX_FRACTION_LENGTH; ++nptr)
        if (*nptr != '.')
            bcd.fraction[i++] = (unsigned char) (*nptr - '0');

    // Jump to the (possible) exponent part.
    nptr = p;
    if (*nptr == 'E')
    {
        ++nptr;
        if (*nptr == '+')
            ++nptr;
        else if (*nptr == '-')
        {
            ++nptr;
            bcd.exp_sign = 0x0F;
        }
        //unsigned long exp = strtoul(nptr, endptr, 10);  // calling this function may bloat this object file
        const char *e = nptr;
        for ( ; isdigit(*e); ++e)
            ;
        size_t numExpDigits = e - nptr;
        if (numExpDigits > MAX_EXP_LENGTH)
            return getHugeVal(bcd.fraction_sign != 0);
        for (unsigned char j = MAX_EXP_LENGTH - (unsigned char) numExpDigits; j != MAX_EXP_LENGTH; ++j, ++nptr)
            bcd.exp[j] = (unsigned char) (*nptr - '0');
    }
    if (endptr)
        *endptr = (char *) nptr;

    #ifdef DUMP_BCD
    printf("# strtof:\n");
    dumpBCD(&bcd);
    #endif

    //unsigned *fpcb = (unsigned *) get_FPCB_SINGLE();
    //printf("# strtof: FPCB_SINGLE=$%04x%04x%04x\n", fpcb[0], fpcb[1], fpcb[2]);

	// Call the MC6839's BINDEC operation.
    float result;
    _CMOC_callMC6839DecBin(&result, &bcd);
    // Note that if the conversion was inexact, the 0x40 bit of the status byte
    // of the FPCB will be set.
    //printf("# strtof: result=$%08lx\n", * (unsigned long *) &result);
    //printf("# strtof: FPCB_SINGLE=$%04x%04x%04x\n", fpcb[0], fpcb[1], fpcb[2]);
    return result;
}
