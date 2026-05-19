#include <coco.h>

#define PROGRAM "check-long.c"

typedef unsigned long ulong;


word numAsserts = 0, numErrors = 0;

#define assert(cond) do { ++numAsserts; \
                          /*printf("TEST AT LINE %u\n", __LINE__);*/ \
                          if (!(cond)) { \
                            printf(PROGRAM ": ERROR: ASSERT FAILED: LINE %d\n", __LINE__); \
                            ++numErrors; \
                            exit(1); \
                            } \
                        } while (0)

void dumpMem(void *p, byte n)
{
    printf("@%p: ", p);
    for (byte i = 0; i < n; ++i)
        printf("%02X ", ((byte *) p)[i]);
    putchar('\n');
}


void dumpULong(unsigned long *p)
{
    dumpMem(p, sizeof(long));
}


#ifdef PART0


const unsigned long constULong = 0xABCDEF98UL;


void declarations()
{
    char temp[128];

    unsigned long ul0 = 0,
                  ul1 = (char) 250,
                  ul2 = 2017,
                  ul3 = (short) 60000,
                  ul4 = 0xDEADBEEF,
                  ul5 = (char) -1,
                  ul6 = (short) -7777,
                  ul7 = 60000,
                  ul8 = -1000UL,
                  ul9 = (byte) 200;
    sprintf(temp, "%lu", ul0);
    assert(!strcmp(temp, "0"));
    sprintf(temp, "%lu", ul1);
    assert(!strcmp(temp, "4294967290"));
    sprintf(temp, "%lu", ul2);
    assert(!strcmp(temp, "2017"));
    sprintf(temp, "%lu", ul3);
    assert(!strcmp(temp, "4294961760"));
    sprintf(temp, "%lu", ul4);
    assert(!strcmp(temp, "3735928559"));
    sprintf(temp, "%lu", ul5);
    assert(!strcmp(temp, "4294967295"));
    sprintf(temp, "%lu", ul6);
    assert(!strcmp(temp, "4294959519"));
    sprintf(temp, "%lu", ul7);
    assert(!strcmp(temp, "60000"));
    sprintf(temp, "%lu", ul8);
    assert(!strcmp(temp, "4294966296"));
    sprintf(temp, "%lu", ul9);
    assert(!strcmp(temp, "200"));


    long sl0 = 0,
         sl1 = (char) 250,
         sl2 = 2017,
         sl3 = (short) 60000,
         sl4 = 0xDEADBEEF,
         sl5 = (char) -1,
         sl6 = (short) -7777,
         sl7 = 60000,
         sl8 = -1000UL,
         sl9 = (byte) 200;
    sprintf(temp, "%ld", sl0);
    assert(!strcmp(temp, "0"));
    sprintf(temp, "%ld", sl1);
    assert(!strcmp(temp, "-6"));
    sprintf(temp, "%ld", sl2);
    assert(!strcmp(temp, "2017"));
    sprintf(temp, "%ld", sl3);
    assert(!strcmp(temp, "-5536"));
    sprintf(temp, "%ld", sl4);
    assert(!strcmp(temp, "-559038737"));
    sprintf(temp, "%ld", sl5);
    assert(!strcmp(temp, "-1"));
    sprintf(temp, "%ld", sl6);
    assert(!strcmp(temp, "-7777"));
    sprintf(temp, "%ld", sl7);
    assert(!strcmp(temp, "60000"));
    sprintf(temp, "%ld", sl8);
    assert(!strcmp(temp, "-1000"));
    sprintf(temp, "%ld", sl9);
    assert(!strcmp(temp, "200"));

    signed long sl10 = -2;
    sprintf(temp, "%ld", sl10);
    assert(!strcmp(temp, "-2"));

    //printf("temp=[%s]\n", temp);

    assert(constULong == 0xABCDEF98UL);
    const unsigned long localConstULong = 0x12ABCDEFLU;
    assert(localConstULong == 0x12ABCDEFUL);
}


void unaryOperators()
{
    ulong ul0 = 1000000, ul2 = 98765;
    assert(ul0 == 1000000);
    assert(ul2 == 98765);
    ul2 = +ul0;
    assert(ul2 == 1000000);
    ul2 = -ul0;
    assert(ul2 == 4293967296);

    ulong *pul0 = &ul0;
    *pul0 = 76543;
    assert(*pul0 == 76543);
    assert(ul0 == 76543);

    assert(ul0++ == 76543);
    assert(ul0 == 76544);
    ++ul0;
    assert(ul0 == 76545);
    --ul0;
    assert(ul0 == 76544);
    assert(ul0-- == 76544);
    assert(ul0 == 76543);

    assert(!(!ul0));
    ul0 = 0L;
    assert(!ul0);
    unsigned char ub = !ul0;

    long sl0 = 1000000, sl2 = 98765;
    assert(sl0 == 1000000);
    assert(sl2 == 98765);
    sl2 = +sl0;
    assert(sl2 == 1000000);
    sl2 = -sl0;
    assert(sl2 == -1000000);

    long *psl0 = &sl0;
    *psl0 = 76543;
    assert(*psl0 == 76543);
    assert(sl0 == 76543);

    assert(sl0++ == 76543);
    assert(sl0 == 76544);
    ++sl0;
    assert(sl0 == 76545);
    --sl0;
    assert(sl0 == 76544);
    assert(sl0-- == 76544);
    assert(sl0 == 76543);

    assert(!(!sl0));
    sl0 = 0L;
    assert(!sl0);
    unsigned char sb = !sl0;


    assert(sizeof(long) == 4);
    assert(sizeof(signed long) == 4);
    assert(sizeof(unsigned long) == 4);
    assert(sizeof(0L) == 4);
    assert(sizeof(0l) == 4);
    assert(sizeof(0UL) == 4);
    assert(sizeof(0uL) == 4);
    assert(sizeof(0Ul) == 4);
    assert(sizeof(0ul) == 4);
    assert(sizeof(0U) == 2);
    assert(sizeof(0u) == 2);
    assert(sizeof(1000) == 2);
    assert(sizeof(1000000) == 4);
    assert(sizeof(-1000) == 2);
    assert(sizeof(-1000L) == 4);
    assert(sizeof(-1000000) == 4);
    assert(sizeof(ul0) == 4);
}


