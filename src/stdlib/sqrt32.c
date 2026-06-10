// Source: sqrt16.c
//
unsigned short
sqrt32(unsigned long value)
{
    unsigned char i;
    unsigned long rem, root;

    rem  = 0;
    root = 0;

    // loop over the 16 bits in the root
    for ( i = 0; i < 16; i++ ) {
        // shift the root up by one bit
        root <<= 1;

        // move next two bits from the input into the remainder
        rem = ((rem << 2) + (value >> 30));
        value <<= 2;

        // test root is (2n + 1)
        ++root;

        if ( root <= rem ) {
            // root not more than the remainder, so the new bit is one
            rem -= root;
            ++root;
        } else {
            // root is greater than the remainder, so the new bit is zero
            --root;
        }
    }
    return (unsigned short) (root >> 1);
}