void unsignedLongBinaryOperators()
{
    ulong ul0 = 1234567UL,
          ul1 = 445566UL,
          ul2 = 0x77777777UL,
          ul3 = 257UL;
    word u0 = 1844,
         u1 = 60000;
    int s0 = 55,
        s1 = -9999;

    byte ok = 0;
    if (ul0 > 0)
	ok = 1;
    assert(ok);
    ok = 0;
    if (ul0 == 1234567)
	ok = 1;
    assert(ok);
    ok = 0;
    if (0 < ul0)
	ok = 1;
    assert(ok);

    assert(ul0 == ul0);
    assert(ul0 <= ul0);
    assert(ul0 >= ul0);
    assert(ul0 != ul1);
    assert(ul0 >  ul1);
    assert(ul0 >= ul1);
    assert(ul1 != ul0);
    assert(ul1 <  ul0);
    assert(ul1 <= ul0);

    assert(55L == s0);
    assert(55L <= s0);
    assert(55L >= s0);
    assert(s0 == 55L);
    assert(s0 <= 55L);
    assert(s0 >= 55L);

    assert(1844L == u0);
    assert(1844L <= u0);
    assert(1844L >= u0);
    assert(u0 == 1844L);
    assert(u0 <= 1844L);
    assert(u0 >= 1844L);

    ulong lZero = 0;
    assert(!lZero);

    assert(ul0 + ul1 == 1680133L);
    assert(ul0 + u0 == 1236411L);
    assert(ul0 + u1 == 1294567L);
    assert(u1 + ul0 == 1294567L);
    assert(ul0 + s0 == 1234622);
    assert(ul0 + s1 == 1224568L);
    assert(s1 + ul0 == 1224568L);

    assert(ul0 - ul1 == 789001UL);
    assert(ul0 - u0 == 1232723UL);
    assert(ul0 - u1 == 1174567UL);
    assert(u1 - ul0 == 4293792729UL);
    assert(ul0 - s0 == 1234512UL);
    assert(ul0 - s1 == 1244566UL);
    assert(s1 - ul0 == 4293722730UL);

    assert(0xFFFFFFFFUL * 0xFFFFFFFFUL == 1UL);
    assert(0x000FF000UL * 0x000FF000UL == 0x1000000UL);
    assert(100UL * 1000UL == 100000UL);
    assert(0xFF000000UL * 0xFF000000UL == 0UL);
    assert(ul0 * ul1 == 325266034UL);
    assert(ul0 * u0 == 2276541548UL);
    assert(ul0 * u1 == 1059575968UL);
    assert(u1 * ul0 == 1059575968UL);
    assert(ul0 * s0 == 67901185UL);
    assert(ul0 * s1 == 540466455UL);
    assert(s1 * ul0 == 540466455UL);

    assert(4000000000UL / 70000UL == 57142UL);
    assert(4000000000UL / 7000UL == 571428UL);
    assert(0UL / 7000UL == 0UL);
    assert(70000UL / 0UL == 0xFFFFFFFFUL);  // division by zero does not hang
    assert(7000UL / 0UL == 0xFFFFFFFFUL);  // division by zero does not hang
    assert(ul0 / u0 == 669UL);
    assert(u1 / ul3 == 233UL);
    assert(ul0 / s0 == 22446UL);
    assert(ul0 / s1 == 0UL);   // -9999 gets sign-extended to 0xffffd8f1
    assert(ul0 / -s0 == 0UL);  // -55 gets sign-extended to 0xffffffc9
    assert(0xFFFFFFFFUL / (signed short) -1 == 1UL);
    assert(0xFFFFFFFFUL / (signed short) 1 == -1UL);
    assert(0xFFFFFFFFUL / (signed short) 1 == -1UL);
    assert(0xFFFFFFFFUL / (signed short) 0x8000 == 1UL);
    assert((signed short) -9999 / 9UL == 477217477UL);

    // Divisions by zero:
    unsigned uz = 0;
    signed sz = 0;
    assert(ul0 / 0UL == 0xFFFFFFFFUL);
    assert(ul0 / uz == 0xFFFFFFFFUL);
    assert(ul0 / sz == 0xFFFFFFFFUL);

    assert(123UL % 10UL == 3UL);
    assert(-123UL % 10UL == 3UL);
    assert(4000000000UL % 70000UL == 60000UL);
    assert(4000000000UL % 7000UL == 4000UL);
    assert(0UL % 7000UL == 0UL);
    assert(0UL % 70000UL == 0UL);
    assert(70000UL % 0UL == 70000UL);  // division by zero does not hang
    assert(7000UL % 0UL == 7000UL);  // division by zero does not hang

    assert(ul0 % u0 == 931UL);
    assert(u1 % ul3 == 119UL);
    assert(ul0 % s0 == 37UL);
    //printf("%lu / %d = %lu\n", ul0, -s0, ul0 % -s0);
    assert(ul0 % -s0 == 1234567UL);  // -55 gets sign-extended to 0xffffffc9
    assert(0xFFFFFFFFUL % (signed short) -1 == 0UL);
    assert(0xFFFFFFFFUL % (signed short) 1 == 0UL);
    assert((signed short) -9999 % 9UL == 4UL);

    // Divisions by zero:
    assert(ul0 % 0UL == ul0);
    assert(ul0 % uz == ul0);
    assert(ul0 % sz == ul0);
}


void conditions()
{
    long l0 = -12345678L;
    long l1;
    if (l0)
	l1 = 1L;
    else
	l1 = 0L;
    assert(l1 == 1L);

    l0 = -12345678L;
    if (!l0)
	l1 = 1L;
    else
	l1 = 0L;
    assert(l1 == 0L);

    if (l0 == 0)
        l1 = 2;
    else
        l1 = 3;
    assert(l1 == 3);

    if (0 == l0)
        l1 = 4;
    else
        l1 = 5;
    assert(l1 == 5);

    if (l0 == 0UL)
        l1 = 2;
    else
        l1 = 3;
    assert(l1 == 3);

    if (0UL == l0)
        l1 = 4;
    else
        l1 = 5;
    assert(l1 == 5);

    while (!l1)
	assert(0);
    while (l1 == 0)
	assert(0);
    while (l1 == 0UL)
	assert(0);

    l1 = 0;
    while (l1)
	assert(0);
    for (; l1; )
	assert(0);

    l0 = 0;
    l1 = 1;
    if (l0 || l1)
        l0 = 2;
    assert(l0 == 2);
    if (l0 && l1)
        l0 = 3;
    assert(l0 == 3);
    assert(l0 != 2);
}


void shifts()
{
    // Left shifts.
    unsigned long ul0 = 1;
    assert((ul0 << 1UL) == 2);
    assert(ul0 == 1);
    unsigned long ul1 = ul0 << 1UL;
    assert(ul1 == 2);
    ul0 <<= 4L;
    assert(ul0 == 16);
    assert(ul1 == 2);
    ul0 <<= 24;
    assert(ul0 == 0x10000000UL);
    ul0 <<= 32;
    assert(!ul0);
    ul0 = 1;
    assert(ul0 <<  8 == 0x100);
    assert(ul0 << 16 == 0x10000);
    assert(ul0 << 24 == 0x1000000);
    assert(ul0 << 31 == 0x80000000);

    long sl0 = 1;
    assert((sl0 << 1UL) == 2);
    assert(sl0 == 1);
    long sl1 = sl0 << 1UL;
    assert(sl1 == 2);
    sl0 <<= 4L;
    assert(sl0 == 16);
    assert(sl1 == 2);
    sl0 <<= 24;
    assert(sl0 == 0x10000000UL);
    sl0 <<= 32;
    assert(!sl0);
    sl0 = 1;
    assert(sl0 <<  8 == 0x100);
    assert(sl0 << 16 == 0x10000);
    assert(sl0 << 24 == 0x1000000);
    assert(sl0 << 31 == 0x80000000);

    // Right shifts.
    ul0 = 0x80000000UL;
    assert((ul0 >> 1UL) == 0x40000000UL);
    assert(ul0 == 0x80000000UL);
    ul1 = ul0 >> 1UL;
    assert(ul1 == 0x40000000UL);
    ul0 >>= 4L;
    assert(ul0 == 0x8000000UL);
    assert(ul1 == 0x40000000UL);
    ul0 >>= 24;
    assert(ul0 == 0x8UL);
    ul0 >>= 32;
    assert(!ul0);
    ul0 = 0x80000000UL;
    assert(ul0 >>  8 == 0x800000UL);
    assert(ul0 >> 16 == 0x8000UL);
    assert(ul0 >> 24 == 0x80UL);
    assert(ul0 >> 32 == 0UL);
    assert(ul0 >> 31 == 1UL);

    sl0 = 0x80000000L;
    assert(sl0 < 0);
    assert(sl0 >> 1UL == 0xC0000000L);  // sign extension
    assert(sl0 == 0x80000000L);
    sl1 = sl0 >> 1UL;
    assert(sl1 == 0xC0000000L);
    sl0 >>= 4L;
    assert(sl0 == 0xF8000000L);
    assert(sl1 == 0xC0000000L);
    sl0 >>= 24;
    assert(sl0 == 0xFFFFFFF8L);
    sl0 >>= 32;
    assert(sl0 == -1);
    sl0 = 0x80000000L;
    assert(sl0 >>  8 == 0xFF800000L);
    assert(sl0 >> 16 == 0xFFFF8000L);
    assert(sl0 >> 24 == 0xFFFFFF80L);
    assert(sl0 >> 31 == 0xFFFFFFFFL);

    sl0 = 0x92345678L;
    assert(sl0 >> 1 == 0xC91a2b3cL);
    sl0 = 0x12345678L;
    assert(sl0 >> 1 == 0x091a2b3cL);

    // Shift a short int by a number of bits given by a long.
    unsigned short u0 = 4200;
    assert(u0 << 1UL == 8400);
    assert(u0 >> 1UL == 2100);
}


struct S0
{
    long l;
    long a[2];
};

void structs()
{
    struct S0 s0 = { 100000L, { -200000L, 300000L } };
    assert(s0.l == 100000L);
    assert(s0.a[0] == -200000L);
    assert(s0.a[1] == 300000L);
}


#endif


#ifdef PART1


void signedLongBinaryOperators()
{
    long sl0 = -1234567L,
          sl1 = 445566L,
          sl2 = 0x77777777L,
          sl3 = 257L;
    word u0 = 1844,
         u1 = 60000;
    int s0 = 55,
        s1 = -9999;

    byte ok = 0;
    if (sl0 < 0)
	ok = 1;
    assert(ok);
    ok = 0;
    if (sl0 == -1234567)
	ok = 1;
    assert(ok);
    ok = 0;
    if (0 > sl0)
	ok = 1;
    assert(ok);

    assert(sl0 == sl0);
    assert(sl0 <= sl0);
    assert(sl0 >= sl0);
    assert(sl0 != sl1);
    assert(sl0 <  sl1);
    assert(sl0 <= sl1);
    assert(sl1 != sl0);
    assert(sl1 >  sl0);
    assert(sl1 >= sl0);

    assert(55L == s0);
    assert(55L <= s0);
    assert(55L >= s0);
    assert(s0 == 55L);
    assert(s0 <= 55L);
    assert(s0 >= 55L);

    assert(1844L == u0);
    assert(1844L <= u0);
    assert(1844L >= u0);
    assert(u0 == 1844L);
    assert(u0 <= 1844L);
    assert(u0 >= 1844L);

    ulong lZero = 0;
    assert(!lZero);

    assert(sl0 + sl1 == -789001L);
    assert(sl0 + u0 == -1232723L);
    assert(sl0 + u1 == -1174567L);
    assert(u1 + sl0 == -1174567L);
    assert(sl0 + s0 == -1234512L);
    assert(sl0 + s1 == -1244566L);
    assert(s1 + sl0 == -1244566L);

    assert(sl0 - sl1 == -1680133L);
    assert(sl0 - u0 == -1236411L);
    assert(sl0 - u1 == -1294567L);
    assert(u1 - sl0 == 1294567L);
    assert(sl0 - s0 == -1234622L);
    assert(sl0 - s1 == -1224568L);
    assert(s1 - sl0 == 1224568L);

    assert(0xFFFFFFFFL * 0xFFFFFFFFL == 1L);
    assert(0x000FF000L * 0x000FF000L == 0x1000000L);
    assert(-100L * 1000L == -100000L);
    assert(100L * -1000L == -100000L);
    assert(0xFF000000L * 0xFF000000L == 0L);
    assert(sl0 * sl1 == -325266034L);
    assert(sl0 * u0 == 2018425748L);
    assert(sl0 * u1 == -1059575968L);
    assert(u1 * sl0 == -1059575968L);
    assert(sl0 * s0 == -67901185L);
    assert(sl0 * s1 == -540466455L);
    assert(s1 * sl0 == -540466455L);

    assert(-4000000L / -70000L ==  57L);
    assert(-4000000L /  70000L == -57L);
    assert( 4000000L / -70000L == -57L);
    assert( 4000000L /  70000L ==  57L);
    assert(2000000000L / 70000L == 28571L);
    assert(4000000000L / 70000L == 57142L);
    assert(70000L / 4000000000L == 0L);
    assert(4000000000L / 7000L == 571428L);
    assert(0L / 7000L == 0L);
    assert(7000L / 0L == 0xFFFFFFFFL);  // division by zero does not hang
    assert(-1L == 0xFFFFFFFFL);
    assert(sl0 / u0 == -669L);
    assert(sl0 / 60000 == -20L);
    assert(u1 / sl3 == 233L);
    assert(sl0 / s0 == -22446L);
    assert(s0 / sl0 == 0L);
    assert((signed short) 60000 / 1000L == -5L);  // -5536 / 1000 = -5
    assert((signed short) -60000 / 1000L == 5L);  // +5536 / 1000 = -5
    assert((unsigned short) 60000 / 1000L == 60L);
    assert(0xFFFFFFFFUL / (signed short) -1 == 1UL);
    assert((signed short) -9999 / 9UL == 477217477UL);

    // Divisions by zero:
    unsigned uz = 0;
    signed sz = 0;
    assert(sl0 / 0UL == 0xFFFFFFFFUL);
    assert(sl0 / uz == 1L);  // division returns 0xFFFFFFFF, negated
    assert(sl0 / sz == 1l);  // ditto

    //printf("%ld / %ld = %ld\n", -4000000L, -70000L, -4000000L % -70000L);

    // Sign of module is sign of left side.
    assert(-4000000L % -70000L == -10000L);
    assert(-4000000L %  70000L == -10000L);
    assert( 4000000L % -70000L ==  10000L);
    assert( 4000000L %  70000L ==  10000L);
    assert(2000000000L % 70000L == 30000L);
    assert(4000000000L % 70000L == 60000L);
    assert(70000L % 4000000000L == 70000L);
    assert(4000000000L % 7000L == 4000L);
    assert(0L % 7000L == 0L);
    assert(7000L % 0L == 7000L);  // division by zero does not hang
    assert(-1L == 0xFFFFFFFFL);
    assert( sl0 %  u0 == -931L);
    assert( sl0 % -u0 == -931L);
    assert(-sl0 %  u0 ==  931L);
    assert(-sl0 % -u0 ==  931L);
    assert( sl0 %  60000 == -34567L);
    assert( sl0 % -60000 == -39L);
    assert(-sl0 %  60000 ==  34567L);
    assert(-sl0 % -60000 ==  39L);
    assert(u1 % sl3 == 119L);
    assert( sl0 % s0  == -37L);
    assert( sl0 % -s0 == -37L);
    assert(-sl0 % s0  ==  37L);
    assert(-sl0 % -s0 ==  37L);
    assert(s0 % sl0 == 55L);
    assert((signed short) 60000 % 1000L == -536L);  // -5536 % 1000 = -536
    assert((signed short) -60000 % 1000L == 536L);  // +5536 / 1000 =  536
    assert((unsigned short) 60003 % 1000L == 3L);
    assert(0xFFFFFFFFL % (signed short) -1 == 0L);
    assert((signed short) -9999 % 9UL == 4L);

    // Divisions by zero:
    assert(sl0 % 0L == sl0);
    assert(sl0 % uz == sl0);
    assert(sl0 % sz == sl0);
}


void assignments()
{
    // Signed word.
    int i0; i0 = 184444UL;
    assert(i0 == -12164);
    int i1; i1 = -1234567L;
    assert(i1 == 10617);

    // Signed byte.
    char c0; c0 = 77777UL;
    assert(c0 == -47);
    char c1; c1 = -55555L;
    assert(c1 == -3);

    // Unsigned word.
    unsigned u0; u0 = 184444UL;
    assert(u0 == 53372);
    unsigned u1; u1 = -1234567L;
    assert(u1 == 10617);

    // Unsigned byte.
    unsigned char b0; b0 = 77777UL;
    assert(b0 == 209);
    unsigned char b1; b1 = -55555L;
    assert(b1 == 253);

    // Assignment to a long from an integral expression.
    ulong ul0; ul0 = (char) -42;
    assert(ul0 == 4294967254UL);
    ulong ul1; ul1 = (unsigned char) 244;
    assert(ul1 == 244L);
    ulong ul2; ul2 = (int) -4242;
    assert(ul2 == 4294963054UL);
    ulong ul3; ul3 = (unsigned) 60000;
    assert(ul3 == 60000UL);

    long sl0; sl0 = (char) -42;
    assert(sl0 == -42L);
    long sl1; sl1 = (unsigned char) 244;
    assert(sl1 == 244L);
    long sl2; sl2 = (int) -4242;
    assert(sl2 == -4242L);
    long sl3; sl3 = (unsigned) 60000;
    assert(sl3 == 60000L);
}


void assignmentsWithOperations()
{
    unsigned long ul0 = 515151UL;
    unsigned long ul1 = 184444UL;
    ul0 += ul1;
    assert(ul0 == 699595UL);
    ul0 -= ul1;
    assert(ul0 == 515151UL);
    ul0 *= ul1;
    assert(ul0 == 527230532UL);
    ul0 /= ul1;
    assert(ul0 == 2858UL);

    ul0 = 0x50002UL;
    assert(ul0 == 327682UL);
    ul0 %= 100000UL;
    assert(ul0 == 27682UL);
    ul0 = 0x50002UL;

    // Integrals on right side.
    ul0 += (char) -5;
    assert(ul0 == 0x4FFFDUL);
    ul0 += (unsigned char) 200;
    assert(ul0 == 0x500C5UL);
    ul0 += (int) -4444;
    assert(ul0 == 0x4EF69UL);
    ul0 += (unsigned) 40000;
    assert(ul0 == 0x58BA9UL);
    ul0 /= 1000;
    assert(ul0 == 363UL);

    // Integrals on left side.
    char c0 = -33;
    unsigned char b0 = 150;
    int i0 = -9898;
    unsigned u0 = 42000;

    c0 += 88888UL;
    assert(c0 == 23);
    b0 += 77777UL;
    assert(b0 == 103);
    i0 += 88888UL;
    assert(i0 == 13454);
    u0 += 88888UL;
    assert(u0 == 65352);

    c0 *= 60000UL;
    assert(c0 == -96);
    i0 /= 2UL;
    assert(i0 == 6727);
    u0 %= 60000UL;
    assert(u0 == 5352);


    // Signed long.

    long sl0 = 515151L;
    long sl1 = 184444L;
    sl0 += sl1;
    assert(sl0 == 699595L);
    sl0 -= sl1;
    assert(sl0 == 515151L);
    sl0 *= sl1;
    assert(sl0 == 527230532L);
    sl0 /= sl1;
    assert(sl0 == 2858L);
    sl0 *= -100000L;
    assert(sl0 == -285800000L);
    sl0 /= -1000000L;
    assert(sl0 == 285L);

    sl0 = 0x50002L;
    assert(sl0 == 327682L);
    sl0 %= 100000L;
    assert(sl0 == 27682L);
    sl0 = 0x50002L;

    // Integrals on right side.
    sl0 += (char) -5;
    assert(sl0 == 0x4FFFDL);
    sl0 += (unsigned char) 200;
    assert(sl0 == 0x500C5L);
    sl0 += (int) -4444;
    assert(sl0 == 0x4EF69L);
    sl0 += (unsigned) 40000;
    assert(sl0 == 0x58BA9L);

    // Integrals on left side.
    c0 = -33;
    b0 = 150;
    i0 = -9898;
    u0 = 42000;

    c0 += 88888L;
    assert(c0 == 23);
    b0 += 77777L;
    assert(b0 == 103);
    i0 += 88888L;
    assert(i0 == 13454);
    u0 += 88888L;
    assert(u0 == 65352);

    c0 *= -60000L;
    assert(c0 == 96);
    i0 /= -2L;
    assert(i0 == -6727);
    u0 %= 60000L;
    assert(u0 == 5352);

    i0 = 65535;
    i0 /= -2L;
    assert(i0 == 0);  // (int) 65535 / -2L == (long) (int) -1 / -2L == -1L / -2L == 0

    i0 = 1234;
    i0 %= 100L;
    assert(i0 == 34);

    c0 = 127;
    c0 /= -2L;
    assert(c0 == -63);

    c0 = 255;
    c0 /= -2L;
    assert(c0 == 0);  // (char) 255 / -2L == (long) (char) -1 / -2L == -1L / -2L == 0

    c0 = 123;
    c0 %= 10L;
    assert(c0 == 3);
}


void casts()
{
    char c0;
    unsigned char b0;
    int i0;
    unsigned u0;
    unsigned long ul0;

    c0 = (char) 0x123456EEL;
    assert(c0 == -18);
    b0 = (unsigned char) 0x654321DDL;
    assert(b0 == 0xDD);
    i0 = (int) 0x9876ABCDL;
    assert(i0 == -21555);
    u0 = (unsigned) 0x3142FACDL;
    assert(u0 == 0xFACD);

    ul0 = (unsigned long) c0;
    assert(ul0 == 0xFFFFFFEEL);
    ul0 = (unsigned long) b0;
    assert(ul0 == 0xDD);
    ul0 = (unsigned long) i0;  // negative short int gets sign-extended
    assert(ul0 == 4294945741UL);
    ul0 = (unsigned long) u0;
    assert(ul0 == 0xFACD);
    ul0 = (unsigned long) 0x654321DDUL;
    assert(ul0 == 0x654321DDUL);
}


long returnSignedLongFromSignedChar() { return (char) -42; }
long returnSignedLongFromUnsignedChar() { return (char) 42; }
long returnSignedLongFromSignedShort() { return -4242; }
long returnSignedLongFromUnsignedShort() { return 4242; }
unsigned long returnUnsignedLongFromSignedChar() { return (char) -42; }
unsigned long returnUnsignedLongFromUnsignedChar() { return (char) 42; }
unsigned long returnUnsignedLongFromSignedShort() { return -4242; }
unsigned long returnUnsignedLongFromUnsignedShort() { return 4242; }


void returningLong()
{
    assert(returnSignedLongFromSignedChar() == -42);
    assert(returnSignedLongFromUnsignedChar() == 42);
    assert(returnSignedLongFromSignedShort() == -4242);
    assert(returnSignedLongFromUnsignedShort() == 4242);
    assert(returnUnsignedLongFromSignedChar() == -42);
    assert(returnUnsignedLongFromUnsignedChar() == 42);
    assert(returnUnsignedLongFromSignedShort() == -4242);
    assert(returnUnsignedLongFromUnsignedShort() == 4242);
}


#endif


char takeChar(char x) { return x; }
unsigned char takeUnsignedChar(unsigned char x) { return x; }
int takeInt(int x) { return x; }
unsigned takeUnsigned(unsigned x) { return x; }
long takeLong(long x) { return x; }
unsigned long takeULong(unsigned long x) { return x; }


#ifdef PART2


void argumentPassing()
{
    // Pass long for intetral.
    assert(takeChar(-100000L) == 96);
    assert(takeUnsignedChar(100000L) == 160);
    assert(takeInt(-100000L) == 31072);
    assert(takeUnsigned(100000L) == 34464);

    // Pass integral for long.
    assert(takeLong((char) -42) == -42L);
    assert(takeLong((unsigned char) 250) == 250L);
    assert(takeLong((int) -1000) == -1000L);
    assert(takeLong((unsigned) 55555) == 55555L);
    assert(takeLong(100000L) == 100000L);
    assert(takeULong((char) -42) == 0xffffffd6UL);
    assert(takeULong((unsigned char) 250) == 250L);
    assert(takeULong((int) -1000) == 0xfffffc18UL);
    assert(takeULong((unsigned) 55555) == 55555L);
    assert(takeULong(100000L) == 100000L);

    // Ignoring return value.
    takeLong(1L);
    takeULong(1L);
}


void stringOps()
{
    char *endptr = 0;

    unsigned long ul0 = strtoul("99999%%%", &endptr, 10);
    assert(ul0 == 99999UL);
    assert(endptr == "99999%%%" + 5);

    ul0 = strtoul("-99999%%%", &endptr, 10);
    assert(ul0 == 0xfffe7961UL);
    assert(endptr == "-99999%%%" + 6);

    signed long sl0 = strtol("99999%%%", &endptr, 10);
    assert(sl0 == 99999UL);
    assert(endptr == "99999%%%" + 5);

    sl0 = strtol("-99999%%%", &endptr, 10);
    assert(sl0 == -99999L);
    assert(endptr == "-99999%%%" + 6);

    ul0 = atoul("491416");
    assert(ul0 == 491416UL);
    ul0 = atoul("-491416");
    assert(ul0 == 0xfff88068UL);

    sl0 = atol("491416");
    assert(sl0 == 491416L);
    sl0 = atol("-491416");
    assert(sl0 == -491416L);

    // %ld, %lu, %lx.
    char temp[128];
    sprintf(temp, "%ld %ld %lu %lu", 100000L, -100000L, 100000UL, -100000L);
    assert(!strcmp(temp, "100000 -100000 100000 4294867296"));
    sprintf(temp, "%lx %lx %lx %lx %lx %lx %lx",
                  0L, 42L, 100000L, 196613L, -42L, -1000L, -100000L);
    assert(!strcmp(temp, "0 2A 186A0 30005 FFFFFFD6 FFFFFC18 FFFE7960"));

    // Width specification.
    sprintf(temp, "%5ld %5ld %2ld", 42L, -42L, 99999L);
    assert(!strcmp(temp, "   42   -42 99999"));
    sprintf(temp, "%5lu %5lu %2lu", 42L, -42L, 99999L);
    assert(!strcmp(temp, "   42 4294967254 99999"));
    sprintf(temp, "%5lx %5lx %20lx %9lx %11lx %5lx %5lx",
                  0L, 42L, 100000L, 196613L, -42L, -1000L, -100000L);
    //printf("L%d: [%s]\n", __LINE__, temp);
    assert(!strcmp(temp, "    0    2A                186A0     30005    FFFFFFD6 FFFFFC18 FFFE7960"));

    // Padding with 0.
    sprintf(temp, "%02ld %02ld %05ld %05ld %02ld", 0L, 6L, 42L, -42L, 99999L);
    assert(!strcmp(temp, "00 06 00042 -0042 99999"));
    sprintf(temp, "%02lu %02lu %05lu %05lu %02lu", 0L, 6L, 42L, -42L, 99999L);
    assert(!strcmp(temp, "00 06 00042 4294967254 99999"));
    sprintf(temp, "%02lx %05lx %05lx %020lx %09lx %011lx %05lx %05lx",
                  6L, 0L, 42L, 100000L, 196613L, -42L, -1000L, -100000L);
    //printf("L%d: [%s]\n", __LINE__, temp);
    assert(!strcmp(temp, "06 00000 0002A 000000000000000186A0 000030005 000FFFFFFD6 FFFFFC18 FFFE7960"));

    // %X instead of %x
    sprintf(temp, "%02lX %05lX %05lX %020lX %09lX %011lX %05lX %05lX",
                  7UL, 0L, 43L, 100000L, 196613L, -42L, -1000L, -100000L);
    //printf("L%d: [%s]\n", __LINE__, temp);
    assert(!strcmp(temp, "07 00000 0002B 000000000000000186A0 000030005 000FFFFFFD6 FFFFFC18 FFFE7960"));

    // printf() warnings.
    sprintf(temp, "%d\n", 1L);
    sprintf(temp, "%ld\n", 1);
}


const long ga0[] = { 55555555UL, 66666666L, 77777777L, -22222222L };
const unsigned long gb0[] = { 5551, 6661, 7771, -2221 };
unsigned long gc0[] = { '%', '\xFF' };
int ge0[] = { 0x123456UL, 0xeeeeeeeeUL };
char gf0[] = { 0x123456UL, 0xeeeeeeeeUL };


void arrays()
{
    assert(ga0[0] == 55555555L);
    assert(ga0[1] == 66666666L);
    assert(ga0[2] == 77777777L);
    assert(ga0[3] == -22222222L);
    assert(ga0[2] * ga0[3] == -1403002286L);

    unsigned long a0[] = { 55555555L, 66666666L, 77777777L, -22222222L };
    assert(sizeof(a0) == 4 * 4);
    assert(sizeof(a0[0]) == 4);
    assert(&a0[4] - &a0[0] == 4);
    assert((char *) &a0[4] - (char *) &a0[0] == 4 * 4);
    assert(a0[0] == 55555555L);
    assert(a0[1] == 66666666L);
    assert(a0[2] == 77777777L);
    assert(a0[3] == -22222222L);
    assert(a0[1] * a0[2] == 4209006858UL);
    assert(a0[2] * a0[3] == 2891965010UL);

    unsigned long b0[] = { 5555, 6666, 7777, -2222 };
    assert(b0[0] == 5555);
    assert(b0[1] == 6666);
    assert(b0[2] == 7777);
    assert(b0[3] == -2222);

    assert(gb0[0] == 5551);
    assert(gb0[1] == 6661);
    assert(gb0[2] == 7771);
    assert(gb0[3] == -2221);

    unsigned long c0[] = { '$' };
    assert(c0[0] == '$');
    assert(gc0[0] == 0x25L);
    assert(gc0[1] == -1L);

    assert(ga0[2UL] == 77777777L);
    long *gp0 = ga0 + 2UL;
    assert(*gp0 == 77777777L);
    assert(gp0[-1L] == 66666666L);

    assert(ge0[0] == 0x3456);
    assert(ge0[1] == 0xEEEE);
    assert(gf0[0] == 0x56);
    assert(gf0[1] == (char) 0xEE);
    unsigned long zero = 0UL, one = 1UL;
    assert(gf0[zero] == 0x56);
    assert(gf0[one] == (char) 0xEE);
    int e0[] = { 0x123457UL, 0xeeeeeee1UL };
    char f0[] = { 0x123458UL, 0xeeeeeee2UL };
    assert(e0[0] == 0x3457);
    assert(e0[1] == 0xEEE1);
    assert(f0[0] == 0x58);
    assert(f0[1] == (char) 0xE2);
}


void initializationExpressions()
{
    // Signed word.
    int i0 = 1844L;
    assert(i0 == 1844);
    int i1 = 16842751L;
    assert(i1 == -1);
    int i2 = -77777L;
    assert(i2 == -12241);
    int i5 = -3L;
    assert(i5 == -3);

    // Signed byte.
    signed char c0 = 77L;
    assert(c0 == 77);
    signed char c1 = 16777471L;
    assert(c1 == -1);
    signed char c2 = -4L;
    assert(c2 == -4);
    signed char c6 = -99999L;
    assert(c6 == 97);

    // Unsigned word.
    unsigned u0 = 1844L;
    assert(u0 == 1844);
    unsigned u1 = 16842751L;
    assert(u1 == 65535);
    unsigned u6 = -99999L;
    assert(u6 == 31073);

    // Unsigned byte.
    unsigned char b0 = 88L;
    assert(b0 == 88);
    unsigned char b1 = 1000000L;
    assert(b1 == 64);
    unsigned char b2 = -4L;
    assert(b2 == 252);
}


void bitwiseOperators()
{
    unsigned long ul0 = 0;

    assert((ul0 | 4UL) == 4);
    assert(ul0 == 0);
    assert((0x400A00L | ul0) == 0x400A00L);
    assert(ul0 == 0);
    assert((ul0 | 1) == 1);
    assert(ul0 == 0);
    assert((0xf000 | ul0) == 0xf000);
    assert(ul0 == 0);
    ul0 |= 0x00200B00UL;
    assert(ul0 == 0x00200B00UL);
    ul0 |= 16;
    assert(ul0 == 0x00200B10UL);

    ul0 = 0xFFFFFFFFUL;
    assert((ul0 & 4UL) == 4);
    assert(ul0 == 0xFFFFFFFF);
    assert((0x400A00L & ul0) == 0x400A00L);
    assert(ul0 == 0xFFFFFFFF);
    assert((ul0 & 1) == 1);
    assert(ul0 == 0xFFFFFFFF);
    assert((0xf000 & ul0) == 0xf000);
    assert(ul0 == 0xFFFFFFFF);
    ul0 &= 0x00200B00UL;
    assert(ul0 == 0x00200B00UL);
    ul0 &= 0xB00;
    assert(ul0 == 0xB00UL);

    ul0 = 0;
    assert((ul0 ^ 4UL) == 4);
    assert(ul0 == 0);
    assert((0x400A00L ^ ul0) == 0x400A00L);
    assert(ul0 == 0);
    assert((ul0 ^ 1) == 1);
    assert(ul0 == 0);
    assert((0xf000 ^ ul0) == 0xf000);
    assert(ul0 == 0);
    ul0 ^= 0x00200B00UL;
    assert(ul0 == 0x00200B00UL);
    ul0 ^= 16;
    assert(ul0 == 0x00200B10UL);

    ul0 = 0xFFFFFFFFUL;
    assert((ul0 ^ 0x000A5000UL) == 0xFFF5AFFFUL);
    assert(ul0 == 0xFFFFFFFFUL);
    ul0 ^= 0xF0000UL;
    assert(ul0 == 0xFFF0FFFFUL);

    long sl0 = 0;
    assert((sl0 | 4UL) == 4);
    assert((sl0 ^ 4UL) == 4);
    sl0 = 0xFFFFFFFFUL;
    assert((sl0 & 4UL) == 4);
}


#endif


int main()
{
    #ifdef _COCO_BASIC__
    cls(255);
    #endif
    //printf("%s\n", __TIME__);

    #ifdef PART0

    declarations();

    unaryOperators();

    unsignedLongBinaryOperators();

    conditions();

    shifts();

    structs();

    #endif

    #ifdef PART1

    signedLongBinaryOperators();

    assignments();

    assignmentsWithOperations();

    casts();

    returningLong();

    #endif

    #ifdef PART2

    argumentPassing();

    stringOps();

    arrays();

    initializationExpressions();

    bitwiseOperators();

    #endif

    if (numErrors == 0)
        printf(PROGRAM ": SUCCESS (%u ASSERTS PASSED).\n", numAsserts);
    else
        printf(PROGRAM ": FAILURE: %u ERROR(S) OUT OF %u ASSERTS.\n", numErrors, numAsserts);
    return 0;
}
