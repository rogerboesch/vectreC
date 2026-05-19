#!/usr/bin/perl -w

use strict;
use Getopt::Long;
use POSIX;


my $stopOnFail = 0;
my $generateCoCoBinary = 0;
my $testCCompilability = 0;
my $hexLoadOffset = 0;  # passed to usim
my $optimLevel = 2;


my @testCaseList =
(

{
title => q{Smallest legal program},
program => 'int main() { return 0; }',
expected => ""  # output expected from running the executable in usim
},


{
title => q{Calling a built-in function},
program => 'int main() { printf(""); return 0; }',
expected => ""
},


{
title => q{printf()},
program => q`
    int main()
    {
        printf("Hello, world.\n");
        printf("A%sB\n", "\n");
        putstr("C\nD\n", 4);
        putchar('\n');
        return 0;
    }
    `,
expected => "Hello, world.\nA\nB\nC\nD\n\n"
},


{
title => q{Conversions between byte and word},
program => '
    byte f(byte x);
    int main()
    {
        printf("%u\n", (byte) f((byte) 1000));
        assert_eq((byte) f((byte) 1000), 208);
        byte boolean = ((byte) f((byte) 1000)) == 208;
        assert(boolean);
        word oneByteUnsignedWord = 208;
        assert_eq(oneByteUnsignedWord, 208);
        byte eightBitUnsignedByte = 208;
        assert_eq(oneByteUnsignedWord, eightBitUnsignedByte);
        char eightBitSignedByte = 208;  // seen as -48
        assert_ne(oneByteUnsignedWord, eightBitSignedByte);  // as words, 208  != -48

        word w = 0x1200 + 42;
        byte b = (byte) w;
        printf("%u\n", b);
        assert_eq(b, 42);
        word k = 256 - b;
        printf("%u\n", k);
        assert_eq(k, 214);
        k = 50 - b;
        printf("%u\n", k);
        assert_eq(k, 8);
        return 0;
    }
    byte f(byte x) { return (byte) (x + 1000); }
    ',
expected => (((1000 % 256) + 1000) % 256) . "\n42\n214\n8\n"
},

{
title => q{Shifts},
program => '
    void checkLeftConst();
    void checkLeftVar();
    void checkRightConst();
    void checkRightVar();
    word checkCounter = 0;
    #define check(actual, expected) _check(__LINE__, actual, expected)
    void _check(int lineno, word actual, word expected);
    #define scheck(actual, expected) _scheck(__LINE__, actual, expected) 
    void _scheck(int lineno, int actual, int expected);
    void checkLeftVarW(word initVal, byte numBits, word expected);
    void checkLeftVarB(byte initVal, byte numBits, byte expected);
    void checkRightVarW(word initVal, byte numBits, word expected);
    void checkRightVarB(byte initVal, byte numBits, byte expected);
    void checkRightVarSW(sword initVal, byte numBits, sword expected);
    void checkRightVarSB(sbyte initVal, byte numBits, sbyte expected);
    
    word initStackPtr, finalStackPtr;

    int main()
    {
        asm { sts initStackPtr };
        assert_eq(((int)  0xFFFF) >> 16, -1);
        assert_eq(((int)  0xFFFF) >> 15, -1);
        assert_eq(((char) 0xFF)   >> 8,  -1);
        assert_eq(((char) 0xFF)   >> 7,  -1);
        checkLeftConst();
        checkLeftVar();
        checkRightConst();
        checkRightVar();
        byte shift = 16;
        assert_eq(0xFFFF >> shift, 0);
        assert_eq(((int) 0xFFFF) >> shift, -1);
        asm { sts finalStackPtr }
        assert_eq(finalStackPtr, initStackPtr);
        return 0;
    }

    void checkLeftConst()
    {
        word w = 1;
        check(w << 0, 1);
        check(w << 1, 2);
        check(w << 4, 16);
        check(w << 7, 128);
        check(w << 8, 256);
        check(w << 9, 512);
        check(w << 15, 32768);
        check(w << 16, 0);

        w = 0xffff;
        check(w << 0, 0xffff);
        check(w << 1, 0xfffe);
        check(w << 4, 0xfff0);
        check(w << 7, 0xff80);
        check(w << 8, 0xff00);
        check(w << 9, 0xfe00);
        check(w << 10, 0xfc00);
        check(w << 15, 32768);
        check(w << 16, 0);

        byte b = 1;
        check(b << 0, 1);
        check(b << 1, 2);
        check(b << 4, 16);
        check(b << 7, 128);
        check(b << 8, 0);
        check(b << 15, 0);
        check(b << 16, 0);

        b = 0xff;
        check(b << 0, 0xff);
        check(b << 1, 0xfe);
        check(b << 4, 0xf0);
        check(b << 7, 0x80);
        check(b << 8, 0);
        check(b << 15, 0);
        check(b << 16, 0);
    }

    void checkLeftVar()
    {
        word w = 1;
        checkLeftVarW(w, 0, 1);
        checkLeftVarW(w, 1, 2);
        checkLeftVarW(w, 4, 16);
        checkLeftVarW(w, 8, 256);
        checkLeftVarW(w, 15, 32768);
        checkLeftVarW(w, 16, 0);

        w = 0xffff;
        checkLeftVarW(w, 0, 0xffff);
        checkLeftVarW(w, 1, 0xfffe);
        checkLeftVarW(w, 4, 0xfff0);
        checkLeftVarW(w, 8, 0xff00);
        checkLeftVarW(w, 9, 0xfe00);
        checkLeftVarW(w, 15, 32768);
        checkLeftVarW(w, 16, 0);

        byte b = 1;
        checkLeftVarB(b, 0, 1);
        checkLeftVarB(b, 1, 2);
        checkLeftVarB(b, 4, 16);
        checkLeftVarB(b, 8, 0);
        checkLeftVarB(b, 15, 0);
        checkLeftVarB(b, 16, 0);

        b = 0xff;
        checkLeftVarB(b, 0, 0xff);
        checkLeftVarB(b, 1, 0xfe);
        checkLeftVarB(b, 4, 0xf0);
        checkLeftVarB(b, 8, 0);
        checkLeftVarB(b, 15, 0);
        checkLeftVarB(b, 16, 0);
    }

    void checkRightConst()
    {
        word w = 0x8000;
        check(w >> 0, 0x8000);
        check(w >> 1, 0x4000);
        check(w >> 4, 0x0800);
        check(w >> 7, 0x0100);
        check(w >> 8, 0x0080);
        check(w >> 15, 0x0001);
        check(w >> 16, 0);

        w = 0xffff;
        check(w >> 0, 0xffff);
        check(w >> 1, 0x7fff);
        check(w >> 4, 0x0fff);
        check(w >> 7, 0x01ff);
        check(w >> 8, 0x00ff);
        check(w >> 15, 0x0001);
        check(w >> 16, 0);

        byte b = 0x80;
        check(b >> 0, 0x80);
        check(b >> 1, 0x40);
        check(b >> 4, 0x08);
        check(b >> 7, 0x01);
        check(b >> 8, 0);
        check(b >> 15, 0);
        check(b >> 16, 0);

        b = 0xff;
        check(b >> 0, 0xff);
        check(b >> 1, 0x7f);
        check(b >> 4, 0x0f);
        check(b >> 7, 0x01);
        check(b >> 8, 0);
        check(b >> 15, 0);
        check(b >> 16, 0);

        // Signed cases: sign must be preserved.
        // When shifting all bits, result must be all ones.

        char sb = 0x55;
        scheck(sb >> 0, 0x55);
        scheck(sb >> 1, 0x2A);
        scheck(sb >> 4, 0x05);
        scheck(sb >> 7,  0);
        scheck(sb >> 8,  0);
        scheck(sb >> 15, 0);
        scheck(sb >> 16, 0);

        sb = 0xAA;  // -86 decimal
        scheck(sb >> 0, -86);
        scheck(sb >> 1, -43);
        scheck(sb >> 2, -22);
        scheck(sb >> 3, -11);
        scheck(sb >> 4, -6);
        scheck(sb >> 5, -3);
        scheck(sb >> 6, -2);
        scheck(sb >> 7, -1);
        scheck(sb >> 8, -1);
        scheck(sb >> 15, -1);
        scheck(sb >> 16, -1);
        
        int sw = 0x5555;
        scheck(sw >> 0, (int) 0x5555);
        scheck(sw >> 1, (int) 0x2AAA);
        scheck(sw >> 4, (int) 0x0555);
        scheck(sw >> 7, (int) 0x00AA);
        scheck(sw >> 8, (int) 0x0055);
        scheck(sw >> 14, 1);
        scheck(sw >> 15, 0);
        scheck(sw >> 16, 0);
 
        sw = (int) 0xAAAA;  // -21846 decimal
        scheck(sw >>  0, (int) 0xaaaa);
        scheck(sw >>  1, (int) 0xd555);
        scheck(sw >>  2, (int) 0xeaaa);
        scheck(sw >>  3, (int) 0xf555);
        scheck(sw >>  4, (int) 0xfaaa);
        scheck(sw >>  5, (int) 0xfd55);
        scheck(sw >>  6, (int) 0xfeaa);
        scheck(sw >>  7, (int) 0xff55);
        scheck(sw >>  8, (int) 0xffaa);
        scheck(sw >>  9, (int) 0xffd5);
        scheck(sw >> 10, (int) 0xffea);
        scheck(sw >> 11, (int) 0xfff5);
        scheck(sw >> 12, (int) 0xfffa);
        scheck(sw >> 13, (int) 0xfffd);
        scheck(sw >> 14, (int) 0xfffe);
        scheck(sw >> 15, (int) 0xffff);
        scheck(sw >> 16, (int) 0xffff);
    }

    void checkRightVar()
    {
        word w = 0x8000;
        checkRightVarW(w, 0, 0x8000);
        checkRightVarW(w, 1, 0x4000);
        checkRightVarW(w, 4, 0x0800);
        checkRightVarW(w, 7, 0x0100);
        checkRightVarW(w, 8, 0x0080);
        checkRightVarW(w, 15, 0x0001);
        checkRightVarW(w, 16, 0);

        w = 0xffff;
        checkRightVarW(w, 0, 0xffff);
        checkRightVarW(w, 1, 0x7fff);
        checkRightVarW(w, 4, 0x0fff);
        checkRightVarW(w, 7, 0x01ff);
        checkRightVarW(w, 8, 0x00ff);
        checkRightVarW(w, 15, 0x0001);
        checkRightVarW(w, 16, 0);

        byte b = 0x80;
        checkRightVarB(b, 0, 0x80);
        checkRightVarB(b, 1, 0x40);
        checkRightVarB(b, 4, 0x08);
        checkRightVarB(b, 7, 0x01);
        checkRightVarB(b, 8, 0);
        checkRightVarB(b, 15, 0);
        checkRightVarB(b, 16, 0);

        b = 0xff;
        checkRightVarB(b, 0, 0xff);
        checkRightVarB(b, 1, 0x7f);
        checkRightVarB(b, 4, 0x0f);
        checkRightVarB(b, 7, 0x01);
        checkRightVarB(b, 8, 0);
        checkRightVarB(b, 15, 0);
        checkRightVarB(b, 16, 0);

        // Signed cases: sign must be preserved.
        // When shifting all bits, result must be all ones.

        char sb = 0x55;
        checkRightVarSB(sb, 0, 0x55);
        checkRightVarSB(sb, 1, 0x2A);
        checkRightVarSB(sb, 4, 0x05);
        checkRightVarSB(sb, 7,  0);
        checkRightVarSB(sb, 8,  0);
        checkRightVarSB(sb, 15, 0);
        checkRightVarSB(sb, 16, 0);

        sb = 0xAA;  // -86 decimal
        checkRightVarSB(sb, 0, -86);
        checkRightVarSB(sb, 1, -43);
        checkRightVarSB(sb, 2, -22);
        checkRightVarSB(sb, 3, -11);
        checkRightVarSB(sb, 4, -6);
        checkRightVarSB(sb, 5, -3);
        checkRightVarSB(sb, 6, -2);
        checkRightVarSB(sb, 7, -1);
        checkRightVarSB(sb, 8, -1);
        checkRightVarSB(sb, 15, -1);
        checkRightVarSB(sb, 16, -1);
        
        int sw = 0x5555;
        checkRightVarSW(sw, 0, (int) 0x5555);
        checkRightVarSW(sw, 1, (int) 0x2AAA);
        checkRightVarSW(sw, 4, (int) 0x0555);
        checkRightVarSW(sw, 7, (int) 0x00AA);
        checkRightVarSW(sw, 8, (int) 0x0055);
        checkRightVarSW(sw, 14, 1);
        checkRightVarSW(sw, 15, 0);
        checkRightVarSW(sw, 16, 0);
 
        sw = (int) 0xAAAA;  // -21846 decimal
        checkRightVarSW(sw,  0, (int) 0xaaaa);
        checkRightVarSW(sw,  1, (int) 0xd555);
        checkRightVarSW(sw,  2, (int) 0xeaaa);
        checkRightVarSW(sw,  3, (int) 0xf555);
        checkRightVarSW(sw,  4, (int) 0xfaaa);
        checkRightVarSW(sw,  5, (int) 0xfd55);
        checkRightVarSW(sw,  6, (int) 0xfeaa);
        checkRightVarSW(sw,  7, (int) 0xff55);
        checkRightVarSW(sw,  8, (int) 0xffaa);
        checkRightVarSW(sw,  9, (int) 0xffd5);
        checkRightVarSW(sw, 10, (int) 0xffea);
        checkRightVarSW(sw, 11, (int) 0xfff5);
        checkRightVarSW(sw, 12, (int) 0xfffa);
        checkRightVarSW(sw, 13, (int) 0xfffd);
        checkRightVarSW(sw, 14, (int) 0xfffe);
        checkRightVarSW(sw, 15, (int) 0xffff);
        checkRightVarSW(sw, 16, (int) 0xffff);
    }

    void _check(int lineno, word actual, word expected)
    {
        ++checkCounter;
        if (actual != expected)
        {
            printf("ERROR: check #%u at line %d: got %u ($%x), expected %u ($%x)\n",
                    checkCounter, lineno, actual, actual, expected, expected);
        }
    }

    void _scheck(int lineno, int actual, int expected)
    {
        ++checkCounter;
        if (actual != expected)
        {
            printf("ERROR: check #%u at line %d: got %d, expected %d\n",
                    checkCounter, lineno, actual, expected);
        }
    }

    void checkLeftVarW(word initVal, byte numBits, word expected)
    {
        word actual = initVal << numBits;
        ++checkCounter;
        if (actual != expected)
        {
            printf("ERROR: checkLeftVarW #%u: %u << %u, got %u, expected %u\n", checkCounter, initVal, numBits, actual, expected);
        }
    }

    void checkLeftVarB(byte initVal, byte numBits, byte expected)
    {
        asm { lda #0xFF }  // put garbage in A, as a robustness test
        byte actual = initVal << numBits;
        ++checkCounter;
        if (actual != expected)
        {
            printf("ERROR: checkLeftVarB #%u: %u << %u, got %u, expected %u\n", checkCounter, initVal, numBits, actual, expected);
        }
    }

    void checkRightVarW(word initVal, byte numBits, word expected)
    {
        word actual = initVal >> numBits;
        ++checkCounter;
        if (actual != expected)
        {
            printf("ERROR: checkRightVarW #%u: %u >> %u, got %u, expected %u\n", checkCounter, initVal, numBits, actual, expected);
        }
    }

    void checkRightVarB(byte initVal, byte numBits, byte expected)
    {
        asm { lda #0xFF }  // put garbage in A, as a robustness test
        byte actual = initVal >> numBits;
        ++checkCounter;
        if (actual != expected)
        {
            printf("ERROR: checkRightVarB #%u: %u >> %u, got %u, expected %u\n", checkCounter, initVal, numBits, actual, expected);
        }
    }

    void checkRightVarSW(sword initVal, byte numBits, sword expected)
    {
        sword actual = initVal >> numBits;
        ++checkCounter;
        if (actual != expected)
        {
            printf("ERROR: checkRightVarSW #%u: %d >> %u, got %d, expected %u\n", checkCounter, initVal, numBits, actual, expected);
        }
    }

    void checkRightVarSB(sbyte initVal, byte numBits, sbyte expected)
    {
        asm { lda #0xFF }  // put garbage in A, as a robustness test
        sbyte actual = initVal >> numBits;
        ++checkCounter;
        if (actual != expected)
        {
            printf("ERROR: checkRightVarSB #%u: %d >> %u, got %d, expected %u\n", checkCounter, initVal, numBits, actual, expected);
        }
    }
    ',
expected => ""
},


{
title => q{Short-circuited evaluation},
program => '
    byte false() { return 0; }
    byte true()  { return 1; }
    byte must_not_be_called()
    {
        printf("ERROR: must_not_be_called() gets called\n");
        return 0;
    }
    int main()
    {
        if (false() && must_not_be_called())
            printf("ERROR: if with && succeeded\n");
        if (true() || must_not_be_called())
            ;
        else
            printf("ERROR: if with || failed\n");

        // Same with relation operators, to test optimization in BinaryOpExpr::emitConditionalJump().
        if (false() != 0 && must_not_be_called() != 0)
            printf("ERROR: if with && succeeded\n");
        if (true() != 0 || must_not_be_called() != 0)
            ;
        else
            printf("ERROR: if with || failed\n");

        // While case.
        //
        while (false() && must_not_be_called())
            printf("ERROR: while with && succeeded\n");
        byte b = 0;
        while (true() || must_not_be_called())
        {
            ++b;
            break;
        }
        assert_eq(b, 1);  // check that while body entered once

        // Do while case.
        b = 0;
        do
            ++b;
        while (false() && must_not_be_called());
        assert_eq(b, 1);  // check that do-while body entered once

        b = 0;
        do
            if (++b == 5)
                break;
        while (true() || must_not_be_called());
        assert_eq(b, 5);  // check that do-while body entered the correct number of times


        // While case with rel ops.
        //
        while (false() != 0 && must_not_be_called() != 0)
            printf("ERROR: while with && succeeded\n");
        b = 0;
        while (true() != 0 || must_not_be_called() != 0)
        {
            ++b;
            break;
        }
        assert_eq(b, 1);  // check that while body entered once

        // Do while case with rel ops.
        b = 0;
        do
            ++b;
        while (false() != 0 && must_not_be_called() != 0);
        assert_eq(b, 1);  // check that do-while body entered once

        b = 0;
        do
            if (++b == 5)
                break;
        while (true() != 0 || must_not_be_called() != 0);
        assert_eq(b, 5);  // check that do-while body entered the correct number of times

        return 0;
    }
    ',
expected => ""
},

{
title => q{More short-circuited evaluation},
program => '
    byte f(byte c)
    {
        if (c != 32 && c != 40 && c != 80 && c != 100)
            return 0;
        return 1;
    }
    byte g(byte c)
    {
        if (c == 32 || c == 40 || c == 80 || c == 100)
            return 1;
        return 0;
    }
    int main()
    {
        assert_eq(f(32), 1);
        assert_eq(f(40), 1);
        assert_eq(f(80), 1);
        assert_eq(f(0), 0);
        assert_eq(f(132), 0);
        assert_eq(f((byte) (32 + 256)), 1);
        assert_eq(g(32), 1);
        assert_eq(g(40), 1);
        assert_eq(g(80), 1);
        assert_eq(g(0), 0);
        assert_eq(g(132), 0);
        assert_eq(g((byte) (32 + 256)), 1);
        return 0;
    }
    ',
expected => ""
},

{
title => q{Returning byte from function of type word},
program => '
    word f()
    {
        asm("LDA", "#42");  // not supposed to affect return value
        byte k = 1;
        return k;  // A must be cleared by this statement
    }
    word g()
    {
        asm("LDD", "#$ABCD");  // not supposed to affect return value
        return 0x12;  // A must be cleared by this statement
    }
    byte *getPtr()
    {
        asm("LDD", "#$4567");  // not supposed to affect return value
        return 0;  // A must be cleared by this statement
    }
    int fi()
    {
        char a = 255;
        return a; 
    }
    int main()
    {
        assert_eq(f(), 1);
        assert_eq(g(), 0x12);
        assert_eq(getPtr(), 0);
        assert_eq(fi(), -1);
        assert_eq((int) ((char) 255), -1);
        char a = 255;
        assert_eq((int) ((char) a), -1);
        return 0;
    }
    ',
expected => ""
},

{
title => q{Recursion},
program => '
    word fact(word n)
    {
        if (n <= 1) return 1;
        return n * fact(n - 1);
    }
    int main()
    {
        assert_eq(fact(5), 120);
        return 0;
    }
    ',
expected => ""
},

{
title => q{More shifts},
program => '
    int main()
    {
        byte b0 = 3 << 3;
        byte b1 = (3 << 3) + 2;
        byte b2 = 3 << 3 + 2;
        //printf("%u %u %u\n", b0, b1, b2);
        assert_eq(b0, 24);
        assert_eq(b1, 26);
        assert_eq(b2, 96);

        word w = 0x4321;
        w <<= 8;
        assert_eq(w, 0x2100);
        w = 0x4321;
        w <<= 5;
        assert_eq(w, 0x6420);

        return 0;
    }
    ',
expected => ""
},

{
title => q{Integer (non-char) constants are words},
program => '
    word f(unsigned word n) { return n; }
    int main()
    {
        assert_eq(f(600),                   600);
        assert_eq(f(60 * 10),               600);
        byte a = 60, b = 10;
        assert_eq(f(a * b),                  88);  // byte * byte done in byte, so bits lost
        assert_eq(f((word) 60 * 10),        600);
        assert_eq(f(60 * (word) 10),        600);
        assert_eq(f((word) 60 * (word) 10), 600);

        assert_eq(f(100 + 99),              199);
        assert_eq(f(200 + 100),             300);
        assert_eq(f(200 + (word) 100),      300);
        return 0;
    }
    ',
expected => ""
},

{
title => q{Taking address and dereferencing},
program => '
    struct S { byte b; };
    void changeByte(unsigned char *pb) { *pb = 42; }
    void changeWord(unsigned int *pw) { *pw = 1844; }
    int main()
    {
        byte b = 0;
        changeByte(&b);
        word w = 0;
        changeWord(&w);
        struct S s;
        s.b = 99;
        assert_eq(s.b, 99);
        struct S *ps = &s;
        assert_eq(ps->b, 99);
        byte *pb = &b;
        byte **ppb = &pb;
        word *pw = &w;
        word **ppw = &pw;  // address of T * is word *
        //printf("%u %u %u %u\n", b, w, b + w, w + b);
        assert_eq(b, 42);
        assert_eq(w, 1844);
        assert_eq(b + w, 1886);
        assert_eq(w + b, 1886);
        assert_eq(*pb, 42);
        assert_eq(*pw, 1844);
        assert_eq(**ppb, 42);
        assert_eq(**ppw, 1844);
        
        return 0;
    }
    ',
expected => ""
},

{
title => q{strcmp(), memcmp(), strlen()},
program => '
    int main()
    {
        assert_eq(strcmp("", ""), 0);
        assert_eq(strcmp("", "x"), -1);
        assert_eq(strcmp("x", ""), 1);
        assert_eq(strcmp("x", "x"), 0);
        assert_eq(strcmp("x", "xy"), -1);
        assert_eq(strcmp("xy", "x"), 1);
        assert_eq(strcmp("xy", "xyz"), -1);
        assert_eq(strcmp("xyz", "xy"), 1);
        assert_eq(strcmp("xyz", "xya"), 1);
        assert_eq(strcmp("xyz", "xay"), 1);
        assert_eq(strcmp("xyz", "axy"), 1);
        assert_eq(strcmp("xya", "xyz"), -1);
        assert_eq(strcmp("xay", "xyz"), -1);
        assert_eq(strcmp("axy", "xyz"), -1);

        assert_eq(stricmp("", ""), 0);
        assert_eq(stricmp("", "x"), -1);
        assert_eq(stricmp("x", ""), 1);
        assert_eq(stricmp("x", "x"), 0);
        assert_eq(stricmp("x", "X"), 0);
        assert_eq(stricmp("X", "x"), 0);
        assert_eq(stricmp("X", "X"), 0);
        assert_eq(stricmp("x", "xy"), -1);
        assert_eq(stricmp("X", "xy"), -1);
        assert_eq(stricmp("x", "XY"), -1);
        assert_eq(stricmp("X", "XY"), -1);
        assert_eq(stricmp("xy", "x"), 1);
        assert_eq(stricmp("xy", "xyz"), -1);
        assert_eq(stricmp("xyz", "xy"), 1);
        assert_eq(stricmp("xyz", "xya"), 1);
        assert_eq(stricmp("XYZ", "xya"), 1);
        assert_eq(stricmp("xyz", "XYA"), 1);
        assert_eq(stricmp("XYZ", "XYA"), 1);
        assert_eq(stricmp("xyz", "xyz"), 0);
        assert_eq(stricmp("XYZ", "xyz"), 0);
        assert_eq(stricmp("xyz", "XYZ"), 0);
        assert_eq(stricmp("XYZ", "XYZ"), 0);
        assert_eq(stricmp("xyz", "xay"), 1);
        assert_eq(stricmp("xyz", "axy"), 1);
        assert_eq(stricmp("xya", "xyz"), -1);
        assert_eq(stricmp("xay", "xyz"), -1);
        assert_eq(stricmp("axy", "xyz"), -1);

        assert_eq(memcmp(0, 0, 0), 0);
        assert_eq(memcmp("aaa", "bbb", 3), -1);
        assert_eq(memcmp("fgh", "feh", 3), 1);
        assert_eq(memcmp("aca", "ada", 3), -1);
        assert_eq(memcmp("ada", "ada", 3), 0);
        assert_eq(memcmp("ada", "adaX", 3), 0);

        assert_eq(memicmp(0, 0, 0), 0);
        assert_eq(memicmp("aaa", "bbb", 3), -1);
        assert_eq(memicmp("AAA", "bbb", 3), -1);
        assert_eq(memicmp("aaa", "BBB", 3), -1);
        assert_eq(memicmp("AAA", "BBB", 3), -1);
        assert_eq(memicmp("fgh", "feh", 3), 1);
        assert_eq(memicmp("FGH", "feh", 3), 1);
        assert_eq(memicmp("fgh", "FEH", 3), 1);
        assert_eq(memicmp("FGH", "FEH", 3), 1);
        assert_eq(memicmp("aca", "ada", 3), -1);
        assert_eq(memicmp("ACA", "ada", 3), -1);
        assert_eq(memicmp("aca", "ADA", 3), -1);
        assert_eq(memicmp("ACA", "ADA", 3), -1);
        assert_eq(memicmp("ada", "ada", 3), 0);
        assert_eq(memicmp("ada", "ADA", 3), 0);
        assert_eq(memicmp("ADA", "ada", 3), 0);
        assert_eq(memicmp("ADA", "ADA", 3), 0);
        assert_eq(memicmp("ada", "adaX", 3), 0);
        assert_eq(memicmp("aDa", "adaX", 3), 0);
        assert_eq(memicmp("ada", "aDaX", 3), 0);
        assert_eq(memicmp("aDa", "aDaX", 3), 0);

        assert_eq(strlen(""), 0);
        assert_eq(strlen("foobar"), 6);
        return 0;
    }
    ',
expected => ""
},


{
title => q{byte *memcpy(byte *, byte *, word)},
program => '
    char globalBuffer[7];

    void check(char *buffer)
    {
        buffer[0] = 42;
        memcpy(buffer, "____", 0);  // must do nothing
        assert_eq(buffer[0], 42);

        void *out = memcpy(buffer, "foobar", 7);
        assert_eq(out, buffer);
        assert_eq(strcmp(buffer, "foobar"), 0);
    }

    int main()
    {
        char localBuffer[7];
        check(localBuffer);
        check(globalBuffer);
        check(localBuffer);
        check(globalBuffer);
        return 0;
    }
    ',
expected => ""
},


{
title => q{byte *memset(byte *s, byte c, word n)},
program => '
    char globalBuffer[300];

    void check(char *buffer)
    {
        void *ret = memset(buffer, 42, 300);
        assert_eq(ret, buffer);
        for (int i = 0; i < 300; ++i)
            assert_eq(buffer[i], 42);
    }

    int main()
    {
        char localBuffer[300];
        check(localBuffer);
        check(globalBuffer);
        check(localBuffer);
        check(globalBuffer);
        return 0;
    }
    ',
expected => ""
},


{
title => q{strcpy(), strncpy(), strcat()},
program => q{
    #define BUFSIZ 11
    char globalBuffer[BUFSIZ];

    void check(char *buffer)
    {
        assert(buffer);

        buffer[0] = 42;
        buffer[1] = 43;
        char *out = strcpy(buffer, "");  // must copy NUL
        assert_eq(out, buffer);
        assert_eq(buffer[0], 0);   // NUL terminator
        assert_eq(buffer[1], 43);  // unaffected by strcpy() call

        char *out0 = strcpy(buffer, "foobar");
        assert_eq(out0, buffer);
        char *out2 = strcat(buffer, "");  // must do nothing
        assert_eq(out2, buffer);
        char *out1 = strcat(buffer, "baz");
        assert_eq(out1, buffer);
        assert_eq(strcmp(buffer, "foobarbaz"), 0);

        buffer[0] = 0;
        strcat(buffer, "");
        assert_eq(strlen(buffer), 0);
        strcat(buffer, "quux");
        assert_eq(strlen(buffer), 4);
        
        strncpy(buffer, "foo", BUFSIZ);
        assert(!strcmp(buffer, "foo"));
        for (char i = 3; i < BUFSIZ; ++i)
            assert_eq(buffer[i], 0);
        
        strncpy(buffer, "abcdefghij", BUFSIZ);
        assert(!strcmp(buffer, "abcdefghij"));

        strncpy(buffer, "Now is the time", 3);
        assert_eq(buffer[0], 'N');
        assert_eq(buffer[1], 'o');
        assert_eq(buffer[2], 'w');

        strncpy(buffer, "ABCDEFGHIJKLMNOP", BUFSIZ);
        assert_eq(buffer[BUFSIZ - 1], 'K');
        buffer[BUFSIZ - 1] = 0;
        assert(!strcmp(buffer, "ABCDEFGHIJ"));
    }

    int main()
    {
        char localBuffer[BUFSIZ];
        check(localBuffer);
        check(globalBuffer);
        check(localBuffer);
        check(globalBuffer);
        return 0;
    }
    },
expected => ""
},


{
title => q{char *strchr(char *, int)},
program => q!
    int main()
    {
        const char *s0 = "foobar";
        char *foundAt = strchr(s0, 'b');
        assert_eq(foundAt, s0 + 3);
        assert_eq(strchr(s0, 'f'), s0);
        assert_eq(strchr(s0, 'o'), s0 + 1);
        assert_eq(strchr(s0, '_'), 0);
        assert_eq(strchr(s0, 0), s0 + 6);
        const char *empty = "";
        assert_eq(strchr(empty, 0), empty);
        assert_eq(strchr(empty, '_'), 0);
        return 0;
    }
    !,
expected => ""
},


{
title => q{char *strstr(const char *haystack, const char *needle)},
program => q!
    int main()
    {
        const char *h = "foobar";
        assert_eq(strstr(h, "f"), h);
        assert_eq(strstr(h, "foo"), h);
        assert_eq(strstr(h, "foobar"), h);
        assert_eq(strstr(h, "b"), h + 3);
        assert_eq(strstr(h, "ba"), h + 3);
        assert_eq(strstr(h, "bar"), h + 3);
        assert_eq(strstr(h, "baz"), 0);
        assert_eq(strstr(h, "x"), 0);
        assert_eq(strstr(h, "xy"), 0);
        assert_eq(strstr(h, ""), h);
        
        h = "foobarbaz";
        assert_eq(strstr(h, "bar"), h + 3);
        assert_eq(strstr(h, "baz"), h + 6);

        h = "";
        assert_eq(strstr(h, "f"), 0);
        assert_eq(strstr(h, "foo"), 0);
        assert_eq(strstr(h, "foobar"), 0);
        assert_eq(strstr(h, "b"), 0);
        assert_eq(strstr(h, "ba"), 0);
        assert_eq(strstr(h, "bar"), 0);
        assert_eq(strstr(h, "x"), 0);
        assert_eq(strstr(h, "xy"), 0);
        assert_eq(strstr(h, ""), h);

        return 0;
    }
    !,
expected => ""
},


{
title => q{char *strlwr(char *)},
program => q!
    char globalBuffer[7];

    void check(char *buffer)
    {
        strcpy(buffer, "");
        strlwr(buffer);
        assert_eq(strlen(buffer), 0);

        strcpy(buffer, "FOOBAR");
        strlwr(buffer);
        assert_eq(strlen(buffer), 6);
        assert_eq(strcmp(buffer, "foobar"), 0);

        strcpy(buffer, "foobar");
        strlwr(buffer);
        assert_eq(strlen(buffer), 6);
        assert_eq(strcmp(buffer, "foobar"), 0);
    }

    int main()
    {
        char localBuffer[7];
        check(localBuffer);
        check(globalBuffer);
        check(localBuffer);
        check(globalBuffer);
        return 0;
    }
    !,
expected => ""
},


{
title => q{char *strupr(char *)},
program => q!
    char globalBuffer[7];

    void check(char *buffer)
    {
        strcpy(buffer, "");
        strupr(buffer);
        assert_eq(strlen(buffer), 0);

        strcpy(buffer, "FOOBAR");
        strupr(buffer);
        assert_eq(strlen(buffer), 6);
        assert_eq(strcmp(buffer, "FOOBAR"), 0);

        strcpy(buffer, "foobar");
        strupr(buffer);
        assert_eq(strlen(buffer), 6);
        assert_eq(strcmp(buffer, "FOOBAR"), 0);
    }
 
    int main()
    {
        char localBuffer[7];
        check(localBuffer);
        check(globalBuffer);
        return 0;
    }
    !,
expected => ""
},


{
title => q{Conditional expression does not use the TST instruction},
program => q`
    int main()
    {
        // The TST instruction does not affect the carry, so it is inappropriate
        // for LBHI et al.

        byte counter = 0;
        word w = 1;
        asm("orcc", "#1");  // force carry
        while (w > 0)  // this comparison assumed to reset carry, for LBHI
        {
            ++counter;
            break;
        }
        assert_eq(counter, 1);

        byte b = 1;
        asm("orcc", "#1");  // force carry
        while (b > 0)  // this comparison assumed to reset carry, for LBHI
        {
            ++counter;
            break;
        }
        assert_eq(counter, 2);
        return 0;
    }
    `,
expected => ""
},


{
title => q{Array name is address of 1st element},
program => q`
    void *f(void *a) { return a; }
    int main()
    {
        unsigned a[1];
        a[0] = '$';
        unsigned *p = a;
        unsigned *q;
        q = a;
        assert_eq(p, a);
        assert_eq(p, q);
        assert_eq(a, &a);  // unneeded &a is tolerated
        assert_eq(f(a), a);
        assert_eq(f(&a), a);  // &a also tolerated in arg passing

        char b[5];
        b[0] = 'a';
        * (unsigned *) (b + 1) = 0x6263;
        b[3] = 'd';
        b[4] = 0;
        assert_eq(strcmp(b, "abcd"), 0);
        assert_eq(&b, b);
        assert_eq(f(b), b);
        assert_eq(f(&b), b);

        assert_eq((b - 1) + 1, b);
        assert_eq((1 + b) - 1, b);

        assert_eq(*b, 97);

        assert_eq(a, &a[0]);
        assert_eq(b, &b[0]);

        // Address of T[] is of type T *.
        unsigned *a1 = &a;
        assert_eq(a1, a);
        assert_eq(a1[0], a[0]);
        char *b1 = &b;
        assert_eq(b1, b);
        assert_eq(b1[0], b[0]);

        // Negate an array name.
        char boolNegation = !b;
        assert_eq(boolNegation, 0);
        assert(!!b);

        unsigned *nullPtr = 0;
        boolNegation = !nullPtr;
        assert_ne(boolNegation, 0);  // not necessarily 1
        return 0;
    }
    `,
expected => ""
},


{
title => q{dwtoa(): double word to ASCII},
program => q`
    int main()
    {
        char buffer[11];
        assert_eq(strcmp(dwtoa(buffer, 0, 0),             "0"), 0);
        assert_eq(strcmp(buffer, "0000000000"), 0);
        assert_eq(strcmp(dwtoa(buffer, 0, 9),             "9"), 0);
        assert_eq(strcmp(buffer, "0000000009"), 0);
        assert_eq(strcmp(dwtoa(buffer, 9, 0),             "589824"), 0);
        assert_eq(strcmp(dwtoa(buffer, 1, 1),             "65537"), 0);
        assert_eq(strcmp(buffer, "0000065537"), 0);
        assert_eq(strcmp(dwtoa(buffer, 14882, 31661),     "975338413"), 0);
        assert_eq(strcmp(buffer, "0975338413"), 0);

        assert_eq(strcmp(dwtoa(buffer, 55857, 33919),     "3660678271"), 0);
        assert_eq(strcmp(dwtoa(buffer,  4190, 26415),     "274622255"), 0);
        assert_eq(strcmp(dwtoa(buffer, 43262, 54960),     "2835273392"), 0);
        assert_eq(strcmp(dwtoa(buffer,  6658, 15199),     "436353887"), 0);
        assert_eq(strcmp(dwtoa(buffer, 20559,  4689),     "1347359313"), 0);
        assert_eq(strcmp(dwtoa(buffer,  5076, 54251),     "332714987"), 0);
        assert_eq(strcmp(dwtoa(buffer, 44962,  4995),     "2946634627"), 0);
        assert_eq(strcmp(dwtoa(buffer,  1896, 16326),     "124272582"), 0);
        assert_eq(strcmp(dwtoa(buffer, 18019,  7597),     "1180900781"), 0);
        assert_eq(strcmp(dwtoa(buffer, 49676, 49489),     "3255615825"), 0);
        assert_eq(strcmp(buffer, "3255615825"), 0);
        return 0;
    }
    `,
expected => ""
},


{
title => q{div328() (division of 32 bits by 8, unsigned)},
program => q`
    word counter = 0;

    void check8(word hi, word lo, byte divisor, word expHi, word expLo)
    {
        ++counter;

        word dividend[2] = { hi, lo };  // copy dividend here, which will be overwritten w/ quotient

        divdwb(dividend, divisor);

        word *actualQuotient = dividend;  // new name, to clarify
        if (actualQuotient[0] != expHi || actualQuotient[1] != expLo)
        {
            printf("ERROR: check8 #%2u: 0x%04x%04x / 0x%02x: got 0x%04x%04x, expected 0x%04x%04x\n",
                    counter,
                    hi, lo, divisor,
                    actualQuotient[0], actualQuotient[1],
                    expHi, expLo);
        }
    }

    void check16(word hi, word lo, word divisor, word expHi, word expLo)
    {
        ++counter;

        word dividend[2] = { hi, lo };  // copy dividend here, which will be overwritten w/ quotient

        divdww(dividend, divisor);

        word *actualQuotient = dividend;  // new name, to clarify
        if (actualQuotient[0] != expHi || actualQuotient[1] != expLo)
        {
            printf("ERROR: check16 #%2u: 0x%04x%04x / 0x%04x: got 0x%04x%04x, expected 0x%04x%04x\n",
                    counter,
                    hi, lo, divisor,
                    actualQuotient[0], actualQuotient[1],
                    expHi, expLo);
        }
    }

    int main()
    {
        check8(0, 0, 1, 0, 0);
        check8(0, 100, 1, 0, 100);
        check8(0, 100, 10, 0, 10);
        check8(0, 1000, 10, 0, 100);
        check8(0, 10000, 10, 0, 1000);
        check8(0x0001, 0x86a0, 10, 0, 10000);
        check8(0xffff, 0xffff, 0xff, 0x0101, 0x0101);
        check8(0x0001, 0x86a0, 0, 0x0001, 0x86a0);  // nothing done on division by zero

        check8(0x7079, 0x34e7, 0x01, 0x7079, 0x34e7);
        check8(0x7079, 0x34e7, 0x02, 0x383c, 0x9a73);
        check8(0x7079, 0x34e7, 0xd5, 0x0087, 0x2deb);
        check8(0x5523, 0x0a32, 0x3f, 0x0159, 0xf3f8);
        check8(0x1fb2, 0x380c, 0x5b, 0x0059, 0x2ad0);
        check8(0x7436, 0x365b, 0x69, 0x011b, 0x55d9);
        check8(0x353e, 0x1c6e, 0x09, 0x05ea, 0x74ef);
        check8(0x0eed, 0x5e1c, 0x79, 0x001f, 0x94e0);
        check8(0x1658, 0x508d, 0x36, 0x0069, 0xee87);
        check8(0x1add, 0x1fbd, 0x99, 0x002c, 0xf2d2);
        check8(0x7ab5, 0x31f8, 0x72, 0x0113, 0x8de9);
        check8(0x2179, 0x7f99, 0x2e, 0x00ba, 0x4b1f);
        check8(0x3c94, 0x103a, 0x4a, 0x00d1, 0x9184);
        check8(0x34ad, 0x247d, 0x72, 0x0076, 0x4a6c);
        check8(0x5cb2, 0x2e33, 0x07, 0x0d3e, 0x0699);
        check8(0x4466, 0x176e, 0x94, 0x0076, 0x4fb9);
        check8(0x4ef9, 0x7383, 0xe9, 0x0056, 0xc52a);
        check8(0x4e52, 0x55e0, 0x11, 0x049b, 0x6e76);
        check8(0x5a5e, 0x3dc7, 0x54, 0x0113, 0x685a);
        check8(0x7b3f, 0x0ded, 0x18, 0x0522, 0xa094);
        check8(0x6408, 0x6c79, 0x23, 0x02db, 0xab53);
        check8(0x63ae, 0x2104, 0x09, 0x0b13, 0x5900);
        check8(0x0a07, 0x1a4d, 0x6c, 0x0017, 0xc4fb);
        check8(0x7ba4, 0x007f, 0xe3, 0x008b, 0x6fa6);
        check8(0x697b, 0x3f16, 0x14, 0x0546, 0x298d);
        check8(0x22f0, 0x3dfe, 0xc3, 0x002d, 0xde2f);
        check8(0x1aa7, 0x6847, 0x2c, 0x009b, 0x13d3);
        check8(0x5a24, 0x4b58, 0xa5, 0x008b, 0xdb38);
        check8(0x081e, 0x7857, 0xc6, 0x000a, 0x7f50);
        check8(0x23f5, 0x7f57, 0xc0, 0x002f, 0xf1ff);
        check8(0x0d07, 0x71ce, 0xc8, 0x0010, 0xad5e);
        check8(0x172e, 0x32d5, 0x4c, 0x004e, 0x14e1);

        counter = 0;

        check16(0, 0, 1, 0, 0);
        check16(0, 100, 1, 0, 100);
        check16(0, 100, 10, 0, 10);
        check16(0, 1000, 10, 0, 100);
        check16(0, 10000, 10, 0, 1000);
        check16(0x0001, 0x86a0, 10, 0, 10000);
        check16(0xffff, 0xffff, 0xff, 0x0101, 0x0101);
        check16(0xffff, 0xffff, 0xffff, 0x0001, 0x0001);
        check16(0x0001, 0x86a0, 0, 0x0001, 0x86a0);  // nothing done on division by zero

        check16(0xedc0, 0xbbaf, 0xe57d, 0x0001, 0x0938);
        check16(0x41d5, 0x8708, 0x20e8, 0x0002, 0x002b);
        check16(0x428b, 0xbbb5, 0x1d43, 0x0002, 0x462f);
        check16(0xc5cd, 0x5506, 0x99e2, 0x0001, 0x4910);
        check16(0xb738, 0x4cc9, 0x55e1, 0x0002, 0x222a);
        check16(0xa676, 0x3203, 0x0d68, 0x000c, 0x6aae);
        check16(0x0518, 0xa8ab, 0xdc57, 0x0000, 0x05eb);
        check16(0x8ff2, 0xb487, 0x8e7c, 0x0001, 0x02a1);
        check16(0xe671, 0x6eda, 0x4ba2, 0x0003, 0x0bff);
        check16(0xe4c2, 0xe584, 0x0b65, 0x0014, 0x138f);
        check16(0x080b, 0x33b8, 0x6cad, 0x0000, 0x12f2);
        check16(0x398e, 0xb786, 0xe5e5, 0x0000, 0x4017);
        check16(0xfb65, 0x6cc8, 0xcbc8, 0x0001, 0x3bd0);
        check16(0x58c4, 0x3d29, 0x6d7b, 0x0000, 0xcf90);
        check16(0xdc3b, 0x4e59, 0x2dbd, 0x0004, 0xd0a6);
        check16(0x7e3e, 0x52ca, 0x8bb8, 0x0000, 0xe74f);
        check16(0x7e00, 0x6e4a, 0x0db8, 0x0009, 0x2f44);
        check16(0xfd3f, 0xaf80, 0x03a5, 0x0045, 0x7cc2);
        check16(0x3a04, 0x4023, 0x003a, 0x0100, 0x12c2);
        check16(0xc759, 0x914c, 0x00a1, 0x013c, 0xfa8a);
        check16(0x6a48, 0xbe6b, 0x0d49, 0x0008, 0x000e);
        check16(0xc094, 0x7890, 0x0f76, 0x000c, 0x74b6);
        check16(0x6109, 0xd539, 0x0241, 0x002b, 0x0dae);
        check16(0x97a4, 0xa6be, 0x088b, 0x0011, 0xc029);
        check16(0x36b8, 0x620a, 0x0541, 0x000a, 0x6a47);
        check16(0x0f5b, 0x5451, 0x01eb, 0x0008, 0x01bc);
        check16(0x2104, 0xdcd5, 0x02cb, 0x000b, 0xd27a);
        check16(0x9420, 0xd1ff, 0x0075, 0x0144, 0x1c0d);
        check16(0x8ac2, 0xfe5b, 0x0bd0, 0x000b, 0xbf3c);
        check16(0x5c53, 0x092f, 0x0307, 0x001e, 0x7f2f);
        check16(0xc730, 0xf7c3, 0x040c, 0x0031, 0x3894);
        check16(0xa027, 0x0fa8, 0x0df5, 0x000b, 0x7985);

        return 0;
    }
    `,
expected => ""
},


{
title => q{mulwb(): multiply word by byte, giving a 24-bit result},
program => q`
    int main()
    {
        unsigned char hi; unsigned int lo;  // declared in this order, so that hi is allocated before lo

        lo = mulwb(&hi, 9 * 256, 68);
        assert_eq(hi, 0x02);
        assert_eq(lo, 0x6400);

        char buffer[11];
        char *firstNonZeroChar = dwtoa(buffer, (word) hi, lo);
        //printf("buffer=[%s], firstNonZeroChar=[%s]\n", buffer, firstNonZeroChar);
        assert_eq(strcmp(firstNonZeroChar, "156672"), 0);
        return 0;
    }
    `,
expected => ""
},


{
title => q{mulww(): multiply word by word, giving a 32-bit result},
program => q`
    // u * v expected to give j:k.
    void check(word u, word v, word j, word k)
    {
        word hi; word lo;  // declared in this order, so that hi is allocated before lo
        lo = mulww(&hi, u, v);
        if (hi != j || lo != k)
            printf("UNEXPECTED: 0x%04x * 0x%04x gives 0x%04x:0x%04x, expected 0x%04x:0x%04x\n",
                    u, v, hi, lo, j, k);
    }

    int main()
    {
        word hi; word lo;  // declared in this order, so that hi is allocated before lo

        lo = mulww(&hi, 10000, 20000);
        //printf("0x%04x 0x%04x\n", hi, lo);
        assert_eq(hi, 0x0BEB);
        assert_eq(lo, 0xC200);

        char buffer[11];
        char *p = dwtoa(buffer, hi, lo);
        assert_eq(strcmp(p, "200000000"), 0);

        check(0x0000, 0x0000, 0x0000, 0x0000);
        check(0x0001, 0x0000, 0x0000, 0x0000);
        check(0x0000, 0x0001, 0x0000, 0x0000);
        check(0x0001, 0x0001, 0x0000, 0x0001);
        check(0x0100, 0x0100, 0x0001, 0x0000);
        check(0xffff, 0xffff, 0xfffe, 0x0001);

        check(0xbf6f, 0x1428, 0x0f12, 0x9558);
        check(0xbbf2, 0xacca, 0x7eda, 0xe4f4);
        check(0x3459, 0x692c, 0x1581, 0x804c);
        check(0x9cd9, 0x7b8e, 0x4bb3, 0x435e);
        check(0xf349, 0x24b4, 0x22e1, 0x5354);
        check(0xb951, 0x38f3, 0x2939, 0x9fe3);
        check(0xd8ad, 0xc03b, 0xa2b3, 0xafdf);
        check(0x2e50, 0x189b, 0x0473, 0x8a70);
        check(0x1adf, 0x6818, 0x0aed, 0x1ce8);
        check(0x1c1a, 0x3ee1, 0x06e6, 0xfeda);
        check(0xf069, 0xea64, 0xdc1d, 0xe304);
        check(0x1e40, 0xd984, 0x19b3, 0xd900);
        check(0xd1e5, 0x5a57, 0x4a11, 0xd6d3);
        check(0xde6a, 0xb155, 0x9a11, 0x2332);
        check(0xba35, 0x8253, 0x5ecb, 0x492f);
        check(0xa233, 0xf502, 0x9b3c, 0x1366);
        check(0xe9b0, 0x0b9d, 0x0a99, 0xe0f0);
        check(0x364d, 0x4495, 0x0e8c, 0x0ed1);
        check(0xf4f0, 0x8a55, 0x845a, 0xb3b0);
        check(0xdd9e, 0xb4c9, 0x9c81, 0x190e);
        check(0xdb5f, 0x025a, 0x0203, 0xdd66);
        check(0x6096, 0x7725, 0x2cf3, 0xafae);
        check(0xa56d, 0xa715, 0x6bf7, 0xacf1);
        check(0x2393, 0x928f, 0x145d, 0xb51d);
        check(0x9438, 0x7894, 0x45cf, 0xf060);
        check(0x6184, 0x0559, 0x0209, 0x7ae4);
        check(0x4548, 0x6455, 0x1b27, 0x20e8);
        check(0x9591, 0x5075, 0x2f01, 0xab45);
        check(0xb226, 0xec08, 0xa440, 0x9930);
        check(0x82ab, 0xb97c, 0x5eac, 0xddd4);
        return 0;
    }
    `,
expected => ""
},


{
title => q{zerodw, addddw, subdww, cmpdww},
program => q`
    int main()
    {
        word dw[2];
        zerodw(dw);
        //printf("0x%04x%04x\n", dw[0], dw[1]);
        assert_eq(dw[0], 0);
        assert_eq(dw[1], 0);
        assert_eq(cmpdww(dw, 0), 0);

        adddww(dw, 0);
        //printf("0x%04x%04x\n", dw[0], dw[1]);
        assert_eq(dw[0], 0);
        assert_eq(dw[1], 0);
        assert_eq(cmpdww(dw, 0), 0);

        adddww(dw, 42);
        //printf("0x%04x%04x\n", dw[0], dw[1]);
        assert_eq(dw[0], 0);
        assert_eq(dw[1], 42);
        assert_eq(cmpdww(dw, 42), 0);

        adddww(dw, 0xFFFF);
        //printf("0x%04x%04x\n", dw[0], dw[1]);
        assert_eq(dw[0], 1);
        assert_eq(dw[1], 41);
        assert_eq(cmpdww(dw, 1000), 1);

        subdww(dw, 0xFFFF);
        assert_eq(dw[0], 0);
        assert_eq(dw[1], 42);
        assert_eq(cmpdww(dw, 42), 0);
        assert_eq(cmpdww(dw, 0), 1);
        assert_eq(cmpdww(dw, 10), 1);
        assert_eq(cmpdww(dw, 100), -1);
        assert_eq(cmpdww(dw, 0xFFFF), -1);

        subdww(dw, 42);
        assert_eq(dw[0], 0);
        assert_eq(dw[1], 0);
        assert_eq(cmpdww(dw, 0), 0);

        subdww(dw, 0);
        assert_eq(dw[0], 0);
        assert_eq(dw[1], 0);
        assert_eq(cmpdww(dw, 0), 0);

        return 0;
    }
    `,
expected => ""
},


{
title => q{sqrt16()},
program => q`
    int main()
    {
        for (byte i = 0; ; ++i)
        {
            word square = (word) i * i;
            assert_eq(sqrt16(square), i);
            if (i > 0)
            {
                assert_eq(sqrt16(square + 1), i);
                assert_eq(sqrt16(square + 2), i);
                if (i < 255)
                {
                    word nextSquare = (word) (i + 1) * (i + 1);
                    assert_eq(sqrt16(nextSquare - 1), i);
                }
            }
            if (i == 255)
                break;
        }
        return 0;
    }
    `,
expected => ""
},


{
title => q{divmod16() and divmod8()},
program => q`
    int main()
    {
        unsigned qw, rw;
        divmod16(12345, 1000, &qw, &rw);
        assert_eq(qw, 12);
        assert_eq(rw, 345);
        unsigned char qb, rb;
        divmod8(234, 100, &qb, &rb);
        assert_eq(qb, 2);
        assert_eq(rb, 34);
        return 0;
    }
    `,
expected => ""
},


{
title => q{Optimization of adds and subs of constants},
program => q`
    int main()
    {
        word w;
        byte b;
        w = 1000 + 2000;
        assert(w == 3000);
        w = 9000 - 3000;
        assert(w == 6000);
        w = 4 - 20;
        assert(w == 0xfff0);
        w = 65535 + 11;
        assert(w == 10);
        b = 10 + 20;
        assert(b == 30);
        b = 230 - 25;
        assert(b == 205);
        b = 5 - 20;
        assert(b == 241);
        b = (byte) (255 + 11);
        assert(b == 10);
        
        // Signed cases.
        int i;
        i = 4 - 20;
        assert_eq(i, -16);
        
        return 0;
    }
    `,
expected => ""
},


{
title => q{printf's number and string field padding},
program => q`
    void asm hook()
    {
        asm {
            sta     $ff00    // assumes USim
            lda     #'?'     // trash A to test printf() (re: zero-padding)
        }
    }
    int main()
    {
        sbyte m1 = -1;
        printf("[%2d]\n", m1);
        printf("[%5d]\n", m1);
        
        printf("[%1x]\n", 10);
        printf("[%2x]\n", 10);
        printf("[%5x]\n", 10);
        printf("[%12x]\n", 10);

        printf("[%1x]\n", 0xFED);
        printf("[%2x]\n", 0xFED);
        printf("[%5x]\n", 0xFED);
        printf("[%12x]\n", 0xFED);

        printf("[%1x]\n", 0xABCD);
        printf("[%2x]\n", 0xABCD);
        printf("[%5x]\n", 0xABCD);
        printf("[%12x]\n", 0xABCD);

        const char *eightChars = "abcdefgh";
        printf("[%-8s]\n", eightChars);

        printf("%s", "");
        byte a[2];
        a[0] = 'A'; a[1] = 0;
        byte b[2];
        b[0] = 'B'; b[1] = 0;
        printf("%s %s\n", a, b);
        printf("[%12s]\n", "foo");
        printf("[%-12s]\n", "foo");
        printf("[%s]\n", "foo");
        printf("%03u %3u\n", 5, 6);
        word w = 65535;
        printf("%d\n", (sword) w);
        
        // With redirected character output.
        ConsoleOutHook oldCHROOT = setConsoleOutHook(hook);
        printf("0x%04X,%06u\n", 1, 2);
        setConsoleOutHook(oldCHROOT);

        return 0;
    }
    `,
expected => "[-1]\n[   -1]\n[A]\n[ A]\n[    A]\n[           A]\n[FED]\n[FED]\n[  FED]\n[         FED]\n[ABCD]\n[ABCD]\n[ ABCD]\n[        ABCD]\n[abcdefgh]\nA B\n[         foo]\n[foo         ]\n[foo]\n005   6\n-1\n0x0001,000002\n"
},


{
title => q{printf %x, %X and %p (hex digits always in upper-case)},
program => q`
    int main()
    {
        printf("%x\n", 65535);
        printf("%X\n", 65535);
        printf("%6X\n", 65535);
        printf("%06X\n", 65535);
        printf("%p\n", 65535);
        printf("%p\n", 0xfff);
        printf("%p\n", 0Xff);  // tests capital X
        printf("%p\n", 0xf);
        printf("%p\n", 0x0);
        return 0;
    }
    `,
expected => "FFFF\nFFFF\n  FFFF\n00FFFF\n\$FFFF\n\$0FFF\n\$00FF\n\$000F\n\$0000\n"
},


{
title => q{toupper(), tolower()},
program => q`
    int main()
    {
        int c = toupper('a');
        assert_eq(c, 'A');
        c = toupper('z');
        assert_eq(c, 'Z');
        c = toupper('J');
        assert_eq(c, 'J');
        c = toupper('*');
        assert_eq(c, '*');

        c = tolower('A');
        assert_eq(c, 'a');
        c = tolower('Z');
        assert_eq(c, 'z');
        c = tolower('j');
        assert_eq(c, 'j');
        c = tolower('*');
        assert_eq(c, '*');
        return 0;
    }
    `,
expected => ""
},


{
title => q{struct},
program => q`
    struct S
    {
        byte a;
        word b;
        byte *c;
        word *d;
    };
    struct Outer
    {
        word stuff;
        struct S first;
        struct S second;
    };
    struct F
    {
        word w;
        byte sec[16];
    };
    word diff(byte *after, byte *before)
    {
        return after - before;
    }
    #define sizeofVar(varName) (diff((byte *) (&(varName) + 1), (byte *) &(varName)))

    void checkPtr(struct F *f)
    {
        //printf("f->w=0x%x, f->sec=0x%x, f=0x%x\n", (word) &f->w, (word) f->sec, (word) f);
        //printf("%u\n", (word) f->sec - (word) f);
        assert_eq((word) f->sec - (word) f, 2);
    }
    void check(struct S *p)
    {
        //printf("%u %u %u %u\n", p->a, p->b, *p->c, *p->d);
        assert_eq(p->a, 43);
        assert_eq(p->b, 1843);
        assert_eq(*p->c, 43);
        assert_eq(*p->d, 1843);
    }

    int main()
    {
        assert_eq(diff((byte *) 100, (byte *) 75), 25);

        struct S s;
        assert_eq(sizeofVar(s), 1 + 2 + 2 + 2);

        s.a = 42;
        s.b = 1844;
        s.c = &s.a;
        s.d = &s.b;
        //printf("%u %u %u %u\n", s.a, s.b, *s.c, *s.d);
        assert_eq(s.a, 42);
        assert_eq(s.b, 1844);
        assert_eq(*s.c, 42);
        assert_eq(*s.d, 1844);

        ++s.a;
        --s.b;
        //printf("%u %u %u %u\n", s.a, s.b, *s.c, *s.d);
        assert_eq(s.a, 43);
        assert_eq(s.b, 1843);
        assert_eq(*s.c, 43);
        assert_eq(*s.d, 1843);

        //printf("%u\n", s.a + s.b);
        assert_eq(s.a + s.b, 1886);

        check(&s);

        struct S *p = &s;
        p->a++;
        //printf("%u ", p->a);
        assert_eq(p->a, 44);
        ++p->a;
        //printf("%u ", p->a);
        assert_eq(p->a, 45);
        p->b++;
        //printf("%u ", p->b);
        assert_eq(p->b, 1844);
        ++p->b;
        //printf("%u\n", p->b);
        assert_eq(p->b, 1845);

        struct F f;
        assert_eq(sizeofVar(f), 2 + 16);
        //printf("%u\n", (word) f.sec - (word) &f);
        assert_eq((word) f.sec - (word) &f, 2);
        checkPtr(&f);

        struct Outer outer;
        assert_eq(sizeofVar(outer), 2 + 7 + 7);
        outer.first.b = 11;
        outer.second.b = 22;
        //printf("%u\n", outer.first.b + outer.second.b);
        assert_eq(outer.first.b + outer.second.b, 33);

        struct Outer *pOuter = &outer;
        pOuter->first.b = 33;
        pOuter->second.b = 44;
        //printf("%u\n", pOuter->first.b + pOuter->second.b);
        assert_eq(pOuter->first.b + pOuter->second.b, 77);

        struct S *pFirst = &outer.first;
        struct S *pSecond = &outer.second;
        pFirst->b = 55;
        pSecond->b = 66;
        //printf("%u\n", pFirst->b + pSecond->b);
        assert_eq(pFirst->b + pSecond->b, 55 + 66);

        pFirst = &pOuter->first;
        pSecond = &pOuter->second;
        pFirst->b = 77;
        pSecond->b = 88;
        //printf("%u\n", pFirst->b + pSecond->b);
        assert_eq(pFirst->b + pSecond->b, 77 + 88);
        return 0;
    }
    `,
expected => ""
},




{
title => q{struct with multi-member declaration},
program => q`
    struct S
    {
        char a, b;
        int c, d;
        char *e, f;  // e is a pointer but f is a char
        signed int g[2], h[3];  // two keyword to represent base type
        unsigned i;
    };
    int main()
    {
        struct S s;
        assert_eq(sizeof(struct S), 1+1+2+2+2+1+2*2+2*3+2); 
        assert_eq(sizeof(s.f), 1);
        assert_eq(sizeof(s.g), 4);  // size of struct member of array type
        assert_eq(sizeof(s.h), 6);
        assert(&s.a < &s.b);  // order in multi-member declaration is respected
        assert(&s.b < &s.c);  // order of declarations is respected
        assert(&s.c < &s.d);
        assert(&s.e < &s.f);
        assert(&s.h < &s.i);
        
        s.a = 'A';
        s.b = 'B';
        s.c = 1844;
        s.d = -4418;
        s.e = (char *) "foo";  // cast to avoid constness warning
        s.f = 'F';
        s.g[1] = 1111;
        s.h[2] = -2222;
        s.i = 3333;
        
        assert_eq(s.a, 'A');
        assert_eq(s.b, 'B');
        assert_eq(s.c, 1844);
        assert_eq(s.d, -4418);
        assert_eq(s.c + s.d, 1844 - 4418);
        assert_eq(strcmp(s.e, "foo"), 0);
        assert_eq(s.f, 'F');
        assert_eq(s.g[1], 1111);
        assert_eq(s.h[2], -2222);
        assert_eq(s.i, 3333);
        
        return 0;
    }
    `,
expected => ""
},


{
title => q{Struct definition local to a function},
program => q`
    int main()
    {
        struct S
        {
            int n;
            char a[3];
        };
        struct S s;
        s.n = 42;
        s.a[0] = 'x';
        s.a[1] = 'y';
        s.a[2] = 0;
        assert_eq(s.n, 42);
        assert(!strcmp(s.a, "xy"));
        return s.n;
    }
    `,
expected => ""
},


{
title => q{Reference to array member of a struct},
program => q`
    struct FileDesc
    {
        byte a;
        byte curSector[256];
        byte b;
    };
    struct FileDescW
    {
        word a;
        word curSector[256];
        word b;
    };
    int main()
    {
        struct FileDesc fd;
        fd.b = 42;
        assert_eq(fd.b, 42);
        fd.curSector[17] = 217;
        assert_eq(fd.curSector[17], 217);
        assert_eq(&fd, &fd.a);
        assert_eq((byte *) &fd + 1, fd.curSector);
        assert_eq((byte *) &fd + 1 + 17, &fd.curSector[17]);
        assert_eq((byte *) &fd + 257, &fd.b);

        struct FileDescW fdw;
        fdw.b = 9999;
        assert_eq(fdw.b, 9999);
        fdw.curSector[17] = 1844;
        assert_eq(fdw.curSector[17], 1844);
        assert_eq(&fdw, &fdw.a);
        assert_eq((byte *) &fdw + 2, fdw.curSector);
        assert_eq((byte *) &fdw + 2 + 17*2, &fdw.curSector[17]);
        assert_eq((byte *) &fdw + 257*2, &fdw.b);
        return 0;
    }
    `,
expected => ""
},


{
title => q{Taking address of a function and calling it indirectly},
program => q`
    // The name of a function, not followed by arguments, yields its address.
    // Using & in front of the name is also supported.
    // Calling through a pointer can be done as pf(...), (*pf)(...), or object.member(...).

    struct Class
    {
        int dummy;
        word (*method)(word);
    };
    word add(word n)
    {
        return 0x4000 + n;
    }
    void delegate(word (*funcPtr)(word))
    {
        assert_eq((*funcPtr)(0x0987), 0x4987);
    }
    int main()
    {
        add;  // function name by itself is OK; no need to assign it for stmt to be legal, although useless

        word (*pf)(word) = add;  // CMOC requires void pointer to represent function pointer
        assert(pf);

        assert_eq(pf(16), 16400);

        asm("ldd", "#0");  // to check that D gets loaded correctly
        asm("ldx", "#0");  // same for X if needed
        pf = add;
        if (pf != add)
        {
            printf("ERROR: pf contains %p instead of address of add() function\n", pf);
            return 1;
        }

        assert_eq(pf(116), 16500);

        assert_eq(sizeof((*pf)(0)), 2);  // return type of call through ptr is assumed to be int
        assert_eq(sizeof(pf(0)), 2);     // same assert with different notation

        struct Class cl;
        asm("ldd", "#0");  // to check that D gets loaded correctly
        asm("ldx", "#0");  // same for X if needed
        cl.method = add;
        assert(cl.method);
        if (cl.method != add)
        {
            printf("ERROR: cl.method contains %p instead of address of add() function\n", pf);
            return 1;
        }
        word t = (word) cl.method(216);  // always cast return value: compiler does not know return type
        assert_eq(t, 16600);
        
        pf = &add;  // ampersand allowed but does not change meaning
        assert_eq(pf(17), 16401);
        assert_eq((*pf)(18), 16402);  // cf FunctionCallExpr::check(), FunctionCallExpr::emitCode()
        
        delegate(add);  // pass add() address to a function that will call it
        delegate(t ? add : add);
        while (add)
            break;
        do { break; } while (add);
        for ( ; add ; )
            break;
        if (add)
            ;
        
        word (*pfAmp)(word) = &add;
        assert(pfAmp);
        assert_eq(pfAmp(17), 16401);
        assert_eq((*pfAmp)(17), 16401);
        
        return 0;
    }
    `,
expected => ""
},


{
title => q{Assignment of word to byte variable, and of byte to word variable},
tolerateWarnings => 1,
program => q`
    word aw[2] = { 0xabcd, 0x1234 };
    int main()
    {
        byte b = 42;
 
        b = aw[1];  // warning
        assert_eq(b, 0x34);

        word w = 0xeedd;
        b = w;  // warning
        assert_eq(b, 0xDD);

        w = b;
        assert_eq(w, 0xDD);

        byte o = '\0377';
        byte h = '\xff';
        assert_eq(o, 255);
        assert_eq(h, 255);
        o = 0;
        h = 0;
        o = '\0377';
        h = '\xff';
        assert_eq(o, 255);
        assert_eq(h, 255);
        
        b = 0xFEDC;  // warning
        assert_eq(b, 0xDC);
        return 0;
    }
    `,
expected => ""
},


{
title => q{Assignment used as sub-expression},
program => q`
    byte gb = 0;
    word gw = 0;
    byte fb(byte k) { gb = k; return gb; }
    word fw(word k) { gw = k; return gw; }
    byte *f() { return &gb; }
    byte *g() { return &gb; }
    byte  h() { return 88;  }
    int main()
    {
        byte b;
        byte c;
        (b = 0) = (c = 7);
        assert(b == 7);
        assert(c == 7);
        (b = 7) = (c = 0);
        assert(b == 0);
        assert(c == 0);

        word w;
        word z;
        (w = 0) = (z = 7);
        assert(w == 7);
        assert(z == 7);
        (w = 7) = (z = 0);
        assert(w == 0);
        assert(z == 0);

        b = 100;
        c = 200;
        (b += 0) = (c += 7);
        assert(b == 207);
        assert(c == 207);
        c = 150;
        (b += 7) = (c += 0);
        assert(b == 150);
        assert(c == 150);

        b = 0;
        gb = 0;
        (b = fb(33)) = 12;
        assert(b == 12);
        assert(gb == 33);

        w = 0;
        gw = 0;
        (w = fw(3333)) = 1212;
        assert(w == 1212);
        assert(gw == 3333);

        b = 0;
        c = 0;
        gb = 0;
        c = (b = fb(44));
        assert(b == 44);
        assert(c == 44);
        assert(gb == 44);

        c = b = fb(55);
        assert(b == 55);
        assert(c == 55);
        assert(gb == 55);

        // Test that second assignment is done before first one:
        gb = 222;
        *f() = *g() = h();  // this must print "hgf", reflecting right-to-left execution
        assert(gb == 88);
        return 0;
    }
    `,
expected => ""
},


{
title => q{while() and do/while()},
program => q`
    int main()
    {
        byte n = 0;
        do
            ++n;
        while (n < 5);
        assert_eq(n, 5);
        n = 0;
        while (n < 5)
            ++n;
        assert_eq(n, 5);

        n = 0;
        do
        {
            ++n;
        } while (n < 5);
        assert_eq(n, 5);
        n = 0;
        while (n < 5)
        {
            ++n;
        }
        assert_eq(n, 5);
        assert(1 == 1);

        // Test always-false condition.
        n = 5;
        while (0)
            n = 8;
        assert_eq(n, 5);
        do
            n = 9;
        while (0);
        assert_eq(n, 9);

        // Test always-true condition.
        byte counter = 0;
        while (1)
            if (++counter == 10)
                break;
        assert_eq(counter, 10);

        do
            if (--counter == 3)
                break;
        while (1);
        assert_eq(counter, 3);
        return 0;
    }
    `,
expected => ""
},


{
title => q{For loop},
program => q`
    void infiniteLoop()
    {
        for (;;)
            ;
        for (;1;)
            ;
    }
    int main()
    {
        word n = 1;
        for (byte i = 5; i--; )
            n *= 2;
        assert_eq(n, 32);

        n = 1;
        for (word j = 5; j--; )
            n *= 2;
        assert_eq(n, 32);

        n = 1;
        word j = 5;
        for ( ; j--; )
            n *= 2;
        assert_eq(n, 32);

        n = 1;
        j = 5;
        for ( ; ; j--)
        {
            if (j == 0)
                break;
            n *= 2;
        }
        assert_eq(n, 32);

        // Control variable wraps around.
        n = 0;
        for (byte x = 0; x < 256; ++x)
        {
            ++n;
            if (n == 270)
                break;
        }
        assert_eq(n, 270);
        return 0;
    }
    `,
expected => ""
},


{
title => q{Octal and hex codes in a string or integer literal},
program => q`
    void checkStr(const char *str, char expectedValueFirstChar)
    {
        char actualFirstChar = str[0];
        if (actualFirstChar != expectedValueFirstChar)
        {
            printf("ERROR: checkStr(): expected char %u, got char %u\n",
                   (byte) expectedValueFirstChar, (byte) actualFirstChar);
        }
        if (str[1])
        {
            printf("ERROR: checkStr(): expected single char string (char %u), got more: %u and %u\n",
                   (byte) expectedValueFirstChar, (byte) str[0], (byte) str[1]);
        }
    }
    void checkChar(char actualFirstChar, char expectedValueFirstChar)
    {
        if (actualFirstChar != expectedValueFirstChar)
        {
            printf("ERROR: checkChar(): expected char %u, got char %u\n",
                   (byte) expectedValueFirstChar, (byte) actualFirstChar);
        }
    }
    int main()
    {
        assert_eq(052, 42);
        assert_eq(0377, 255);
        assert_eq(03464, 1844);
        assert_eq(0177777, 0xFFFF);

        assert(!strcmp("\0101", "A"));
        assert(!strcmp("\x42", "B"));
        assert(!strcmp("\0101\x42\01018\x42z\n", "ABA8Bz\x0A"));
        checkStr("\a", 7);
        checkStr("\b", 8);
        checkStr("\t", 9);
        checkStr("\n", 10);
        checkStr("\v", 11);
        checkStr("\f", 12);
        checkStr("\r", 13);
        checkStr("\'", 39);
        checkStr("\"", 34);
        checkStr("\\\\", 92);  // 4 backslashes because of Perl interpretation
        checkStr("\x43", 64 + 3);
        checkStr("\0", 0);
        checkStr("\0377", (char) (3 * 8 * 8 + 7 * 8 + 7));
        checkChar('\a', 7);
        checkChar('\b', 8);
        checkChar('\t', 9);
        checkChar('\n', 10);
        checkChar('\v', 11);
        checkChar('\f', 12);
        checkChar('\r', 13);
        checkChar('\'', 39);
        checkChar('\"', 34);
        checkChar('\\\\', 92);  // 4 backslashes because of Perl interpretation
        checkChar('\x43', 64 + 3);
        checkChar('\0', 0);
        checkChar('\0377', (char) (3 * 8 * 8 + 7 * 8 + 7));
        checkChar('\377', (char) (3 * 8 * 8 + 7 * 8 + 7));
        checkChar('\1', 1);
        checkChar('\12', 10);
        checkChar('\123', 64 + 16 + 3);
        const char *s = "a\\\\b";  // 4 backslashes because of Perl interpretation
        assert(strlen(s) == 3);
        assert(s[0] == 97);
        assert(s[1] == 92);
        assert(s[2] == 98);
        
        // Check for an a09 bug where a 0x in a string literal gets replaced with $. 
        const char *h = "0xEF";
        assert_eq(strlen(h), 4);
        assert_eq(h[0], '0');
        assert_eq(h[1], 'x');
        assert_eq(h[2], 'E');
        assert_eq(h[3], 'F');
        const char *H = "0XEF";
        assert_eq(strlen(H), 4);
        assert_eq(H[0], '0');
        assert_eq(H[1], 'X');
        assert_eq(H[2], 'E');
        assert_eq(H[3], 'F');
        return 0;
    }
    `,
expected => ""
},


{
title => q{Bitwise operators},
program => q`
    int main()
    {
        {
            byte n = 0x43;
            byte m = 0xC9;
            assert_eq(n | m, 0xCB);
            assert_eq(n ^ m, 0x8A);
            assert_eq(n & m, 0x41);
            assert_eq(n & 3, 0x03);
        }
        {
            word n = 0x4343;
            word m = 0xC9C9;
            assert_eq(n | m, 0xCBCB);
            assert_eq(n ^ m, 0x8A8A);
            assert_eq(n & m, 0x4141);
            assert_eq(m & 0x0F71, 0x0941);
            word k = 0xABCD;
            assert_eq(((byte) k) & 0xF0, 0xC0);
        }
        return 0;
    }
    `,
expected => ""
},


{
title => q{Array initializers},
program => q`
    unsigned char b[4] = { 0x80, 0x40, 0x20, 0x10, };  // trailing comma accepted
    word w[4] = { 0xaa80, 0xbb40, 0xcc20, 0xdd10 };
    char c[3] = { 'b', 'a', 'z' };
    char cc[] = { 'B', 'A', 'Z' };  // size of array taken from size of init list
    char d[7];
    char e[4] = "baz";
    char buffer[] = "HELLO";
    int sc[] = { (char) 255 };

    void local()
    {
        unsigned char b_[4] = { 0x80, 0x40, 0x20, 0x10 };
        word w_[4] = { 0xaa80, 0xbb40, 0xcc20, 0xdd10 };
        char c_[3] = { 'b', 'a', 'z' };
        char cc_[] = { 'B', 'A', 'Z' };
        char e_[4] = "BAZ";
        assert_eq(b_[0], 0x80);
        assert_eq(b_[1], 0x40);
        assert_eq(b_[2], 0x20);
        assert_eq(b_[3], 0x10);
        assert_eq(w_[0], 0xAA80);
        assert_eq(w_[1], 0xBB40);
        assert_eq(w_[2], 0xCC20);
        assert_eq(w_[3], 0xDD10);
        assert_eq(c_[0], 'b');
        assert_eq(c_[1], 'a');
        assert_eq(c_[2], 'z');
        assert_eq(cc_[0], 'B');
        assert_eq(cc_[1], 'A');
        assert_eq(cc_[2], 'Z');
        assert_eq(strcmp(e_, "BAZ"), 0);
        assert_eq(e_[2], 'Z');
        assert_eq(e_[3], 0);
        
        char buffer_[] = "PIZZA";
        assert_eq(strcmp(buffer_, "PIZZA"), 0);
        
        int sc_[] = { (char) 255 };
        assert_eq(sc_[0], -1);
    }
    int main()
    {
        assert_eq(b[0], 0x80);
        assert_eq(b[1], 0x40);
        assert_eq(b[2], 0x20);
        assert_eq(b[3], 0x10);
        assert_eq(w[0], 0xAA80);
        assert_eq(w[1], 0xBB40);
        assert_eq(w[2], 0xCC20);
        assert_eq(w[3], 0xDD10);
        assert_eq(c[0], 'b');
        assert_eq(c[1], 'a');
        assert_eq(c[2], 'z');
        assert_eq(cc[0], 'B');
        assert_eq(cc[1], 'A');
        assert_eq(cc[2], 'Z');
        d[0] = 42;
        d[6] = 71;
        assert_eq(d[0], 42);
        assert_eq(d[6], 71);
        assert_eq(strcmp(e, "baz"), 0);
        assert_eq(e[2], 'z');
        assert_eq(e[3], 0);
        
        char buffer[] = "HELLO";
        assert_eq(strcmp(buffer, "HELLO"), 0);

        assert_eq(sc[0], -1);
        
        local();
        return 0;
    }
    `,
expected => ""
},


{
title => q{Array arithmetic},
program => q`
    byte byteArray[5];
    word wordArray[5];
    struct S { byte foo[40]; word bar; };
    struct S s;
    struct S sArray[5];
    int main()
    {
        byte *pb = byteArray + 3;
        word byteArrayDiff = pb - byteArray;
        assert_eq(byteArrayDiff, 3);

        word *pw = wordArray + 3;
        word wordArrayDiff = pw - wordArray;
        assert_eq(wordArrayDiff, 3);
        byte *pwb = (byte *) pw;
        byte *wab = (byte *) wordArray;
        assert_eq(pwb - wab, 6);

        s.bar = 42;
        assert(s.bar == 42);

        struct S *ps = sArray + 3;
        assert((word) ps - (word) sArray == 3 * 42);
        word sArrayDiff = ps - sArray;
        //printf("sArrayDiff=%u\n", sArrayDiff);
        assert_eq(sArrayDiff, 3);
        byte *psb = (byte *) ps;
        byte *sab = (byte *) sArray;
        //printf("byte diff: %u\n", psb - sab);
        assert_eq(psb - sab, 3 * 42);

        ps = &s + 1;
        //printf("%u, %u\n", &s, ps);
        assert_eq((word) ps - (word) &s, 42);
        ps = &s + 3;
        assert_eq((word) ps - (word) &s, 3 * 42);

        struct S *ps2 = &sArray[2];
        struct S *ps4 = &sArray[4];
        word byteDiff = (byte *) ps4 - (byte *) ps2;
        assert_eq(byteDiff, 2 * 42);

        word *bar2 = &ps2->bar;
        word *bar4 = &ps4->bar;
        byteDiff = (byte *) bar4 - (byte *) bar2;
        assert_eq(byteDiff, 2 * 42);

        *byteArray = 254;
        assert_eq(byteArray[0], 254);
        *wordArray = 0x4141;
        assert_eq(wordArray[0], 0x4141);

        // Check cases for which there are specialized optimizations.

        word wa[3] = { 0x1122, 0x3344, 0x4455 };
        assert_eq(wa[2], 0x4455);  // and not 0x3344
        word wi = 2;
        assert_eq(wa[wi], 0x4455);
        byte bi = 2;
        assert_eq(wa[bi], 0x4455);
        
        word *wp = &wa[2];
        assert_eq(wp[-1], 0x3344);
        signed int swi = -1;
        assert_eq(wp[swi], 0x3344);
        signed char sci = -2;
        assert_eq(wp[sci], 0x1122);
        
        return 0;
    }
    `,
expected => ""
},


{
title => q{Bitwise NOT operator},
program => q`
    int main()
    {
        byte b = 0x55;
        word w = 0xaaaa;
        assert_eq(~b, 0xAA);
        assert_eq(~w, 0x5555);
        return 0;
    }
    `,
expected => ""
},


{
title => q{Additive assignment, etc},
program => q`
    int main()
    {
        // +=, -=

        byte *p = 0;
        byte inc = 4;
        byte dummy = 0x55;  // this byte allocated after 'inc'
        p += inc;
        assert_eq(p, 4);
        p -= inc;
        assert_eq(p, 0);

        byte b = 0;
        word w = 0xabcd;
        b += (byte) w;
        assert_eq(b, 0xCD);

        // Signed cases.        
        char c = 100;
        c += 1;
        assert_eq(c, 101);
        c += -1;
        assert_eq(c, 100);
        c -= 1;
        assert_eq(c, 99);
        c -= -1;
        assert_eq(c, 100);
        
        int i = 1000;
        i += 1;
        assert_eq(i, 1001);
        i += -1;
        assert_eq(i, 1000);
        i -= 1; 
        assert_eq(i, 999);
        i -= -1; 
        assert_eq(i, 1000);
        i += 100;
        assert_eq(i, 1100);
        i += -100;
        assert_eq(i, 1000);
        i -= 100; 
        assert_eq(i, 900);
        i -= -100; 
        assert_eq(i, 1000);

        // *=

        word k = 0x3000;
        k *= inc;
        assert_eq(k, 0xC000);

        byte m = 0x05;
        word n = 0x1111;
        m *= (byte) n;
        assert_eq(m, 0x55);

        // /=

        assert_eq(inc, 4);
        word z = 0x444;
        z /= inc;
        assert_eq(z, 0x111);

        byte q = 0xcc;
        word v = 0x2203;
        q /= (byte) v;
        assert_eq(q, 0x44);

        // ^=

        b = 0x5F;
        b ^= 1;
        w = 0x543F;
        w ^= 1;
        assert(b == 0x5E);
        assert(w == 0x543E);
        b ^= 1;
        w ^= 1;
        assert(b == 0x5F);
        assert(w == 0x543F);
        b ^= 0x81;
        w ^= 0x801;
        assert(b == 0xDE);
        assert(w == 0x5C3E);
        w ^= (byte) 0xff01;
        assert(w == 0x5C3F);

        // &=

        b = 0x5F;
        b &= 0xFE;
        w = 0x543F;
        w &= 0xFFFE;
        assert(b == 0x5E);
        assert(w == 0x543E);
        b &= 0x0F;
        w &= 0x4F0;
        assert(b == 0x0E);
        assert(w == 0x0430);
        b &= 0x0C;
        w &= 0x0220;
        assert(b == 0x0C);
        assert(w == 0x0020);
        w &= (byte) 0xFF80;
        assert(w == 0x0000);
        
        * (byte *) 0x0400 = 0xfc;
        assert_eq(* (byte *) 0x0400, 0xfc);
        * (byte *) (0x0400 + 0x0032) = 0xfc;
        assert_eq(* (byte *) 0x0432, 0xfc);
        * (byte *) 0x0400 &= 7; 
        assert_eq(* (byte *) 0x0400, 4);        

        // |=

        b = 0x08;
        b |= 0x28;
        w = 0x0100;
        w |= 0x7155;
        assert(b == 0x28);
        assert(w == 0x7155);
        b |= 0x80;
        w |= 0x8080;
        assert(b == 0xa8);
        assert(w == 0xf1d5);
        b |= 0x0C;
        w |= 0x4220;
        assert(b == 0xac);
        assert(w == 0xf3f5);
        w |= (byte) 0xFF02;
        assert(w == 0xf3f7);

        // %=
        b = 123;
        b %= 10;
        assert(b == 3);
        w = 1234;
        w %= 500;
        assert(w == 234);
        w = 1234;
        w %= 25;
        assert(w == 9);
        return 0;
    }
    `,
expected => ""
},


{
title => q{Verbatim assembly language},
program => q`
    void f(int *p)
    {
        asm {
            ldd #1844
            std [p]     // brackets are words by themselves
        }
    }
    enum { FOO = 42 };
    int main()
    {
        word w = 0x1000;
        byte x = 40;
        asm("ldd", w);
        asm {
            addd #$234  // comment must start with "//"
            exg a,b     /* or use C-style comments */
            nop         ; or start with a semi-colon
            bra @localLabel
; Next line tests that a colon in a semi-colon comment is not processed as a variableNameEscapeChar.
; foo bar:
; Next label tests that semi-colon comment lines are sent to the assembler.
; @localLabel is local in LWASM. Semi-colon comment lines preserve the "local scope". 
@localLabel
            inca
            incb
            pshs x      // register X
            inc  :x     // C variable 'x' ("inc x" would refer to register, i.e., error) 
            puls x      // register X
        }
        asm("std", w);
        assert_eq(w, 0x3513);
        assert_eq(x, 41);

        int n = 0;
        f(&n);
        assert_eq(n, 1844);
        
        byte m[2] = { 22, 33 };
        byte a[2] = { 44, 55 };
        int vi[16];
        asm {
            ldb     m
            addb    m[1]
            stb     m[0]
            ldb     :a
            addb    :a[1]
            stb     :a[0]
            ldd     #$1234
            std     :vi[30]      ; offset in bytes, not ints
        }
        assert_eq(m[0], 22 + 33);
        assert_eq(a[0], 44 + 55);
        assert_eq(vi[15], 0x1234);
        
        byte foo = 42;
        asm {
            ldb #1+:FOO*2  // escaped enumerated name
            stb foo
        }
        assert_eq(foo, 85);
        
        word d;
        asm {
            leax d,x  // 'd' must refer to register D, not C variable 'd'
        }

        return 0;
    }
    `,
expected => ""
},


{
title => q{Referring to variables from asm directive},
program => q`
    int main()
    {
        word big = 0x1234;
        byte small = 0xaa;
        asm {
            ldd big
            coma
            comb
            std big
            ldb small
            comb
            stb small
loop:                   // no white space allowed before a label
            brn loop
        }
        assert_eq(big, 0xEDCB);
        assert_eq(small, 0x55);
        return 0;
    }
    `,
expected => ""
},


{
title => q{Declaring two local variables with same name, in different blocks},
program => q`
    int main()
    {
        byte *addr1;
        byte *addr2;
        {
            byte b = 11;
            assert_eq(b, 11);
            addr1 = &b;
        }
        {
            byte b = 22;
            assert_eq(b, 22);
            addr2 = &b;
        }
        assert_eq(addr1, addr2);  // both b's expected to be at same address
        return 0;
    }
    `,
expected => ""
},


{
title => q{Conditional expression (cond ? foo : bar)},
tolerateWarnings => 1,
program => q`
    void foo(byte p) {}
    byte returnByte(byte b)
    {
        asm {
            lda #$DD    // put dirt in A
        }
        return b;
    }
    int main()
    {
        assert((1 ? 2 : 3) == 2);
        assert((0 ? 2 : 3) == 3);
        foo(1 ? (byte) 2 : (byte) 3);

        {
            word cond = 1;
            word t = 2;
            word f = 3;
            assert((cond ? t : f) == 2);
            cond = 0;
            assert((cond ? t : f) == 3);
        }

        {
            byte cond = 1;
            byte t = 2;
            byte f = 3;
            assert((cond ? t : f) == 2);
            cond = 0;
            assert((cond ? t : f) == 3);
            t = (cond ? 4 : 5);  // 4 and 5 are ints, but they fit in byte, so no error
        }

        {
            asm { ldx #0 }  // robustness check
            word a = 4;
            word b = 5;
            word c = 1;
            (c ? a : b) = 6;
            assert_eq(a, 6);
            assert(b == 5);
            c = 0;
            (c ? a : b) = 7;
            assert(a == 6);
            assert(b == 7);
        }

        {
            byte a = 4;
            byte b = 5;
            byte c = 1;
            (c ? a : b) = 6;
            assert(a == 6);
            assert(b == 5);
            c = 0;
            (c ? a : b) = 7;
            assert(a == 6);
            assert(b == 7);
        }

        {
            byte *a = (byte *) 4;
            byte *b = (byte *) 5;
            byte *c = (byte *) 1;
            (c ? a : b) = 6;
            assert(a == 6);
            assert(b == 5);
            c = 0;
            (c ? a : b) = 7;
            assert(a == 6);
            assert(b == 7);
        }
        
        // True and false expressions of different sizes.
        {
            unsigned u = 100;
            u = (u > 99 ? 50 : u);  // warning
            assert_eq(u, 50);
            byte b = 12;
            u = (u > 66 ? u : returnByte(b));  // warning
            assert_eq(u, 12);
            u = 999;
            u = (u < 2000 ? u : returnByte(b));  // warning
            assert_eq(u, 999);
            
            int a0[] = { 10 }, a1[] = { 11 };
            b = 50;
            int *p = (b < 77 ? a0 : a1);
            assert_eq(p, &a0);
        }

        // Cond. expr. used in signed byte initializer.
        {
             int sy = (2 < 1) ? 1 : -1;
             assert_eq(sy, -1);
        }
        
        return 0;
    }
    `,
expected => ""
},


{
title => q{Calling a with same name as local variable declared after the call},
program => q`
    byte b = 0;
    void func() { b = 77; }
    int main()
    {
        assert_eq(b, 0);
        func();
        assert_eq(b, 77);
        word func = 0;
        assert_eq(b, 77);
        return 0;
    }
    `,
expected => ""
},


{
title => q{Optimized increments and decrements},
program => q`
    int main()
    {
        {
            char buf[8];
            memcpy(buf, "xxxxxxxx", 8);
            char *dest = buf;
            unsigned char i = 0;
            while (i < 8)
            {
                *dest++ = 'x';  // detect unsafe optimizations on this post-inc.
                ++i;
            }
            assert_eq(memcmp(buf, "xxxxxxxx", 8), 0);
        }
    
        {
            char b = 10;
            b += 0;
            assert_eq(b, 10);
            b += 1;
            assert_eq(b, 11);
            b += 2;
            assert_eq(b, 13);
            b += -1;
            assert_eq(b, 12);
            b += -2;
            assert_eq(b, 10);
            asm("CLRB");  // to prove that += 0 still loads variable in register, for init of b1
            char b1 = (b += 0);
            assert_eq(b1, 10);
            char b2 = (b += 4);
            assert_eq(b2, 14);
        }
        
        {
            int w = 10;
            w += 0;
            assert_eq(w, 10);
            w += 1;
            assert_eq(w, 11);
            w += 2;
            assert_eq(w, 13);
            w += -1;
            assert_eq(w, 12);
            w += -2;
            assert_eq(w, 10);
            asm("CLRA");  // to prove that += 0 still loads variable in register, for init of w1
            asm("CLRB");
            int w1 = (w += 0);
            assert_eq(w1, 10);
            int w2 = (w += 4);
            assert_eq(w2, 14);
        }
        
        return 0;
    }
    `,
expected => ""
},


{
title => q{Increment on word pointer},
program => q`
    int main()
    {
        word *wp = (word *) 100;
        wp += 16;
        assert_eq(wp, 132);
        wp = 100;
        wp -= 16;
        assert_eq(wp, 68);

        wp = 1000;
        wp += 0x80;  // 1000 + 128 * 2 == 1256
        assert_eq(wp, 1256);
        wp = 2256;
        wp -= 0x80;  // 2256 - 128 * 2 == 2000
        assert_eq(wp, 2000);

        byte *bp = (byte *) 100;
        bp += 16;
        assert_eq(bp, 116);
        bp = 100;
        bp -= 16;
        assert_eq(bp, 84);

        word w = 200;
        w += 16;
        assert_eq(w, 216);
        w = 200;
        w -= 16;
        assert_eq(w, 184);

        byte b = 200;
        b += 16;
        assert_eq(b, 216);
        b = 200;
        b -= 16;
        assert_eq(b, 184);
        return 0;
    }
    `,
expected => ""
},


{
title => q{Increment on struct pointer},
program => q`
    struct S { word a; word b; byte c; };
    void f(byte *start, byte *end)
    {
        assert_eq(end, start + 5 * sizeof(struct S));
        struct S *last = (struct S *) end - 1;
        assert_eq(last, start + 4 * sizeof(struct S));
        struct S *p = last;
        --p;
        assert_eq(p, start + 3 * sizeof(struct S));
        p--;
        assert_eq(p, start + 2 * sizeof(struct S));
        p = p - 1;
        assert_eq(p, start + sizeof(struct S));
        ++p;
        assert_eq(p, start + 2 * sizeof(struct S));
        p++;
        assert_eq(p, start + 3 * sizeof(struct S));
        p = p + 1;
        assert_eq(last, start + 4 * sizeof(struct S));
    }
    int main()
    {
        assert_eq(sizeof(struct S), 5);
        struct S a[5];
        struct S *p = a;
        assert_eq(p, a);
        assert_eq(p + 1, &a[1]);
        ++p;
        assert_eq(p, &a[1]);
        assert_eq(p - 1, &a[0]);
        byte *aCast = (byte *) a;
        byte *pCast = (byte *) p;
        assert_eq(pCast, aCast + sizeof(struct S));
        
        // For loop increment may be subject to optimization. 
        for (p = a; p < &a[5]; ++p)
            assert_eq((p - a) * sizeof(struct S), (byte *) p - (byte *) a);

        f((byte *) a, (byte *) (a + 5));

        return 0;
    }
    `,
expected => ""
},


{
title => q{exit()},
program => q`
    void h() { exit(0); }
    void g() { h(); }
    void f() { g(); }
    int main()
    {
        f();
        printf("end\n");
        return 0;
    }
    `,
expected => ""
},


{
title => q{if/while condition ending in == 0 or != 0. String literal sequence},
program => q`
    #define BUFSIZ 200
    char buffer[BUFSIZ];
    word index = 0;
    void write(const char *str)
    {
        word len = strlen(str);
        assert(index + len < BUFSIZ);
        strcpy(buffer + index, str);
        index += len;
    }
    byte b0() { return 0; }
    byte b1() { return 1; }
    byte w0() { return 0; }
    byte w1() { return 1; }
    int main()
    {
        if (b0() == 0)
            write("b0-eq-then\n");
        else
            write("b0-eq-else\n");
        if (b1() == 0)
            write("b1-eq-then\n");
        else
            write("b1-eq-else\n");
        if (w0() == 0)
            write("w0-eq-then\n");
        else
            write("w0-eq-else\n");
        if (w1() == 0)
            write("w1-eq-then\n");
        else
            write("w1-eq-else\n");

        if (b0() != 0)
            write("b0-ne-then\n");
        else
            write("b0-ne-else\n");
        if (b1() != 0)
            write("b1-ne-then\n");
        else
            write("b1-ne-else\n");
        if (w0() != 0)
            write("w0-ne-then\n");
        else
            write("w0-ne-else\n");
        if (w1() != 0)
            write("w1-ne-then\n");
        else
            write("w1-ne-else\n");

        while (b0() == 0)
        {
            write("b0-eq-body\n");
            break;
        }
        while (b1() == 0)
        {
            write("b1-eq-body\n");
            break;
        }
        while (w0() == 0)
        {
            write("w0-eq-body\n");
            break;
        }
        while (w1() == 0)
        {
            write("w1-eq-body\n");
            break;
        }

        while (b0() != 0)
        {
            write("b0-eq-body\n");
            break;
        }
        while (b1() != 0)
        {
            write("b1-eq-body\n");
            break;
        }
        while (w0() != 0)
        {
            write("w0-eq-body\n");
            break;
        }
        while (w1() != 0)
        {
            write("w1-eq-body\n");
            break;
        }

        buffer[index] = 0;

        // Test using a string literal sequence.
        assert(!strcmp(buffer, "b0-eq-then\nb1-eq-else\nw0-eq-then\nw1-eq-else\n"
                               "b0-ne-else\nb1-ne-then\nw0-ne-else\nw1-ne-then\n"
                               "b0-eq-body\nw0-eq-body\n"
                               "b1-eq-body\nw1-eq-body\n"));
        return 0;
    }
    `,
expected => ""
},

{
title => q{Multiple declarators per var_decl},
program => q`
    int main()
    {
        byte b0, b1;
        byte b2 = 2, b3;
        byte b4, b5 = 5;
        byte b6 = 6, b7 = 7;
        byte b8 = 8, b9 = 9, b10 = 10, b11 = 11, b12 = 12;
        assert_eq(b2, 2);
        assert_eq(b5, 5);
        assert_eq(b6, 6);
        assert_eq(b7, 7);
        assert_eq(b8, 8);
        assert_eq(b9, 9);
        assert_eq(b10, 10);
        assert_eq(b11, 11);
        assert_eq(b12, 12);

        word w0, w1;
        word w2 = 2, w3;
        word w4, w5 = 5;
        word w6 = 6, w7 = 7;
        word w8 = 8, w9 = 9, w10 = 10, w11 = 11, w12 = 12;
        assert_eq(w2, 2);
        assert_eq(w5, 5);
        assert_eq(w6, 6);
        assert_eq(w7, 7);
        assert_eq(w8, 8);
        assert_eq(w9, 9);
        assert_eq(w10, 10);
        assert_eq(w11, 11);
        assert_eq(w12, 12);

        byte b = 1, *pb = &b, bb = 99;
        assert_eq(b, 1);
        assert_eq(bb, 99);
        assert(pb);
        assert_eq(*pb, 1);
        *pb = 42;
        assert_eq(*pb, 42);
        assert_eq(b, 42);
        pb = &bb;
        --*pb;
        assert_eq(bb, 98);

        word w = 1, *pw = &w, ww = 999;
        assert_eq(w, 1);
        assert_eq(ww, 999);
        assert(pw);
        assert_eq(*pw, 1);
        *pw = 42;
        assert_eq(*pw, 42);
        assert_eq(w, 42);
        pw = &ww;
        --*pw;
        assert_eq(ww, 998);
        return 0;
    }
    `,
expected => ""
},


{
title => q{Interrupt handler},
program => q`
    word global = 0;
    interrupt void isr()
    {
        global = 42;
    }
    int main()
    {
        assert_eq(global, 0);
        
        // Simulate an IRQ.
        interrupt void (*isrAddr)(void) = isr;  // arg list "(void)" is equivalent to "()"
        byte *pushedX = 0, *restoredX = 0;
        asm {
            leax returnAddr,pcr
            stx pushedX
            pshs x  // RTI will pop this into PC
            orcc #$80  // set E flag (whole environment saved)
            pshs u,y,x,dp,b,a,cc  // save rest of environment
            ldx isrAddr
            jmp ,x
returnAddr:
            stx restoredX
        }
        assert_eq(global, 42);
        assert_eq(restoredX, pushedX);  // X must have been restored by RTI
        return 0;
    }
    `,
expected => ""
},


{
title => q{Global variables},
program => q`
    byte bytes[4] = { 11, 22, 33, 44 };
    word words[3];
    byte gb0 = 42;
    byte gb1;
    word gw0 = 7777;
    word gw1;

    word a = 4000;
    word x = 2000;
    word wa[] = { 42, 11000, 17 };
    byte ba[] = { (byte) 0xd123 }; 

    int main()
    {
        gw1 = 2424;
        words[2] = gw0;
        gb1 = bytes[3];
        gb0++;

        assert_eq(gw1, 2424);
        assert_eq(words[2], 7777);
        assert_eq(gb1, 44);
        assert_eq(gb0, 43);
        assert((void *) bytes != (void *) words);
        assert(gb0 != gb1);
        assert(gw0 != gw1);
        assert(gb0 != gw1);
        assert(gb1 != gw0);
        
        assert_eq(a, 4000);
        assert_eq(x, 2000);
        assert_eq(wa[0], 42);
        assert_eq(wa[1], 11000);
        assert_eq(wa[2], 17);
        assert_eq(ba[0], 0x23);
        
        return 0;
    }
    `,
expected => ""
},


{
title => q{sizeof(type)},
program => q`
    struct S1
    {
        char b[5];
    };
    struct S0
    {
        char b;
        unsigned *pw;  // no 2-byte alignment
        char byteArray[9 + 1];  // non-trivial size expression tree (cf DeclaratorInfo::computeArraySize())
        struct S1 s1;
        struct S0 *next;
    };
    struct Empty
    {
    };
    int main()
    {
        assert_eq(sizeof(void), 1);  // as with GCC
        assert_eq(sizeof(char), 1);
        assert_eq(sizeof(word), 2);
        assert_eq(sizeof(char *), 2);
        assert_eq(sizeof(int *), 2);

        assert_eq(sizeof(unsigned char), 1);
        assert_eq(sizeof(signed char), 1);
        assert_eq(sizeof(unsigned), 2);
        assert_eq(sizeof(signed), 2);
        assert_eq(sizeof(unsigned short), 2);
        assert_eq(sizeof(signed short), 2);
        assert_eq(sizeof(unsigned int), 2);
        assert_eq(sizeof(signed int), 2);
        assert_eq(sizeof(unsigned char *), 2);
        assert_eq(sizeof(signed char *), 2);
        assert_eq(sizeof(unsigned int *), 2);
        assert_eq(sizeof(signed int *), 2);

        assert_eq(sizeof(struct S1), 5);
        assert_eq(sizeof(struct S0), 1 + 2 + 10 + 5 + 2);
        assert_eq(sizeof(struct S0 *), 2);
        assert_eq(sizeof(struct Empty), 0);

        char b[8 + 2];
        assert_eq(sizeof(b), 10);
        return 0;
    }
    `,
expected => ""
},


{
title => q{sizeof(expr)},
program => q`
    struct S1
    {
        byte b[5];
    };
    struct S0
    {
        byte b;
        word *pw;  // no 2-byte alignment
        byte byteArray[10];
        struct S1 s1;
        struct S0 *next;
    };
    struct Empty
    {
    };
    int main()
    {
        byte b;
        assert_eq(sizeof(b), 1);
        word w;
        assert_eq(sizeof(w), 2);
        byte *pb;
        assert_eq(sizeof(pb), 2);
        word *pw;
        assert_eq(sizeof(pw), 2);
        struct S1 s1;
        assert_eq(sizeof(s1), 5);
        struct S0 s0;
        assert_eq(sizeof(s0), 1 + 2 + 10 + 5 + 2);
        assert_eq(sizeof(&s0), 2);
        struct Empty empty;
        assert_eq(sizeof(empty), 0);

        byte ab[5];
        assert_eq(sizeof(ab[0]), 1);
        assert_eq(sizeof(ab), 5);

        word aw[5];
        assert_eq(sizeof(aw[0]), 2);
        assert_eq(sizeof(aw), 5 * 2);
        assert_eq(sizeof(aw) / sizeof(aw[0]), 5);  // the division should be optimized out

        word twoDim[5][3];
        assert_eq(sizeof(twoDim), 30);
        assert_eq(sizeof(twoDim[0]), 6);
        assert_eq(sizeof(twoDim) / sizeof(twoDim[0]), 5);
        assert_eq(sizeof(twoDim[0][0]), 2);
        assert_eq(sizeof(twoDim[18][44]), 2);  // indices don't matter

        word threeDim[5][3][4];
        assert_eq(sizeof(threeDim), 120);
        assert_eq(sizeof(threeDim[0]), 24);
        assert_eq(sizeof(threeDim) / sizeof(threeDim[0]), 5);
        assert_eq(sizeof(threeDim[0][0]), 8);
        assert_eq(sizeof(threeDim[0][0][0]), 2);

        struct S1 as1[5];
        assert_eq(sizeof(as1), 5 * sizeof(struct S1));
        struct S0 as0[5];
        assert_eq(sizeof(as0), 5 * sizeof(struct S0));
        
        word arrayWithNoSize[] = { 1, 2, 3, 4, 5 };
        assert_eq(sizeof(arrayWithNoSize), 10);

        assert_eq(sizeof(""), 1);
        assert_eq(sizeof("foobar"), 7);
        assert_eq(sizeof("\0\0\0"), 4);
        char s[] = "quux";
        assert_eq(sizeof(s), 5);

        return 0;
    }
    `,
expected => ""
},


{
title => q{Empty statement},
program => q`
    void f()
    {
        ;
    }
    int main()
    {
        return 0;
    }
    `,
expected => ""
},


{
title => q{Variadic function. va_list is a typedef for "word *"},
program => q`
    #include <stdarg.h>

    struct Object
    {
        byte b;
    };
    void variadic(const char *firstFixed, const char *lastFixed, ...)
    {
        va_list ap;
        assert_eq(sizeof(ap), 2);  // va_list is a pointer

        va_start(ap, lastFixed);
        va_list orig = ap;

        word w = va_arg(ap, word);
        assert_eq(w, 17);
        assert_eq(ap, orig + 2);  // ap must be 2 bytes from start; va_list is a char *

        byte b = va_arg(ap, byte);
        assert_eq(b, 42);
        assert_eq(ap, orig + 4);

        char *s = va_arg(ap, char *);
        assert(!strcmp(s, "foobar"));
        assert_eq(ap, orig + 6);

        struct Object *obj = va_arg(ap, struct Object *);
        assert(obj);
        assert_eq(obj->b, 99);
        assert_eq(ap, orig + 8);

        va_end(ap);

        assert(!strcmp(firstFixed, "fixed1"));
        assert(!strcmp(lastFixed, "fixed2"));
        assert_eq(w, 17);
        assert_eq(b, 42);
        assert(!strcmp(s, "foobar"));
        assert_eq(obj->b, 99);
    }
    int main()
    {
        struct Object obj;
        obj.b = 99;
        variadic("fixed1", "fixed2", 17, 1024 + 42, "foobar", &obj);
        return 0;
    }
    `,
expected => ""
},


{
title => q{rand(): Check that counters are filled quasi-uniformly},
program => q`
    enum { BUCKETS = 16,  // must be power of 2
           ITERATIONS = 6400 };
    int main()
    {
        word counters[BUCKETS];
        memset(counters, 0, BUCKETS * sizeof(word));

        {
            int n = rand();
            assert_ne(n, 0);
            srand(42);
            int m = rand();
            assert_ne(m, n);  // proves that SEED was affected by srand()
    
            int mask = BUCKETS - 1;
            for (word i = 0; i < ITERATIONS; ++i)
                ++counters[rand() & mask];
            word expectedAverage = ITERATIONS / BUCKETS;
            word low  = expectedAverage *  93 / 100, veryLow  = expectedAverage *  86 / 100;
            word high = expectedAverage * 110 / 100, veryHigh = expectedAverage * 123 / 100;
            //printf("expectedAverage=%u. Limits: small=[%u, %u], large=[%u, %u]\n",
            //        expectedAverage, low, high, veryLow, veryHigh);
            word numLargeDiffs = 0;
            for (word b = 0; b < BUCKETS; ++b)
            {
                word c = counters[b];
                //printf("Bucket %2u: %5u\n", b, c);
                if (c < veryLow || c > veryHigh)
                {
                    printf("Bucket %u has %u, which is out of expected range [%u, %u]\n",
                            b, c, veryLow, veryHigh);
                    assert(!"Unexpected value in bucket");
                }
                else if (c < low || c > high)
                {
                    printf("Large diff: bucket %u, count %u\n", b, c);
                    ++numLargeDiffs;
                }
            }
            //printf("numLargeDiffs=%u\n", numLargeDiffs);
            assert(numLargeDiffs <= 0);
        }
        
        // Check that the 15-bit values are equally distributed on 0..$3FFF and $4000..$7FFF
        // across a relatively short sample.
        {
            word N = 50, numLargeDiffs = 0, numTests = 0;
            for (unsigned seed = 0; seed < 60000; seed += 3000)
            {
                srand(seed);
                word zeroCounter;
                for (unsigned mask = 0x4000; mask; mask >>= 1)  // for each of the 15 bits to test
                {
                    // Draw N numbers. Check the bit at 'mask'. Expect about N/2 zeroes, N/2 ones. 
                    zeroCounter = 0;  // number of results where bit is 0
                    for (word i = 0; i < N; ++i)
                    {
                        int k = rand();
                        //printf("%d ", k & mask);
                        if ((k & mask) == 0)
                            ++zeroCounter;
                    }
                    //printf("\n");
                    char isLargeDiff = (zeroCounter < 21 || zeroCounter > 29);
                    //printf("seed=%5u, mask=$%04x: %5u %d\n", seed, mask, zeroCounter, isLargeDiff);
                    if (isLargeDiff)
                        ++numLargeDiffs;
                    if (zeroCounter < 15 || zeroCounter > 38)  // max allowed deviation
                    {
                        printf("Mask: $%04x. Counter: %u\n", mask, zeroCounter);
                        assert(!"Disallowed deviation");
                    }
                    ++numTests;
                }
            }
            //printf("%u / %u\n", numLargeDiffs, numTests);
            assert(numLargeDiffs * 1000UL / 240 <= numTests);  // at most 24% of the tests show a "large" deviation
        }

        return 0;
    }
    `,
expected => ""
},


{
title => q{No predictable bit in the rand() return value},
linkerModeOnly => 1,
program => q`
    byte testRandBit(char bit, int seed)
    {
        word mask = 1 << bit;
        word previous = (word) rand();
        assert_eq(previous & 0x8000, 0);
        byte repetitions = 0;  // number of times the tested bit has been predicted
        srand(seed);
        for (byte i = 0; i < 255; ++i)
        {
            word current = (word) rand();
            assert_eq(current & 0x8000, 0);
            if ((current & mask) == ((previous & mask) ^ mask))  // if bit is inverse of previous bit
            {
                ++repetitions;
                if (repetitions >= 13)  // fail if the tested bit is too predictable
                {
                    printf("Tested bit: %u. Seed: %d. Repetitions: %u\n", bit, seed, repetitions);
                    assert(!"tested bit is too predictable");
                    return 0;
                }
            }
            else
                repetitions = 0;
            previous = current;
        }
        return 1;
    }
    int main()
    {
        for (int seed = 10000; seed > 0; seed /= 5)
        {
            char bit;
            for (bit = 15; bit >= 0; --bit)  // test each of the 15 bits returned by rand()
                if (!testRandBit(bit, seed))
                    break;
            if (bit >= 0)  // if failure
                break;
        }
        return 0;
    }
    `,
expected => ""
},


{
title => q{srand() affects rand()},
program => q`
    enum { N = 10 };
    int randResults[N];
    byte first = 1;
    void testRand(int seed)
    {
        //printf("testRand: seeding with %d.\nResults: ", seed);
        srand(seed);
        word numEqualResults = 0;
        for (byte i = 0; i < N; ++i)
        {
            int newResult = rand();
            //printf("%d ", newResult);
            if (!first && newResult == randResults[i])
                ++numEqualResults;
            randResults[i] = newResult;
        }
        //printf("\nnumEqualResults=%u\n", numEqualResults);
        assert_eq(numEqualResults, 0);
        first = 0;
    }
    int main()
    {
        memset(randResults, 0, sizeof(randResults));
        for (int seed = 1; seed <= 100; ++seed)
            testRand(seed);
        for (int seed = 1000; seed <= 32000; seed += 499)
            testRand(seed);
        return 0;
    }
    `,
expected => ""
},


{
title => q{Double negation},
program => q`
    void dummy()
    {
        // Force "foobar" string to have address >= 0x0100.
        const char *b0 = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
        const char *b1 = "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";
        const char *b2 = "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc";
        const char *b3 = "dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd";
        const char *b4 = "eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee";
    }
    int main()
    {
        char b = 0;
        if (!(!"foobar"))
            b = 1;
        else
            b = -1;
        assert_eq(b, 1);
        return 0;
    }
    `,
expected => ""
},


{
title => q{Signed types},
program => q`
    char comparator(unsigned int w1, unsigned int w2)
    {
        char result;
        if (w1 < w2)
            result = -1;
        else if (w1 > w2)
            result = 1;
        else
            result = 0;
        return result;
    }
    int main()
    {
        sbyte sb = 255;
        assert_eq(sb, -1);

        sword sw = 65535;
        assert_eq(sw, -1);

        // Argument promotion.
        printf("sb=%d\n", sb);

        // Comparison for jumps.
        if (sb < 0)
            printf("sb < 0\n");
        else
            assert(!"sb < 0 failed");
        if (sb <= 0)
            printf("sb <= 0\n");
        else
            assert(!"sb <= 0 failed");

        if (sw < 0)
            printf("sw < 0\n");
        else
            assert(!"sw < 0 failed");
        if (sw <= 0)
            printf("sw <= 0\n");
        else
            assert(!"sw <= 0 failed");

        // Comparisons as integers.
        if (!(sb < 0))
            assert(!"!(sb < 0) failed");
        else
            printf("!!(sb < 0)\n");
        if (!(sb <= 0))
            assert(!"!(sb <= 0) failed");
        else
            printf("!!(sb <= 0)\n");

        if (!(sw < 0))
            assert(!"!(sw < 0) failed");
        else
            printf("!!(sw < 0)\n");
        if (!(sw <= 0))
            assert(!"!(sw <= 0) failed");
        else
            printf("!!(sw <= 0)\n");

        // Comparisons as integers.
        word bool0 = (sb < 0);
        assert(bool0);
        bool0 = (sb <= 0);
        assert(bool0);
        word bool1 = (sw < 0);
        assert(bool1);
        bool1 = (sw <= 0);
        assert(bool1);

        sbyte t = -1;
        word comp = (t > 0);
        assert_eq(comp, 0);
        comp = (t >= 0);
        assert_eq(comp, 0);
        t = 1;
        comp = (t < 0);
        assert_eq(comp, 0);
        comp = (t <= 0);
        assert_eq(comp, 0);

        assert(sb < 0);
        assert(sb <= 0);
        assert(sw < 0);
        assert(sw <= 0);
        assert(!(sb > 0));
        assert(!(sb >= 0));
        assert(!(sw > 0));
        assert(!(sw >= 0));

        assert(comparator(0, 1) < 0);
        assert(comparator(2, 1) > 0);
        assert(comparator(1, 1) == 0);
        assert(comparator(30, 31) < 0);
        assert(comparator(32, 31) > 0);
        assert(comparator(31, 31) == 0);

        sbyte minusOne = -1;

        // Binary operand promotion.
        assert(1000 + (sbyte) 255 == 999);
        assert((sbyte) 255 + 1000 == 999);
        assert(1000 + sb == 999);
        assert(sb + 1000 == 999);
        assert(1000 + sw == 999);
        assert(sw + 1000 == 999);

        assert(1000 - (sbyte) 255 == 1001);
        assert((sbyte) 255 - 1000 == -1001);
        assert(1000 - sb == 1001);
        assert(sb - 1000 == -1001);
        assert(1000 - sw == 1001);
        assert(sw - 1000 == -1001);

        assert_eq(sb * sb, 1);
        assert_eq(sb / sb, 1);
        assert_eq(-1 * -1, 1);
        assert_eq(-1 / -1, 1);
        assert_eq(-1000 * -5, 5000);
        assert_eq(1000 * -5, -5000);
        assert_eq(-5 * -1000, 5000);
        assert_eq(-5 * 1000, -5000);
        assert_eq(-1000 / -5, 200);
        assert_eq(1000 / -5, -200);
        assert_eq(-5 / -1000, 0);
        assert_eq(-5 / 1000, 0);

        sw = -1000;
        sb = -5;
        assert_eq(sb, -5);
        assert_eq(sw * sb, 5000);
        assert_eq(sb * sw, 5000);
        assert_eq(sw / sb, 200);

        sw *= -5;
        assert_eq(sw, 5000);
        sw /= -5;
        sw /= -5;
        assert_eq(sw, 200);

        assert_eq( 23 %  10,  3);
        assert_eq(-23 %  10, -3);
        assert_eq(-23 % -10, -3);
        assert_eq( 23 % -10,  3);

        assert_eq(-1000 % 400, -200);

        sw = -1000;
        assert_eq(sw % 10, 0);
        assert_eq(sw % 9, -1);
        assert_eq(sw % 400, -200);

        sw = 255;
        assert_eq(sw * sw, 65025);
        return 0;
    }
    `,
expected => "sb=-1\nsb < 0\nsb <= 0\nsw < 0\nsw <= 0\n!!(sb < 0)\n!!(sb <= 0)\n!!(sw < 0)\n!!(sw <= 0)\n"
},


{
title => q{sbrk(), sbrkmax()},
program => q`
    int global1 = 0xAABB;
    int main()
    {
        assert_eq(global1, 0xAABB);
        unsigned initMax = sbrkmax();
        assert(initMax > 0);

        // Count number of times 1k can be allocated.
        unsigned counter = 0;
        unsigned bufsiz = 1024;
        void *prevAllocated = (void *) main;
        void *newlyAllocated;
        while ((newlyAllocated = sbrk(bufsiz)) != (void *) -1)
        {
            assert_eq(global1, 0xAABB);
            ++counter;
            assert((unsigned) newlyAllocated > (unsigned) prevAllocated);
            prevAllocated = newlyAllocated;
            
            memset(newlyAllocated, 0xEE, bufsiz);
            assert_eq(global1, 0xAABB);
        }

        assert(counter > 0);
        assert(counter < 64);
        assert(counter * bufsiz <= initMax);
        assert_eq(counter, initMax / bufsiz);
        assert_eq(global1, 0xAABB);

        unsigned finalMax = sbrkmax();
        assert_eq(global1, 0xAABB);
        assert(finalMax < bufsiz);
        assert(finalMax < initMax);
        assert_eq(initMax - counter * bufsiz, finalMax);

        //printf("%u; %u -> %u; %u\n", counter, initMax, finalMax, initMax / bufsiz);
        return 0;
    }
    `,
expected => ""
},


{
title => q{sbrkmax() vs #pragma stack_space},
program => q`
    #pragma stack_space 32768 
    int global1 = 0xAABB;
    int main()
    {
        assert_eq(global1, 0xAABB);
        unsigned initMax = sbrkmax();
        //printf("initMax = %u\n", initMax);
        assert(initMax > 12000 && initMax < 18000);  // would be about 45000 without #pragma
        return 0;
    }
    `,
expected => ""
},


{
title => q{Casting signed byte value into 2-byte types},
program => q`
    struct S { word n; };
    int main()
    {
        assert_eq(sizeof((byte *) 0), 2);
        assert_eq(sizeof((byte *) 100), 2);
        assert_eq((byte *) -1, 0xFFFF);
        assert_eq((word *) -1, 0xFFFF);
        assert_eq((word) -1, 0xFFFF);
        assert_eq((sword) -1, 0xFFFF);
        assert_eq((byte) -1, 0xFF);
        assert_eq((sbyte) -1, -1);

        assert_ne((sbyte) -1, 0xFF);  // negative signed value cannot be equal to positive value

        // Cast to a struct pointer.
        assert_eq((struct S *) -1, 0xFFFF);
        byte buf[2];
        assert(buf);
        struct S *ps = (struct S *) buf;
        assert_eq(ps, buf);
        ps->n = 1844;
        assert_eq(* (word *) buf, 1844);
        * (word *) buf = 4418;
        assert_eq(ps->n, 4418);
        
        int i = (char) -42;
        assert_eq(i, -42);
        
        return 0;
    }
    `,
expected => ""
},


{
title => q{Casting to 2-keyword types},
program => q`
    struct S { char c; };
    int main()
    {
        unsigned char c;
        c = (unsigned char) 42;
        assert_eq(c, 42);
        (signed int) 0xFFFF;
        (unsigned short *) 0x1234;
        (struct S *) 1;
        (struct S ***) 3;
        
        struct S s = { '$' };
        struct S *ps = &s;
        struct S **pps = &ps;
        struct S ***ppps = &pps;
        unsigned short address = (unsigned short) ppps;
        ppps = 0;
        assert_eq(ppps, 0);
        ppps = (struct S ***) address;

        assert_eq(s.c, '$'); 
        assert_eq((*ps).c, '$'); 
        assert_eq((**pps).c, '$'); 
        assert_eq((***ppps).c, '$'); 

        return 0;
    }
    `,
expected => ""
},


{
title => q{Right shift on a signed value must do an arithmetic shift, not a logical one},
program => q`
    void test()
    {
        word initStackPtr;
        asm { sts initStackPtr };

        byte ub = 0x80;
        assert_eq(ub >> 1, 0x40);  // LSR
        sbyte sb = 0x80;
        assert_eq(sb >> 1, (sbyte) 0xC0);  // ASR
        word uw = 0x8000;
        assert_eq(uw >> 1, 0x4000);  // LSR
        sword sw = 0x8000;
        assert_eq(sw >> 1, (sword) 0xC000);  // ASR

        assert_eq(   128 >> 1, 0x40);           // LSR
        assert_eq(   -64 >> 1, 0xFFE0);         // ASR
        assert_eq(((sbyte) -64) >> 1, 0xFFE0);  // ASR: -64 starts as $FFC0, gets cast as $C0, right shift extends it to $FFC0, then shifts it
        assert_eq(  -128 >> 1, 0xFFC0);         // ASR
        assert_eq(  -128 >> 1, (sbyte) 0xC0);   // ASR
        assert_eq( 32768 >> 1, 0x4000);         // LSR
        assert_eq(-16384 >> 1, (sword) 0xE000); // ASR
        assert_eq(-32768 >> 1, (sword) 0xC000); // ASR

        ub = 0x80;
        ub >>= 2;
        assert_eq(ub, 0x20);
        sb = 0x80;
        sb >>= 2;
        assert_eq(sb, 0xE0);

        asm { clrb }  // to prove next line is compiled correctly

        sbyte *psb = &(sb >>= 1);  // shift-assignment as l-value expression
        assert_eq(sb, 0xF0);
        assert_eq(psb, &sb);
        assert_eq(*psb, (sbyte) 0xF0);

        uw = 0x8000;
        uw >>= 2;
        assert_eq(uw, 0x2000);
        sw = 0x8000;
        sw >>= 2;
        assert_eq(sw, 0xE000);
        
        word finalStackPtr;
        asm { sts finalStackPtr };
        assert_eq(finalStackPtr, initStackPtr);
    }
    int main()
    {
        test();
        return 0;
    }
    `,
expected => ""
},


{
title => q{Returning integer 0 allowed in pointer-returning function},
program => q`
    byte *b()
    {
        return 0;
    }
    word *w()
    {
        return 0;
    }
    int main()
    {
        assert_eq(b(), 0);
        assert_eq(w(), 0);
        return 0;
    }
    `,
expected => ""
},


{
title => q{Struct array initializer. For loop increment on a struct pointer. Initializer with non-constant expression},
program => q`
    struct SystemVar
    {
        const char *name;
        char *addr;
        char type;
    };
    struct SystemVar systemVars[] =
    {
        { "foo", 0x0019, 21 },
        { "bar", 0x0112, 25 },
        { "baz", 0x011A, 14 },
        { 0, 0, 0 }  // marks the end
    };
    struct CardBitmap
    {
        unsigned int rows[5];
    };
    struct CardBitmap cardBitmaps[] =
    {
        { { 1, 2, 3, 4, 5 } },  // outer {} for struct, inner for rows[]
        { { 6, 7, 8, 9, 10 } },
        { { 11, 12, 13, 14, 15} },
    };
    int main()
    {
        word numElements = sizeof(systemVars) / sizeof(systemVars[0]) - 1;
        assert_eq(numElements, 3);
        char buffer[128];
        buffer[0] = 0;
        word addrSum = 0;
        word typeSum = 0;
        for (struct SystemVar *p = systemVars; p->name; ++p)
        {
            strcat(buffer, p->name);
            addrSum += (word) p->addr;
            typeSum += p->type;
        }
        assert(!strcmp(buffer, "foobarbaz"));
        assert_eq(addrSum, 0x0019 + 0x0112 + 0x011A);
        assert_eq(typeSum, 21 + 25 + 14);
        
        word wa[2] = { addrSum + 1, typeSum * 2 };
        assert_eq(wa[0], 0x0019 + 0x0112 + 0x011A + 1);
        assert_eq(wa[1], (21 + 25 + 14) * 2);
        
        assert_eq(cardBitmaps[0].rows[0], 1);
        assert_eq(cardBitmaps[1].rows[2], 8);
        assert_eq(cardBitmaps[2].rows[4], 15);

        return 0;
    }
    `,
expected => ""
},


{
title => q{Initializer for structs and arrays},
program => q`
    struct S
    {
        byte b;
        word w;
    };
    struct S global =
    {
        255,
        12700
    };
    struct StructWithArray
    {
        byte b;
        word a[3];
        word w;
    };
    struct StructWithArray globalWithArray =
    {
        77, { 2000, 21, 2002 }, 9988
    };
    struct StructWithArray globalArrayOfStructWithArray[2] =
    {
        {  77, { 2000, 21, 2002 },  9988 },
        { 177, { 2100, 22, 2102 }, 10088 },
    };
    struct StructWithByteArray
    {
        char str[7];
    };
    struct StructWithByteArray globalString = { "foobar" };
    
    struct A
    {
        byte byteField;
    }; 
    struct A a = { 10 };
    struct B
    {
        struct A aMember;
    };
    struct B b = { { 11 } };
    struct C
    {
        struct A aMemberArray[2];
    };
    struct C c = { { { 12 }, { 13 } } };
    
    int main()
    {
        assert_eq(global.b, 255);
        assert_eq(global.w, 12700);

        struct S local =
        {
            25,
            1200
        }; 
        assert_eq(local.b, 25);
        assert_eq(local.w, 1200);
        
        assert_eq(globalWithArray.b, 77);
        assert_eq(globalWithArray.a[0], 2000);
        assert_eq(globalWithArray.a[1], 21);
        assert_eq(globalWithArray.a[2], 2002);
        assert_eq(globalWithArray.w, 9988);
        
        assert_eq(globalArrayOfStructWithArray[0].b,      77);
        assert_eq(globalArrayOfStructWithArray[0].a[0], 2000);
        assert_eq(globalArrayOfStructWithArray[0].a[1],   21);
        assert_eq(globalArrayOfStructWithArray[0].a[2], 2002);
        assert_eq(globalArrayOfStructWithArray[0].w,    9988);

        assert_eq(globalArrayOfStructWithArray[1].b,     177);
        assert_eq(globalArrayOfStructWithArray[1].a[0], 2100);
        assert_eq(globalArrayOfStructWithArray[1].a[1],   22);
        assert_eq(globalArrayOfStructWithArray[1].a[2], 2102);
        assert_eq(globalArrayOfStructWithArray[1].w,   10088);
        
        struct StructWithArray localArrayOfStructWithArray[2] =
        {
            {  77, { 2000, 21, 2002 },  9988 },
            { 177, { 2100, 22, 2102 }, 10088 },
        };
        
        assert(!strcmp(globalString.str, "foobar"));
        struct StructWithByteArray localString = { "FOOBAR" };
        assert(!strcmp(localString.str, "FOOBAR"));
        
        assert_eq(a.byteField, 10);
        assert_eq(b.aMember.byteField, 11);
        assert_eq(c.aMemberArray[0].byteField, 12);
        assert_eq(c.aMemberArray[1].byteField, 13);

        return 0;
    }
    `,
expected => ""
},


{
title => q{Pointer to pointer, array of pointers},
program => q`
    const char *strings[] = { "foo", "bar", "baz", "abcdefghij" };
    const char *moreStrings[][2] = { { "foo", "bar" }, { "baz", "abcdefghij" } };
    struct S { char b[2]; };
    void check(const char **pba)
    {
        const char *s = pba[0];
        assert(!strcmp(s, "foo"));
        s = pba[1];
        assert(!strcmp(s, "bar"));
        s = pba[2];
        assert(!strcmp(s, "baz"));
        assert_eq(s[2], 'z');
    }
    int main()
    {
        assert(!strcmp(strings[0], "foo"));
        assert(!strcmp(strings[1], "bar"));
        assert(!strcmp(strings[2], "baz"));
        assert(!strcmp(strings[3], "abcdefghij"));
        assert_eq(strings[3][7], 'h');
        
        const char **ptrToByteArray = strings;
        const char *s = ptrToByteArray[0];
        assert(!strcmp(s, "foo"));
        s = ptrToByteArray[1];
        assert(!strcmp(s, "bar"));
        s = ptrToByteArray[2];
        assert(!strcmp(s, "baz"));
        assert_eq(s[2], 'z');
        
        check(strings);
        
        char *bytePtr = (char *) 0x1234;
        word w = 0x9876;
        * (char **) &w = bytePtr;
        assert_eq(w, (word) bytePtr);
        
        w = 0x6655;
        ((struct S *) &w)->b[1] = 0x77;
        assert_eq(w, 0x6677);
        
        (* (struct S **) &w) = 0x5678;
        assert_eq(w, 0x5678);
        
        assert(!strcmp(moreStrings[0][0], "foo"));
        assert(!strcmp(moreStrings[0][1], "bar"));
        assert(!strcmp(moreStrings[1][0], "baz"));
        assert(!strcmp(moreStrings[1][1], "abcdefghij"));
        assert_eq(moreStrings[1][1][9], 'j');

        return 0;
    }
    `,
expected => ""
},


{
title => q{Automatic conversion of an integer to a pointer, without a warning},
program => q`
    int main()
    {
        byte *p = 0x123;
        assert_eq(p, 0x123);
        p = 0x456;
        assert_eq(p, 0x456);
        return 0;
    }
    `,
expected => ""
},


{
title => q{char, short, signed, unsigned},
program => q`
    word f(char ch, short sh, signed si, unsigned un)
    {
        return ch + sh + si + un;
    }
    int main()
    {
        char ch;
        assert_eq(sizeof(ch), 1);
        short sh;
        assert_eq(sizeof(sh), 2);
        signed si;
        assert_eq(sizeof(si), 2);
        unsigned un;
        assert_eq(sizeof(un), 2);
        assert_eq(f(-128, -300, -400, 10000), 9172);

        signed char sch;
        assert_eq(sizeof(sch), 1);        
        unsigned char uch;
        assert_eq(sizeof(uch), 1);
        signed short shint;
        assert_eq(sizeof(shint), 2);
        unsigned short unshint;
        assert_eq(sizeof(unshint), 2);
        signed int sint;
        assert_eq(sizeof(sint), 2);
        unsigned int unsint;
        assert_eq(sizeof(unsint), 2);
        return 0;
    }
    `,
expected => ""
},


{
title => q{typedef},
program => q`
    typedef unsigned short *Ptr, uint16_t;  // more than one declarator is allowed in a typedef
    
    // Typedef for an array:
    typedef unsigned Addr[2];
    Addr addr = { 0xAAAA, 0xBBBB }; 

    typedef char string10_t[10];
    typedef struct {
        string10_t str1;
        char str2[10];
        int str3[10];
    } test_t;

    struct S {
        string10_t grid[5];
        char c;
    };
    
    typedef int TenInts[10];
    struct T {
        TenInts tens[5];
    };        

    void f1(char *a) {}
    void f2(int *a) {}
    unsigned diff(void *a, void *b) { return b - a; }

    int main()
    {
        uint16_t u = 2014;
        assert_eq(u, 2014);
        Ptr p = &u;
        assert_eq(*p, 2014);
        
        assert_eq(addr[0], 0xAAAA);
        assert_eq(addr[1], 0xBBBB);
        addr[0] = 0x1111;
        addr[1] = 0x2222;
        assert_eq(addr[0], 0x1111);
        assert_eq(addr[1], 0x2222);

        Addr localAddr = { 1234, 2345 };
        assert_eq(localAddr[0] + localAddr[1], 3579);
        ++localAddr[0];
        ++localAddr[1];
        assert_eq(localAddr[0] + localAddr[1], 3581);
        
        string10_t str1;
        assert_eq(sizeof(str1), 10);
        assert_eq((str1 + 1) - str1, 1);

        string10_t tens[5];
        assert_eq(sizeof(tens), 50);
        assert_eq(sizeof(tens[2]), 10);
        assert_eq(tens[4] - tens[3], 10);
        assert_eq((tens + 1) - tens, 1);
        
        char str2[10];
        test_t test;
        assert_eq(sizeof(string10_t), 10);
        assert_eq(sizeof(test.str1), 10);
        assert_eq(sizeof(test.str2), 10);
        assert_eq(sizeof(test.str3), 20);
        assert_eq(sizeof(test), 40);
        f1(str1);
        f1(str2);
        f1(test.str1);
        f1(test.str2);
        f2(test.str3);
        assert_eq(diff( test.str1,  test.str2), 10);
        assert_eq(diff(&test.str1,  test.str2), 10);
        assert_eq(diff( test.str1, &test.str2), 10);
        assert_eq(diff( test.str2,  test.str3), 10);
        assert_eq(diff(&test.str2,  test.str3), 10);
        assert_eq(diff( test.str2, &test.str3), 10);

        struct S s;
        assert_eq(sizeof(s.grid), 50);
        assert_eq(sizeof(s.grid[2]), 10);
        assert_eq(sizeof(s), 51);
        assert_eq(diff(s.grid, &s.c), 50);
        
        struct T t;
        assert_eq(sizeof(TenInts), 10 * 2);
        assert_eq(sizeof(t.tens), 50 * 2);
        assert_eq(sizeof(t.tens[3]), 10 * 2);

        // Initializers.

        char str3[10] = "foobarbaz";
        assert_eq(strcmp(str3, "foobarbaz"), 0);
        assert_eq(strlen(str3), 9); 
        string10_t str4 = "FOOBARBAZ"; 
        assert_eq(strcmp(str4, "FOOBARBAZ"), 0);
        assert_eq(strlen(str4), 9); 
        
        string10_t strArray1[] = { "A", "B" };
        assert_eq(strArray1, strArray1[0]);
        assert_ne(strArray1[0], strArray1[1]);
        assert_eq(strcmp(strArray1[0], "A"), 0);
        assert_eq(strcmp(strArray1[1], "B"), 0);
        
        struct S s1 = { { "a", "b", "c", "d", "e" }, '$' };
        assert_eq(s1.grid, s1.grid[0]);
        assert_ne(s1.grid, s1.grid[1]);
        assert_ne(s1.grid, &s1.c);
        assert_ne(s1.grid[4], &s1.c);
        assert_eq(strcmp(s1.grid[0], "a"), 0);
        assert_eq(strcmp(s1.grid[1], "b"), 0);
        assert_eq(strcmp(s1.grid[2], "c"), 0);
        assert_eq(strcmp(s1.grid[3], "d"), 0);
        assert_eq(strcmp(s1.grid[4], "e"), 0);
        assert_eq(s1.c, '$');
        
        struct S s2 = { { "f", { 'g', 0, 1, 2, 3, 4, 5, 6, 7, 8 }, "h", "i", "j" }, '%' };
        assert_eq(strcmp(s2.grid[0], "f"), 0);
        assert_eq(strcmp(s2.grid[1], "g"), 0);
        assert_eq(s2.grid[1][9], 8);
        assert_eq(strcmp(s2.grid[2], "h"), 0);
        assert_eq(strcmp(s2.grid[3], "i"), 0);
        assert_eq(strcmp(s2.grid[4], "j"), 0);
        assert_eq(s2.c, '%');
        
        return 0;
    }
    `,
expected => ""
},


{
title => q{Multi-dimensional arrays},
program => q`
    int twoD[3][5];
    int threeD[4][3][5];
    int v[][2][3] = {
        { { 10, 11, 12 }, { 13, 14, 15 } },
        { { 20, 21, 22 }, { 23, 24, 25 } },
        { { 30, 31, 32 }, { 33, 34, 35 } },
        { { 40, 41, 42 }, { 43, 44, 45 } },
        };
    unsigned char b[][2][3] = {  // same as v, but 100 added to each row
        { { 110, 11, 12 }, { 13, 14, 15 } },
        { { 20, 121, 22 }, { 23, 24, 25 } },
        { { 30, 31, 132 }, { 33, 34, 35 } },
        { { 40, 41, 42 }, { 143, 44, 45 } },
        };
    struct Point
    {
        int x;
        int y;
        int z;
    };
    struct Point s[2][3] = {
        { {  1,  2,  3 }, {  4,  5,  6 }, {  7,  8,  9 }, },
        { { 10, 11, 12 }, { 13, 14, 15 }, { 16, 17, 18 }, },
        };
    int computeSum(int v[2][3][4])
    {
        int result = 0;
        for (int i = 0; i < 2; ++i)
            for (int j = 0; j < 3; ++j)
                for (int k = 0; k < 4; ++k)
                    result += v[i][j][k];
        return result;
    }
    struct S
    {
        int a[2][3][4];
    };
    int main()
    {
        // 2 dimensions.
        int *p = (int *) twoD;
        for (int i = 0; i < 15; ++i)
            p[i] = 100 + i;
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 5; ++j)
            {
                int expected = 100 + i * 5 + j;
                if (twoD[i][j] != expected)
                {
                    printf("ERROR: line %d: twoD[%d][%d] == %d, expected %d\n",
                            __LINE__, i, j, twoD[i][j], expected);
                    exit(1);
                }
            }

        // Constant indexes.
        twoD[2][4] = 244;
        assert_eq(twoD[2][4], 244);

        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 5; ++j)
                twoD[i][j] = 200 - i * 5 - j;
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 5; ++j)
            {
                int *actualAddress = &twoD[i][j];
                unsigned actualOffset = actualAddress - twoD;
                assert_eq(actualOffset, i * 5 + j);
            
                assert_eq(twoD[i][j], 200 - i * 5 - j);
            }
            
        int sum = 0;

        // 3 dimensions.
        p = (int *) threeD;
        for (int i = 0; i < 60; ++i)
            p[i] = 100 + i;
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 3; ++j)
                for (int k = 0; k < 5; ++k)
                {
                    int *actualAddress = &threeD[i][j][k];
                    unsigned actualOffset = actualAddress - threeD;
                    assert_eq(actualOffset, i * 15 + j * 5 + k);

                    int expected = 100 + i * 15 + j * 5 + k;
                    if (threeD[i][j][k] != expected)
                    {
                        printf("ERROR: line %d: threeD[%d][%d][%d] == %d, expected %d\n",
                                __LINE__, i, j, k, threeD[i][j][k], expected);
                        exit(1);
                    }
                    int temp = threeD[i][j][k];
                    ++threeD[i][j][k];
                    assert_eq(threeD[i][j][k], temp + 1);
                }

        // Constant indexes.
        threeD[3][2][4] = 1000;
        assert_eq(threeD[3][2][4], 1000);

        // Size of first dimension specified by initializer.
        sum = 0;
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 2; ++j)
                for (int k = 0; k < 3; ++k)
                    sum += v[i][j][k];
        assert_eq(sum, 660);

        // Bytes.
        sum = 0;
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 2; ++j)
                for (int k = 0; k < 3; ++k)
                    sum += b[i][j][k];
        assert_eq(sum, 660 + 400);

        // Multi-dim. array of structs.
        sum = 0;
        for (int i = 0; i < 2; ++i)
            for (int j = 0; j < 3; ++j)
                sum += s[i][j].x + s[i][j].y + s[i][j].z;
        assert_eq(sum, 171);
    
        // Struct containing multi-dim array member.
        struct S s;
        assert_eq(sizeof(s), sizeof(int) * 2 * 3 * 4);
        memset(&s, 0, sizeof(s));
        s.a[0][0][0] = 42;
        assert_eq(s.a[0][0][0], 42);
        s.a[1][2][3] = 1844;
        assert_eq(s.a[1][2][3], 1844);
        assert_eq(computeSum(s.a), 42 + 1844);

        return 0;
    }
    `,
expected => ""
},


{
title => q{Function parameter of array type (one or more dimensions)},
program => q`
    int f(int a[], int i)
    {
        return a[i];
    }
    int g(int a[][3], int i, int j)
    {
        return a[i][j];
    }
    int h(int a[][3][3], int i, int j, int k)
    {
        return a[i][j][k];
    }
    int *fptr(int a[], int i)
    {
        return &a[i];
    }
    int *gptr(int a[][3], int i, int j)
    {
        return &a[i][j];
    }
    int *hptr(int a[][3][3], int i, int j, int k)
    {
        return &a[i][j][k];
    }
    int *getArray2(int a[][3], int i)
    {
        return a[i];
    }
    int *getArray3(int a[][3][3], int i, int j)
    {
        return a[i][j];
    }
    void checkConstIndexes(int a[][3])
    {
        int *p = &a[4][2];
        assert_eq(p - a, 4 * 3 + 2);
        
        int i = 2, j = 5;
        p = &a[i][j];
        assert_eq(p - a, 2 * 3 + 5);
    }
    int main()
    {
        int v1[] = { 5, 6, 7, 8 };
        assert_eq(f(v1, 2), 7);
        int v2[][3] = { { 9, 8, 7 }, { 99, 88, 77 }, { 66, 55, 44 } };  
        assert_eq(f(v2[1], 2), 77);  // v2[1] points to { 99, 88, 77 } 
        assert_eq(g(v2, 1, 2), 77);
        int v3[][3][3] =
            {
                {
                    { 9, 8, 7 },
                    { 99, 88, 77 },
                    { 66, 55, 44 }
                },
                { 
                    { 19, 18, 17 },
                    { 199, 188, 177 },
                    { 166, 155, 144 }
                }
            };
        assert_eq(h(v3, 1, 2, 1), 155);
        
        assert_eq(sizeof(v1), 2 * 4);
        assert_eq(sizeof(v2), 2 * 3 * 3);
        assert_eq(sizeof(v3), 2 * 2 * 3 * 3);
        
        int sum;
        
        sum = 0;
        for (int i = 0; i < 4; ++i)
            sum += f(v1, i);
        assert_eq(sum, (int)5+6+7+8);
        
        sum = 0;
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                sum += g(v2, i, j);
        assert_eq(sum, (int)9+8+7 + (int)99+88+77 + (int)66+55+44);

        sum = 0;
        for (int i = 0; i < 2; ++i)
            for (int j = 0; j < 3; ++j)
                for (int k = 0; k < 3; ++k)
                    sum += h(v3, i, j, k);
        assert_eq(sum, 1536);
        
        // Test functions like f, g, h but that return pointers.
        //
        *fptr(v1, 2) = 4545;
        assert_eq(f(v1, 2), 4545);
        *gptr(v2, 1, 2) = 9876;
        assert_eq(g(v2, 1, 2), 9876);
        *hptr(v3, 1, 2, 1) = 1234;
        assert_eq(h(v3, 1, 2, 1), 1234);
        
        // Test functions that return an array.
        //
        assert_eq(getArray2(v2, 2)[1], 55);
        assert_eq(getArray3(v3, 1, 1)[1], 188);
        
        int aa[1][3] = { { 1, 2, 3 } };
        checkConstIndexes(aa);
        
        return 0;
    }
    `,
expected => ""
},


{
title => q{switch()},
program => q`
    int testSwitch1(int n)
    {
        int ret = 7777;
        switch (n)
        {
        case -3:
            return 100;
        case 0:
            ret = 101;
            break;
        case 5:
            ret = 102;
            // FALLTHROUGH
        case 2:
            ret = 103;
            break;
        default:
            ret = 999;
        }
        return ret;
    }
    int testSwitch2(int n)  // default in the middle
    {
        int ret = 7777;
        switch (n)
        {
        case -3:
            return 100;
        default:
            ret = 999;
            break;
        case 0:
            ret = 101;
            break;
        case 5:
            ret = 102;
            // FALLTHROUGH
        case 2:
            ret = 103;
        }
        return ret;
    }
    int testSwitch3(char n)  // byte expression, default in the middle
    {
        int ret = 7777;
        switch (n)
        {
        case -3:
            return 100;
        default:
            ret = 999;
            break;
        case 0:
            ret = 101;
            break;
        case 5:
            ret = 102;
            // FALLTHROUGH
        case 2:
            ret = 103;
        
        }
        
        return ret;
    }
    char testSwitch4(char n)
    {
        switch (n)
        {
            case 2:
            case 8:
            case 11:  // jack
                return 0;
            default:
                return 1;
        }
    }
    int testSwitch5(unsigned n)  // default in the middle
    {
        int ret = 7777;
        switch (n)
        {
        case 65533:
            return 100;
        default:
            ret = 999;
            break;
        case 0:
            ret = 101;
            break;
        case 5:
            ret = 102;
            // FALLTHROUGH
        case 2:
            ret = 103;
        }
        return ret;
    }
    int testSwitch6(unsigned n)
    {
        int ret = 7777;
        switch (n)
        {
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
        case 17:
        case 18:
        case 19:
        case 20:
        case 21:
        case 22:
        case 23:
        case 24:
            return 100;
        case 30:
        case 31:
        case 32:
        case 33:
        case 34:
        case 35:
        case 36:
        case 37:
        case 38:
        case 39:
            return 101;
        default:
            return 102;
        }
        return ret;
    }
    int main()
    {
        assert_eq(testSwitch1(-3), 100);
        assert_eq(testSwitch1( 0), 101);
        assert_eq(testSwitch1( 5), 103);
        assert_eq(testSwitch1( 2), 103);
        assert_eq(testSwitch1(-9), 999);

        assert_eq(testSwitch2(-3), 100);
        assert_eq(testSwitch2( 0), 101);
        assert_eq(testSwitch2( 5), 103);
        assert_eq(testSwitch2( 2), 103);
        assert_eq(testSwitch2(-9), 999);

        assert_eq(testSwitch3(-3), 100);
        assert_eq(testSwitch3( 0), 101);
        assert_eq(testSwitch2( 5), 103);
        assert_eq(testSwitch3( 2), 103);
        assert_eq(testSwitch3(-9), 999);
        
        assert_eq(testSwitch4(2),  0);
        assert_eq(testSwitch4(8),  0);
        assert_eq(testSwitch4(11), 0);
        assert_eq(testSwitch4(13), 1);

        assert_eq(testSwitch2(65533), 100);
        assert_eq(testSwitch2(    0), 101);
        assert_eq(testSwitch2(    5), 103);
        assert_eq(testSwitch2(    2), 103);
        assert_eq(testSwitch2(65527), 999);
        
        for (unsigned i = 3; i <= 41; ++i)
        {
            int expected = (i < 5 ? 102 : (i <= 24 ? 100 : (i <= 29 ? 102 : (i <= 39 ? 101 : 102)))); 
            int actual = testSwitch6(i);
            //printf("i=%2u: got %5u, expected %5u\n", i, actual, expected);
            assert_eq(actual, expected);
        }
        
        // break inside an if().
        int n = 17;
        switch (3)
        {
        case 3:
            if (n == 17)
            {
                n = 88;
                break;
            }
        case 4:
            n = 99;
            break;
        }
        assert_eq(n, 88);
        
        return 0;
    }
    `,
expected => ""
},


{
title => q{break vs. switch in a switch},
program => q`
    char testSwitchInASwitch(byte a, byte b)
    {
        //printf("testSwitchInASwitch(a=%u, b=%u): start\n", a, b);
        byte iter = 0;
        switch (a)
        {
        case 10:
            //printf("case 10\n");
            switch (b)
            {
            case 22:
                //printf("case 22\n");
                break;
            default:
                return 1;
            }
            //printf("iter=%u\n", iter);
            if (++iter > 1)
                return 2;
            break;  // With CMOC <= 0.1.22, we'd jump to end of inner switch(),
                    // causing infinite loop, which 'iter' detects.
                    // Cause: Missing call to popBreakableLabels()
                    // in SwitchStmt::emitCode().
        }
        //printf("testSwitchInASwitch: end\n");
        return 0;  // supposed to come here
    }
    int main()
    {
        byte result = testSwitchInASwitch(10, 22);
        assert_eq(result, 0);
        return 0;
    }
    `,
expected => ""
},


{
title => q{break vs. switch in a loop},
program => q`
    char testSwitchInAFor(byte b)
    {
        byte iter = 0;
        for (;;)
        {
            switch (b)
            {
            case 22:
                //printf("case 22\n");
                break;
            default:
                return 1;
            }
            //printf("iter=%u\n", iter);
            if (++iter > 1)
                return 2;
            break;
        }
        return 0;  // supposed to come here
    }
    char testSwitchInAWhile(byte b)
    {
        byte iter = 0;
        while (1)
        {
            switch (b)
            {
            case 22:
                //printf("case 22\n");
                break;
            default:
                return 1;
            }
            //printf("iter=%u\n", iter);
            if (++iter > 1)
                return 2;
            break;
        }
        return 0;  // supposed to come here
    }
    char testSwitchInADoWhile(byte b)
    {
        byte iter = 0;
        do
        {
            switch (b)
            {
            case 22:
                //printf("case 22\n");
                break;
            default:
                return 1;
            }
            //printf("iter=%u\n", iter);
            if (++iter > 1)
                return 2;
            break;
        } while (1);
        return 0;  // supposed to come here
    }
    char testForInSwitch(byte b)
    {
        byte iter = 0;
        switch (b)
        {
        case 22:
            for (;;)
            {
                break;
            }
            if (++iter > 1)
                return 2;
            break;
        default:
            return 1;
        }
        return 0;  // supposed to come here
    }
    int main()
    {
        byte result = testSwitchInAFor(22);
        assert_eq(result, 0);
        result = testSwitchInAWhile(22);
        assert_eq(result, 0);
        result = testSwitchInADoWhile(22);
        assert_eq(result, 0);
        result = testForInSwitch(22);
        assert_eq(result, 0);
        return 0;
    }
    `,
expected => ""
},


{
title => q{Function pointer in an array or a struct},
program => q`
    int f() { return 99; }
    struct S {
        int (*funcPtr)();
    };
    char g;
    void someWork() { g = 42; }
    int addOne(int x) { return x + 1; }
    struct T {
        void (*work)();
        int (*incLong)(int n); 
    };
    int main()
    {
        {
            int (*funcPtrs[])() = { f };
            assert_eq(funcPtrs[0](), 99);
            assert_eq((*funcPtrs[0])(), 99);
    
            struct S s = { f };
            assert_eq(s.funcPtr, f);
            assert_eq(s.funcPtr(), 99);
            assert_eq((*s.funcPtr)(), 99);
        }
        
        {
            struct T funcPtrs;
            funcPtrs.work = someWork;
            funcPtrs.incLong = addOne;
            assert_eq((char) funcPtrs.work(), 42);
            assert(funcPtrs.incLong(4444) == 4445);
        }
        
        return 0;
    }
    `,
expected => ""
},


{
title => q{+=, -=},
program => q`
    int main()
    {
        char a = 42;
        char b = 11;
        a += b;
        assert_eq(a, 53);
        a -= b;
        assert_eq(a, 42);
        
        int c = 4000;
        int d = 123;
        c += d;
        assert_eq(c, 4123);
        c -= d;
        assert_eq(c, 4000);

        return 0;
    }
    `,
expected => ""
},


{
title => q{+=, -=, *= operators with right-hand side of pointer type},
program => q`
    typedef unsigned char *FuncPtr;
    int main()
    {
        FuncPtr origISR = 0xFEF7;

        FuncPtr finalAddr = 0x0212;
        finalAddr += 0xFEFA;
        assert_eq(finalAddr, 0x010C);

        finalAddr = 0x0212;
        finalAddr += origISR + 3;
        assert_eq(finalAddr, 0x010C);

        finalAddr = 0x0212;
        finalAddr += origISR;
        assert_eq(finalAddr, 0x0109);

        finalAddr = 0x0212;
	origISR = 0x0101;
        finalAddr -= origISR;
        assert_eq(finalAddr, 0x0111);

        finalAddr = 0x0212;
	origISR = 3;
        finalAddr *= origISR;
        assert_eq(finalAddr, 0x0636);

        finalAddr = 0x0212;
	origISR = 2;
        finalAddr /= origISR;
        assert_eq(finalAddr, 0x0109);

        return 0;
    }
    `,
expected => ""
},


{
title => q{#pragma const_data},
suspended => 1,
compilerOptions => "--org=5C00 --data=4F00",
program => q`
    // Read-only globals go after 5C00.
    // Writable globals go after 4F00.
    int writable = 42;
    #pragma const_data start
    unsigned char readonly[] = { 9, 8, 7, 6 };
    int k = 1000;
    #pragma const_data end
    int main()
    {
        assert_eq(writable, 42);
        assert_eq(readonly[2], 7);
        assert_eq(k, 1000);
        
        assert(&writable >= (void  *) 0x4F00 && &writable < 0x5000);
        assert(readonly > (void *) 0x5C00);
        assert(&k > (void *) 0x5C00);
        return 0;
    }
    `,
expected => ""
},


{
title => q{const globals go in rodata section},
compilerOptions => "--org=5C00 --data=4F00",
program => q`
    // Read-only globals go after 5C00.
    // Writable globals go after 4F00.
    // This test is the same as "#pragma const_data", but with const keyword
    // instead of #pragma const_data.
    int writable = 42;
    const unsigned char readonly[] = { 9, 8, 7, 6 };
    const int k = 1000;
    int main()
    {
        assert_eq(writable, 42);
        assert_eq(readonly[2], 7);
        assert_eq(k, 1000);
        
        assert(&writable >= (void  *) 0x4F00 && &writable < 0x5000);
        assert(readonly > (void *) 0x5C00); 
        assert(&k > (void *) 0x5C00);
        return 0;
    }
    `,
expected => ""
},


{
title => q{global array of const arrays with --no-relocate},
compilerOptions => "--org=5C00 --data=4F00 --no-relocate",
program => q`
    const unsigned char byteArray0[] = { 99, 88, 77, 66 };
    const unsigned char byteArray1[] = { 55, 44, 33, 22 };
          unsigned char byteArray2[] = { 12, 34, 56, 78 };
    const unsigned char * const byteArraysA[] = { byteArray0, byteArray1 };
    const unsigned char *       byteArraysB[] = { byteArray0, byteArray1 };
    const unsigned char * const byteArraysC[] = { byteArray2 };
    int main()
    {
        assert(byteArray0  >= (void *) 0x5C00);  // rodata
        assert(byteArray1  >= (void *) 0x5C00);  // rodata
        assert(byteArraysA >= (void *) 0x5C00);  // rodata
        assert(byteArraysB <  (void *) 0x5C00);  // rwdata b/c byteArraysB[i] can be changed 
        assert(byteArraysC >= (void *) 0x5C00);  // rodata
        assert_eq(byteArraysA[0], byteArray0);
        assert_eq(byteArraysA[1], byteArray1);
        assert_eq(byteArraysB[0], byteArray0);
        assert_eq(byteArraysB[1], byteArray1);
        assert_eq(byteArraysC[0], byteArray2);
        assert_eq(byteArraysA[0][2], 77);
        assert_eq(byteArraysB[1][3], 22);
        assert_eq(byteArraysC[0][1], 34);
        return 0;
    }
    `,
expected => ""
},


{
title => q{global array of const arrays without relocatability},
compilerOptions => "--org=5C00 --data=4F00",  # not passing --no-relocate
program => q`
    const unsigned char byteArray0[] = { 99, 88, 77, 66 };
    const unsigned char byteArray1[] = { 55, 44, 33, 22 };
          unsigned char byteArray2[] = { 12, 34, 56, 78 };
    const unsigned char * const byteArraysA[] = { byteArray0, byteArray1 };
    const unsigned char *       byteArraysB[] = { byteArray0, byteArray1 };
    const unsigned char * const byteArraysC[] = { byteArray2 };
    int main()
    {
        assert(byteArray0  >= (void *) 0x5C00);  // rodata
        assert(byteArray1  >= (void *) 0x5C00);  // rodata
        assert(byteArraysA <  (void *) 0x5C00);  // rwdata b/c byteArray0/byteArray1 depend on load address
        assert(byteArraysB <  (void *) 0x5C00);  // ditto
        assert(byteArraysC <  (void *) 0x5C00);  // ditto re: byteArray2
        assert_eq(byteArraysA[0], byteArray0);
        assert_eq(byteArraysA[1], byteArray1);
        assert_eq(byteArraysB[0], byteArray0);
        assert_eq(byteArraysB[1], byteArray1);
        assert_eq(byteArraysC[0], byteArray2);
        assert_eq(byteArraysA[0][2], 77);
        assert_eq(byteArraysB[1][3], 22);
        assert_eq(byteArraysC[0][1], 34);
        return 0;
    }
    `,
expected => ""
},


{
title => q{Function pointer in an array vs. --no-relocate},
compilerOptions => "--org=5C00 --data=4F00 --no-relocate",
program => q`
    int f() { return 9999; }
    typedef int (*FuncPtr)();
    const FuncPtr fpArray[] = { f };
    int main()
    {
        assert(fpArray >= (void *) 0x5C00);  // rodata
        return 0;
    }
    `,
expected => ""
},


{
title => q{Function pointer in an array vs. relocatability},
compilerOptions => "--org=5C00 --data=4F00",
program => q`
    int f() { return 9999; }
    typedef int (*FuncPtr)();
    const FuncPtr fpArray[] = { f };
    int main()
    {
        assert(fpArray < (void *) 0x5C00);  // rwdata b/c address of function f() depends on load address
        return 0;
    }
    `,
expected => ""
},


{
title => q{#pragma exec_once},
program => q`
    #pragma exec_once
    int main()
    {
        char *_program_end;
        char *_INITGL;
        asm {
            leax    program_end,pcr
            stx     _program_end
            leax    INITGL,pcr
            stx     _INITGL
        }
        assert(_program_end == _INITGL);
        return 0;
    }
    `,
expected => ""
},


{
title => q{Absence of #pragma exec_once},
program => q`
    int main()
    {
        char *_program_end;
        char *_INITGL;
        asm {
            leax    program_end,pcr
            stx     _program_end
            leax    INITGL,pcr
            stx     _INITGL
        }
        assert(_program_end > _INITGL);
        return 0;
    }
    `,
expected => ""
},


{
compilerOptions => "--check-null",
title => q{--check-null option to detect null pointer accesses},
program => q`
    char called = 0;
    void *g_addressOfFailedCheck = 0;
    void nullPointerHandler(void *addressOfFailedCheck)
    {
        assert(addressOfFailedCheck >= 0x4000 && addressOfFailedCheck < 0x5000);
        called = 1;
        g_addressOfFailedCheck = addressOfFailedCheck;
    }
    int main()
    {
        set_null_ptr_handler(nullPointerHandler);
        void *before, *after;
        char *p = 0;
        asm {
Before:
        }
        char c = *p;  // must trigger call to nullPointerHandler()
        asm {
After:
            leax    Before,pcr
            stx     :before
            leax    After,pcr
            stx     :after
        }
        assert_eq(called, 1);
        // Address of failed check is expected to be between labels Before and After:
        assert(before <= g_addressOfFailedCheck && g_addressOfFailedCheck < after);
        return 0;
    }
    `,
expected => ""
},


{
compilerOptions => "--check-stack",
title => q{--check-stack option to detect stack overflows},
program => q`
    void stackOverflowHandler(void *addressOfFailedCheck, void *stackRegister)
    {
        //printf("[%p, %p]\n", addressOfFailedCheck, stackRegister);
        assert(addressOfFailedCheck >= 0x4000 && addressOfFailedCheck < 0x5000);
        assert(stackRegister >= 0xFA00 && stackRegister < 0xFE00);  // 1k of stack space
        exit(1);  // expected point of exit
    }
    unsigned level = 0;
    void checkLevel()  // fail if too much recursion, i.e., stack check did not detect overflow
    {
        if (level < 1000)
            return;
        void *sreg;
        asm { sts :sreg }
        printf("ER""ROR: level %u, S=%p\n", level, sreg);
        exit(2);
    }
    void recurse()
    {
        ++level;
        checkLevel();  // exit(1) statement should have been reached before this fails
        recurse();
    }
    int main()
    {
        set_stack_overflow_handler(stackOverflowHandler);
        recurse();
        return 0;
    }
    `,
expected => ""
},


{
title => q{Typical C copying idiom},
program => q`
    int main()
    {
        const char *str = "foobar";
        const char *src;
        for (src = str; *src++; )
            ;
        assert_eq(src - str, 7);

        for (src = str; *src; src++)
            ;
        assert_eq(src - str, 6);

        char a[7];
        memset(a, 'X', sizeof(a));
        char *dest;
        for (src = str, dest = a; *src; )
            *dest++ = *src++;
        assert_eq(*dest, 'X');
        *dest = '\0';
        printf("%s\n", a);
        assert_eq(strcmp(a, "foobar"), 0);
        assert_eq(src - str, 6);
        assert_eq(*src, 0);
        return 0;
    }
    `,
expected => "foobar\n"
},


{
title => q{Adding a constant to an array address received as an argument},
program => q`
    char *f(char *p)
    {
        //printf("p=%p\n", p);
        return p;
    }
    void g(char a[12])
    {
        char *p0 = f(a);
        char *p6 = f(a + 6);  // bug was in BinaryOpExpr::emitAddImmediateToVariable()
        assert_eq(p6 - p0, 6);
    }
    int main()
    {
        char v[12];
        g(v);
        return 0;
    }
    `,
expected => ""
},


{
title => q{sprintf(), putstr(), putchar()},
program => q`
    int main()
    {
        char buf[10];
        assert_eq(sizeof(buf), 10);
        memset(buf, '#', sizeof(buf));
        putstr(buf, sizeof(buf));
        putchar('\n');

        sprintf(buf, "foo%s%u", "bar", 777);
        printf("L1: [%s]\n", buf);
        assert_eq(buf[sizeof(buf) - 1], 0);
        assert_eq(strcmp(buf, "foobar777"), 0);

        printf("L2\n");
        sprintf(buf, "");
        printf("L3: [%u]\n", strlen(buf));
        assert_eq(strlen(buf), 0);

        // Field width > 255.
        char temp[260];
        memset(temp, 'X', 260);
        sprintf(temp, "%258s", ' ');  // overwrite 0..258; don't overwrite last 'X'
        assert(temp[258] == 0);
        assert(temp[259] == 'X');
        for (int i = 0; i < 258; ++i)
            assert(temp[i] == ' ');
        return 0;
    }
    `,
expected => "##########\nL1: [foobar777]\nL2\nL3: [0]\n"
},


{
title => q{sprintf() without printf()},
program => q`
    int main()
    {
        char buf[2];
        sprintf(buf, "$");
        // We do not use an assert macro because it calls printf().
        if (buf[0] != '$')
            putstr("ERR""OR: sprintf() did not write '$' in buf[].\n", 45);  // error keyword cut to avoid false negative in smoke test
        return 0;
    }
    `,
expected => ""
},


{
title => q{Size of pointed type in the case of multi-dim array},
program => q`
    // Size of pointed type, in the case of multi-dim array,
    // is the product of array dimensions, ignoring the first dimension.

    int a[5][7][13];  // pointed type here is int[7][13], i.e., a[i] designates 91 ints
    unsigned char e[3][7];
    int main()
    {
        int *p0 = (int *) a[0];
        int *p1 = (int *) a[1];
        assert_eq(p1 - p0, 7 * 13);
        
        char c = 34;
        unsigned char *src = (unsigned char *) e + (c - ' ') * 7;
        assert_eq(src - (unsigned char *) e, 14);
        
        return 0;
    }
    `,
expected => ""
},


{
title => q{Optimization of assignments of zero},
program => q`
    int main()
    {
        unsigned char uc;
        uc = 0;
        assert_eq(uc, 0);
        uc = 42;
        assert_eq(uc, 42);
        unsigned u;
        u = 0;
        assert_eq(u, 0);
        u = 4242;
        assert_eq(u, 4242);
        
        // Signed cases.
        char b;
        b = 0;
        assert_eq(b, 0);
        b = -42;
        assert_eq(b, -42);
        int i;
        i = 0;
        assert_eq(i, 0);
        i = -42;
        assert_eq(i, -42);
        i = -4242;
        assert_eq(i, -4242);
        return 0;
    }
    `,
expected => ""
},


{
title => q{Multiplication},
program => q`
    char fc(char c) { return c; }
    int fi(int n) { return n; }
    int main()
    {
        {
        byte b0 = 3;
        byte b1 = 5;
        byte b2 = b0 * b1;
        assert_eq(b2, 15);
        b2 = b0 * 6;
        assert_eq(b2, 18);
        b2 = 4 * b1;
        assert_eq(b2, 20);

        // Check that other binary operators accept an int constant and still return a byte.
        b2 = b0 / 6;
        b2 = b0 % 6;
        b2 = b0 + 6;
        b2 = b0 - 6;
        b2 = 6 / b0;
        b2 = 6 % b0;
        b2 = 6 + b0;
        b2 = 6 - b0;
        
        b0 = 10;
        assert_eq(b0 * 160, 64);  // mul done in 8 bits
        assert_eq((word) b0 * 160, 1600);
        assert_eq(160 * (word) b0, 1600);
        
        byte b = 3;
        assert_eq(b * 2, 6);
        assert_eq(b * 4, 12);
        assert_eq(b * 8, 24);
        assert_eq(b * 16, 48);
        assert_eq(b * 32, 96);
        assert_eq(b * 64, 192);
        assert_eq(2 * b, 6);
        assert_eq(4 * b, 12);
        assert_eq(8 * b, 24);
        assert_eq(16 * b, 48);
        assert_eq(32 * b, 96);
        assert_eq(64 * b, 192);
        
        // Special cases:
        assert_eq(b * 0, 0);
        assert_eq(0 * b, 0);
        assert_eq(b * 1, 3);
        assert_eq(1 * b, 3);
        
        // Mult. by shifting:
        assert_eq(b * 2, 6);
        assert_eq(2 * b, 6);
        assert_eq(b * 4, 12);
        assert_eq(4 * b, 12);
        assert_eq(b * 32, 96);
        assert_eq(32 * b, 96);
        assert_eq(b * 64, 192);
        assert_eq(64 * b, 192);
        }

        {
        word w0 = 250;
        word w1 = 251;
        word w2 = w0 * w1;
        assert_eq(w2, 62750);
        w2 = w0 * 252;
        assert_eq(w2, 63000);
        w2 = 249 * w1;
        assert_eq(w2, 62499);
        }
        
        // Signed cases.
        {
        char c0 = -3;
        char c1 = 5;
        char c2 = c0 * c1;
        assert_eq(c2, -15);
        c2 = c0 * 6;
        assert_eq(c2, -18);
        c2 = -4 * c1;
        assert_eq(c2, -20);
        c2 = c1 * -8;
        assert_eq(c2, -40);
        c2 = -4 * 5;
        assert_eq(c2, -20);
        c2 = 5 * -8;
        assert_eq(c2, -40);
        c2 = -6 * -8;
        assert_eq(c2, +48);
        c2 = -4 * c0;
        assert_eq(c2, 12);
        
        assert_eq(fc(-3) * fc(5), -15);
        assert_eq(fc(-3) * fc(6), -18);
        assert_eq(fc(-4) * fc(5), -20);
        assert_eq(fc(-4) * fc(-3), 12);
        
        c2 = -50 / c1;
        assert_eq(c2, -10); 
        }

        {
        int i0 = -300;
        int i1 = 100;
        int i2 = i0 * i1;
        assert_eq(i2, -30000);
        i2 = i0 * 99;
        assert_eq(i2, -29700);
        i2 = i0 * -99;
        assert_eq(i2, 29700);
        i2 = -299 * i1;
        assert_eq(i2, -29900);
                
        assert_eq(fi(-300) * fi(100), -30000);
        assert_eq(fi(-300) * fi(99),  -29700);
        assert_eq(fi(-299) * fi(100), -29900);
        assert_eq(fi(-299) * fi(-100), 29900);
        }
        
        return 0;
    }
    `,
expected => ""
},


{
title => q{Assembly-only function},
program => q`
    int global0 = 42;
    int asm f(int m, int n)  // no stack frame because of 'asm' modifier
    {
        // U not pushed, so 1st arg is at 2,s
        asm {
            ldd 2,s     // load m
            addd 4,s    // add n, leave sum in D
        }
        asm {       // more than one asm{} statement allowed, although not useful
            nop
        }
    }
    unsigned char asm g(int m, int n)
    {
        asm {
            ldd 2,s
            addd 4,s
            inca        // function returns byte, so trashing A must not matter
        }
    }
    int asm h()
    {
        asm {
            ldd global0
        }
    }
    int asm q()
    {
        asm("ldd", global0);
    }
    int main()
    {
        assert_eq(f(3000, 1299), 4299);
        assert_eq(g(3000, 1299), 4299 % 256);
        assert_eq(4299 % 256, 203);
        assert_eq(h(), 42);
        assert_eq(q(), 42);
        return 0;
    }
    `,
expected => ""
},


{
title => q{Subtraction without spaces surrounding the minus sign},
program => q`
    int main()
    {
        int k = 5;
        int n = k-4;  // k-4 must be seen as tokens 'k', '-', '4', not, 'k' and '-4'
        assert_eq(n, 1);
        assert_eq(99-88, 11);
        return 0;
    }
    `,
expected => ""
},


{
title => q{Draw lines of pixels in two diagonals across a 640x192 screen},
program => q`
    int main()
    {
        word xx;
        xx = 328;
        assert_eq(xx * 100 / 333, (word) 98);
        for(xx=0; xx<640; xx++) {
          word y0 = ((xx+1)*100)/333 - 1;
          word y1 = 191 - (((xx+1)*100)/333 - 1);
          //printf("%5u %5u %5u\n", xx, y0, y1);
          assert_range((int) y0, -1, 191);
          assert_range(y1, 0, 192); 
        }
        return 0;
    }
    `,
expected => ""
},


{
title => q{No sign extension when passing argument of form unsigned-byte-expr & 8-bit-const},
program => q`
    // Correctly determine signedness of unsigned-byte-expression & 8-bit-constant,
    // so that no sign extension is done on argument passing.

    unsigned f(unsigned char b)
    {
        unsigned receivedWord;
        asm {
            ldd 4,u
            std receivedWord
        }
        //printf("receivedWord=$%04X\n", receivedWord);
        assert_eq(receivedWord & 0xFF00, 0);  // no sign extension expected when passing argument to f()
        assert_eq(b, 0x80);
        return b;
    } 
    int main()
    {
        unsigned w0 = 0x8A;
        unsigned w1 = f((byte) (w0 & 0x80));  // 2-byte bitwise AND, result cast to byte
        assert_eq(w1, 0x80);

        // Operands of & are byte and word, but 0x80 fits in a byte, so is seen as byte.
        // Sign of result of & is unsigned because operands have different signedness
        // (unsigned on left, signed int on right).
        unsigned w2 = f(((byte) w0) & 0x80);
        assert_eq(w2, 0x80);
        return 0;
    }
    `,
expected => ""
},


{
title => q{Product of small positive integers can give negative 16-bit result},
program => q`
    int main()
    {
        assert_eq(320 * 200, 0xFA00);
        assert_eq(320 * 200, -1536);  // mul is signed because operands are both signed
        assert_eq(320 * 200 / 2, 0xFD00);  // div is signed, so 0xFA00 gets ASR
        assert_eq(320 * 200 / 2, -768);
        assert_eq((unsigned) 320 * 200, 64000);
        assert_eq((unsigned) 320 * 200 / 2, 32000);
        return 0;
    }
    `,
expected => ""
},


{
noPreamble => 1,  # do not include cmoc.h
title => q{Redefining a function provided by the system library},
program => q`
    // cmoc.h not included, so we need to declare putstr() here.
    // Note unsigned instead of size_t, just for giggles.
    void putstr(const char *s, unsigned n);
    void putchar(int i)
    {
        char a[] = { '[', (char) i, ']', '\n' };
        putstr(a, 4);
    }
    void rand(int a, int b)  // different arguments than the normal rand(int)
    {
        putstr("rand(int, int)\n", 15);
    }
    // Note that in linker mode, this works only because rand(), etc.,
    // are in the same module as main(). 
    int main()
    {
        putchar('C');
        rand(8, 9);
        return 0;
    }
    `,
expected => "[C]\nrand(int, int)\n"
},


{
title => q{Redefining a system function while using the system's prototype},
program => q`
    // Note that in linker mode, this works only because rand()
    // is in the same module as main(). 
    char g = 0;
    int main()
    {
        assert_eq(g, 0);
        assert_eq(rand(), 1000);
        assert_eq(g, 42);
        return 0;
    }
    int rand() { g = 42; return 1000; }
    `,
expected => ""
},


{
title => q{Signed multiplication, division, modulo},
program => q`
    int multiply(int dividend, int divisor)
    {
        return dividend * divisor;
    }
    int divide(int dividend, int divisor)
    {
        return dividend / divisor;
    }
    int modulo(int dividend, int divisor)
    {
        return dividend % divisor;
    }
    char multiplyc(char dividend, char divisor)
    {
        return dividend * divisor;
    }
    char dividec(char dividend, char divisor)
    {
        return dividend / divisor;
    }
    char moduloc(char dividend, char divisor)
    {
        return dividend % divisor;
    }
    byte multiplyuc(byte dividend, byte divisor)
    {
        return dividend * divisor;
    }
    byte divideuc(byte dividend, byte divisor)
    {
        return dividend / divisor;
    }
    byte modulouc(byte dividend, byte divisor)
    {
        return dividend % divisor;
    }
    int main()
    {
        // Test operations with non-constant operands, via function.
        assert_eq(multiply(+1000, +25), +25000);
        assert_eq(multiply(-1000, +25), -25000);
        assert_eq(multiply(+1000, -25), -25000);
        assert_eq(multiply(-1000, -25), +25000);

        assert_eq(divide(+1000, +25), +40);
        assert_eq(divide(-1000, +25), -40);
        assert_eq(divide(+1000, -25), -40);
        assert_eq(divide(-1000, -25), +40);

        assert_eq(modulo(+1000, +33), +10);  // 1000 - 10 == 990, which is divisible by 33
        assert_eq(modulo(-1000, +33), -10);  // -1000 - (-10) == -990, which is divisible by 33
        assert_eq(modulo(+1000, -33), +10);  // negative divisor gives same modulo as positive one
        assert_eq(modulo(-1000, -33), -10);

        // Test operations with constant operands (may get optimized).
        assert_eq(+1000 * +25, +25000);
        assert_eq(-1000 * +25, -25000);
        assert_eq(+1000 * -25, -25000);
        assert_eq(-1000 * -25, +25000);

        assert_eq(+1000 / +25, +40);
        assert_eq(-1000 / +25, -40);
        assert_eq(+1000 / -25, -40);
        assert_eq(-1000 / -25, +40);

        assert_eq(+1000 % +33, +10);
        assert_eq(-1000 % +33, -10);
        assert_eq(+1000 % -33, +10);
        assert_eq(-1000 % -33, -10);

        // Test signed char operations with non-constant operands, via function.
        assert_eq(multiplyc(+10, +5), +50);
        assert_eq(multiplyc(-10, +5), -50);
        assert_eq(multiplyc(+10, -5), -50);
        assert_eq(multiplyc(-10, -5), +50);

        assert_eq(dividec(+100, +25), +4);
        assert_eq(dividec(-100, +25), -4);
        assert_eq(dividec(+100, -25), -4);
        assert_eq(dividec(-100, -25), +4);

        assert_eq(moduloc(+100, +30), +10);  // 100 - 10 == 90, which is divisible by 30
        assert_eq(moduloc(-100, +30), -10);  // -100 - (-10) == -90, which is divisible by 30
        assert_eq(moduloc(+100, -30), +10);  // negative divisor gives same modulo as positive one
        assert_eq(moduloc(-100, -30), -10);

        // Test unsigned char operations with non-constant operands, via function.
        assert_eq(multiplyuc(+10, +25), +250);
        assert_eq(divideuc(+200, +25), +8);
        assert_eq(modulouc(+200, +30), +20);  // 200 - 20 == 180, which is divisible by 30
        
        // Special cases of byte division.
        unsigned char uc = 250;
        assert_eq(uc / (byte) 1, 250);
        assert_eq(uc / (byte) 2, 125);
        assert_eq(uc / (byte) 4, 62);
        assert_eq(uc / (byte) 64, 3);
        assert_eq(uc / (byte) 128, 1);
        char sc = 125;
        assert_eq(sc / (char) 1, 125);
        assert_eq(sc / (char) 2, 62);
        assert_eq(sc / (char) 4, 31);
        assert_eq(sc / (char) 32, 3);
        sc = -125;
        assert_eq(sc / (char) 1, -125);
        assert_eq(sc / (char) 2, -62);
        assert_eq(sc / (char) 4, -31);
        assert_eq(sc / (char) 32, -3);
        
        // Special cases of byte modulo.
        assert_eq(uc % (byte) 1, 0);
        assert_eq(uc % (byte) 2, 0);
        assert_eq(uc % (byte) 4, 2);
        assert_eq(uc % (byte) 64, 58);
        
        // Division of unsigned byte by 7 (LBSR DIV8BY7).
        byte dividend = 0;
        for (byte quotient = 0; quotient <= 36; ++quotient)
        {
            for (byte remainder = 0; remainder <= 6; ++remainder, ++dividend)
            {
                //printf("%3u / 7 = %2u remainder %u\n", dividend, quotient, remainder);
                byte computedQuotient = dividend / 7;
                assert_eq(computedQuotient, quotient);
                byte computedRemainer = dividend % 7;
                assert_eq(computedRemainer, remainder);
                if (dividend == 255)
                    break;
            }
            if (dividend == 255)
                break;
        }
        
        // Division of unsigned word by 10 (LBSR DIV16BY10).
        {
            word quotient = 0;
            byte counter = 0;
            for (word dividend = 0; ; ++dividend)
            {
                word computedQuotient = dividend / 10;
                //printf("%5u / 10 = %5u\n", dividend, computedQuotient);
                assert_eq(computedQuotient, quotient);
                if (++counter == 10)
                {
                    ++quotient;
                    counter = 0;
                }
                if (dividend == 0xFFFF)
                    break;
            }
            
            assert_eq(56789u / 10u, 5678); 
            assert_eq(56789 / 10u, 5678); 
            assert_eq(56789u / 10, 5678); 
            assert_eq(56789 / 10, 5678); 
        }

        // Division of unsigned byte by 10.
        {
            byte quotient = 0;
            byte counter = 0;
            for (byte dividend = 0; ; ++dividend)
            {
                byte computedQuotient = dividend / 10;
                //printf("%5u / 10 = %5u\n", dividend, computedQuotient);
                assert_eq(computedQuotient, quotient);
                if (++counter == 10)
                {
                    ++quotient;
                    counter = 0;
                }
                if (dividend == 0xFF)
                    break;
            }
        }

        // Division of signed word by 10 (does not use DIV16BY10).
        {
            signed quotient = -3276;
            byte counter = 1;
            for (signed dividend = -32768; ; ++dividend)
            {
                signed computedQuotient = dividend / 10;
                //printf("%5d / 10 = %5d\n", dividend, computedQuotient);
                assert_eq(computedQuotient, quotient);
                if (++counter == 10)
                {
                    if (dividend == 0)
                        counter = 1;
                    else
                    {
                        ++quotient;
                        counter = 0;
                    }
                 }
                if (dividend == 32767)
                    break;
            }
        }

        // Division of signed byte by 10.
        {
            char quotient = -12;
            byte counter = 1;
            for (char dividend = -128; ; ++dividend)
            {
                char computedQuotient = dividend / 10;
                //printf("%5d / 10 = %5d\n", dividend, computedQuotient);
                assert_eq(computedQuotient, quotient);
                if (++counter == 10)
                {
                    if (dividend == 0)
                        counter = 1;
                    else
                    {
                        ++quotient;
                        counter = 0;
                    }
                }
                if (dividend == 127)
                    break;
            }
        }
        
        // Check optimization of non-var-non-const left side by 8.
        byte b = 42;
        byte n = (b + 7) / 8;
        assert_eq(n, 6);
        
        // Check optimization of word by power of 2.
        word w = 42;
        assert_eq(w / 2, 21);
        w = 12800;
        assert_eq(w / 128, 100);
        assert_eq(w / 256, 50);  // special case: TFR A,B; CLRA
        w = 0xAAAA;
        assert_eq(w / 4096, 0x000A);
        
        // Mixed byte/word case.
        b = 2;
        assert_eq(w / b, 0x5555);
        b = 200;
        w = 8;
        assert_eq(b / w, 25);
        
        // Modulo.
        byte card = 135;
        byte v = (card - 1) % 13;
        assert_eq(v, 4);
        
        // Signed division by power of 2.
        int i = 400;
        int j = i / 2;
        assert_eq(j, 200);
        
        // Signed byte, 0..127.
        char c;
        for (c = 0; c >= 0; ++c)
        {
            char computed = c / 2;
            char expected = c >> 1;
            assert_eq(computed, expected);
        }
        // Signed byte, -1..-128.
        for (c = -1; c != -128; --c)
        {
            char computed = c / 2;
            char expected = -((-c) >> 1);
            //printf("%d %d %d\n", c, computed, expected);
            assert_eq(computed, expected); 
        }
        assert_eq(c / 2, -64);

        unsigned u0 = 0, u1 = 5;
        assert_eq(u0 / u1, 0); 
        assert_eq(u1 / u0, 0xFFFF);  // division by zero does not hang

        return 0;
    }
    `,
expected => ""
},


{
title => q{Uninitialized globals are grouped together},
program => q`
    int a[1];
    int b[1] = { 42 };  // we don't want 'b' to be emitted between 'a' and 'c'
    int c[1];
    int *min(int *x, int *y) { return x < y ? x : y; }
    int *max(int *x, int *y) { return x > y ? x : y; }
    int main()
    {
        // Don't assume that 'a' is emitted before 'c'.
        int *firstUninit  = min(a, c);
        int *secondUninit = max(a, c);

        // If the two uninitialized variables are grouped together,
        // that 'b' cannot be between them, i.e., 'b' had to be before
        // the first or after the second.
        assert(b < firstUninit || secondUninit < b);
        return 0;
    }
    `,
expected => ""
},


{
title => q{pushLoadDiscardAdd optimization (see ASMText.cpp)},
program => q`
    byte t() { return 1; }
    byte f() { return 0; } 
    int main()
    {
        byte isUsersTurn = t();
        byte row = 30 + (isUsersTurn ? +24 : -24);  // optimization on this line
        assert_eq(row, 54);
        isUsersTurn = f();
        row = 30 + (isUsersTurn ? +24 : -24);  // optimization on this line
        assert_eq(row, 6);
        return 0;
    }
    `,
expected => ""
},


{
title => q{pushLoadDLoadX optimization},
program => q`
    byte f() { return 42; }
    int main()
    {
        byte b = f();
        word w = (word) b * 32;  // cast to ensure 16-bit multiplication
        assert_eq(w, 1344);
        word k = 10;
        w = (word) b * k;
        assert_eq(w, 420);
        return 0;
    }
    `,
expected => ""
},


{
title => q{Statement in compound statement with constant cast to void generates no code},
program => q`
    byte t() { return 1; }
    int main()
    {
        byte *bb, *aa;
        asm {
before:
        }
        ((void) (1 + 2));
        asm {
after:
            leax    before,pcr
            stx     bb
            leax    after,pcr
            stx     aa
        }
        assert_eq(bb, aa);
        return 0;
    }
    `,
expected => ""
},


{
title => q{Optimization of complex boolean expressions},
program => q`
    byte p1;
    byte b1;
    byte pend;
    byte bend;
    int f()
    {
        if (((p1 <= b1) && (pend >= b1))
            || ((b1 <= p1) && (bend >= p1)))
            return 11;
        return -11;
    }
    int main()
    {
        p1 = 10; b1 = 15; pend = 12; bend = 99;
        assert_eq(f(), -11);  // pend >= b1 fails
        p1 = 17;
        assert_eq(f(), 11);
        bend = 3;
        assert_eq(f(), -11);  // p1 <= b1 fails then bend >= p1 fails
        
        byte r = 0;
        //printf("%u %u %u %u\n", p1, b1, bend, pend);
        if (! (p1 < b1 || bend < pend))
            r = -1;
        else
            r = 1;
        assert_eq(r, 1);

        return 0;
    }
    `,
expected => ""
},


{
title => q{Union},
program => q`
    struct S
    {
        char c;
        int i;
    };
    union Word
    {
        unsigned w;
        unsigned char b[2];
        struct S s;
    };
    int main()
    {
        union Word u;
        u.w = 0x1234;
        assert_eq(u.b[0], 0x12);
        assert_eq(u.b[1], 0x34);
        u.b[0] = 0x56;
        assert_eq(u.w, 0x5634);
        u.b[1] = 0x78;
        assert_eq(u.w, 0x5678);
        memcpy(u.b, "AB", 2);
        assert_eq(u.w, 0x4142);
        assert_eq(&u, &u.w); 
        assert_eq(&u, &u.b);
        assert_eq(&u, &u.s);
        assert_eq(&u, &u.s.c);
        assert_eq((int *) ((unsigned) &u + 1), &u.s.i);
        
        assert_eq(u.w >> 8, (unsigned) u.s.c);
        
        u.s.c = 0xAB;
        u.s.i = 0xCDEF;
        assert_eq(u.b[0], 0xAB);
        assert_eq(u.b[1], 0xCD);
        assert_eq(u.b[2], 0xEF);  // reading beyond end of b[]
        assert_eq(u.w, 0xABCD);
        
        assert_eq(sizeof(union Word), 3);
        assert_eq(sizeof(u), 3);
        assert_eq(sizeof(u.w), 2);
        assert_eq(sizeof(u.b), 2);
        assert_eq(sizeof(u.s.c), 1);
        assert_eq(sizeof(u.s.i), 2);
        
        return 0;
    }
    `,
expected => ""
},


{
title => q{Offset of members of a union},
program => q!
    union U0
    {
        char a, b;
    };
    struct S0 { int s[8]; };
    union U1
    {
        char m0;
        int m1;
        void *m2;
        struct S0 m3;
        union U0 m4;
    };
    struct S1
    {
        int w;
        union U0 u0;
        union U1 u1;
    };
    int main()
    {
        assert_eq(offsetof(union U0, a), 0);
        assert_eq(offsetof(union U0, b), 0);
        assert_eq(offsetof(union U1, m0), 0);
        assert_eq(offsetof(union U1, m1), 0);
        assert_eq(offsetof(union U1, m2), 0);
        assert_eq(offsetof(union U1, m3), 0);
        assert_eq(offsetof(union U1, m4), 0);
        assert_eq(offsetof(union U1, m4.a), 0);
        assert_eq(offsetof(union U1, m4.b), 0);
        assert_eq(offsetof(struct S1, w), 0);
        assert_eq(offsetof(struct S1, u0), 2);
        assert_eq(offsetof(struct S1, u0.a), 2);
        assert_eq(offsetof(struct S1, u0.b), 2);
        assert_eq(offsetof(struct S1, u1), 3);
        assert_eq(offsetof(struct S1, u1.m3), 3);
        assert_eq(offsetof(struct S1, u1.m4), 3);
        return 0;
    }
    !,
expected => "",
},



{
title => q{Branch optimizations vs. inline assembly},
program => q`
    char f(int v)
    {
        char ret;
        if (v)
        {
            asm
            {
                ldb #1
                stb ret
            }
        }
        else  // optimizer must not remove branch over else clause
        {
            asm
            {
                clr ret
            }
        }
        return ret;
    }
    int main()
    {
        assert_eq(f(0), 0);
        assert_eq(f(5), 1);
        return 0;
    }
    `,
expected => ""
},


{
title => q{Avoid optimizing an add just because FFxx constant is a negative that fits a byte},
program => q`
    int main()
    {
        byte slot = 12;
        word p;
        p = 0xffb0 + slot;  // 16-bit operation because 0xffb0 is unsigned, there does not fit a byte
        assert_eq(p, 0xffbc);
        p = 65456 + slot;  // same in decimal
        assert_eq(p, 0xffbc);
        p = -80 + slot;  // 8-bit operation: -80 is represented internally as 0xffb0, but is signed, so fits a byte
        assert_eq(p, -68);
        return 0;
    }
    `,
expected => ""
},


{
title => q{Character array in const_data section initialized with a string literal},
suspended => 1,
compilerOptions => "--org=5000 --data=4C00",
program => q`
    char writableText[] = "foo";
    
    #pragma const_data start
    char constText[] = "bar";
    #pragma const_data end
    
    const char *p = "baz";  // would not be allowed in const_data: init of 'p' would have to be at run-time
    const char *q = "bar";  // uses same literal as constText[] 
    const char *r = "foo";  // uses same literal as writableText[]
    int main()
    {
        printf("writableText=[%s]\n", writableText);
        printf("constText=[%s]\n", constText);
        assert_eq(writableText[0], 'f');
        assert_eq(constText[2], 'r');
        assert_eq(p[2], 'z');
        assert_eq(strcmp(constText, "bar"), 0);

        writableText[1] = 'X';  // modifies writableText's copy of "foo"; does not affect r
        printf("writableText=[%s]\n", writableText);
        assert_eq(strcmp(writableText, "fXo"), 0);
        assert_eq(strcmp(r, "foo"), 0);
        assert_eq(strcmp(q, "bar"), 0);

        *((char *) q + 2) = 'Y';  // does not affect constText[], which has its own copy of "bar"
        assert_eq(strcmp(q, "baY"), 0);

        // Test constText is not affected.
        // Do not test with string literal "bar", which is currently corrupted.
        // (Modifying a string literal is generally a bad practice.)
        //
        //assert_eq(strcmp(constText, "bar"), 0);
        printf("constText=[%s]\n", constText);
        assert_eq(constText[0], 'b');
        assert_eq(constText[1], 'a');
        assert_eq(constText[2], 'r');
        assert_eq(constText[3], '\0');

        *((char *) q + 2) = 'r';  // restore literal "bar" (modifying a string literal is generally a bad practice)
        printf("bar");  // test the restored literal
        putchar('\n');
        
        const char *local = "quux";
        printf("local=[%s]\n", local);
        local = 0x1234;
        assert_eq(local, 0x1234);

        return 0;
    }
    `,
expected => "writableText=[foo]\nconstText=[bar]\nwritableText=[fXo]\nconstText=[bar]\nbar\nlocal=[quux]\n"
},


{
title => q{Promotion to int for comparisons},
program => q`
    int s1(char *e)
    {
        if (*e == (char) 0xFF)  // *e compared with -1
            return 1;
        return 0;
    }
    int s2(char *e)
    {
        if (*e == 0xFFFF)  // *e sign-extended, then compared to 0xFFFF
            return 1;
        return 0;
    }
    int u0(unsigned char *e)
    {
        if (*e == 0xFF)
            return 1;
        return 0;
    }
    int u1(unsigned char *e)
    {
        if (*e == (char) 0xFF)  // *e is zero-extended, then compared with -1, i.e., 0xFFFF
            return 1;
        return 0;
    }
    int u2(unsigned char *e)
    {
        if (*e == 0xFFFF)  // *e zero-extended, then compared to 0xFFFF
            return 1;
        return 0;
    }
    int main()
    {
        char c = (char) 0xFF;
        assert_eq(s1(&c), 1);
        assert_eq(s2(&c), 1);
        unsigned char uc = 0xFF;
        assert_eq(u0(&uc), 1);
        assert_eq(u1(&uc), 0);
        assert_eq(u2(&uc), 0);
        return 0;
    }
    `,
expected => ""
},


{
title => q{const_data initializer with non-trivial constant expression},
suspended => 1,
program => q`
    #pragma const_data start
    int a0[] = { 0, -1, 2 + 2, -(10 / 2) * 3 };
    char n = -42;
    #pragma const_data end
    int main()
    {
        assert_eq(a0[0], 0);
        assert_eq(a0[1], -1);
        assert_eq(a0[2], 4);
        assert_eq(a0[3], -15);
        assert_eq(sizeof(a0), 8);
        assert_eq(n, -42);
        return 0;
    }
    `,
expected => ""
},


{
title => q{Calling a C function from inline assembly},
program => q`
    char cFuncCalled = 0;
    void cFunc()
    {
        ++cFuncCalled;
    }
    byte counter = 0;
    asm byte condBranchToStart()
    {
        asm {
            inc counter
            ldb counter
            cmpb #5
            blo condBranchToStart
        }
    }
    int main()
    {
        asm {
            jsr     cFunc
            lbsr    cFunc
            leax    cFunc
            jsr     ,x
            
            // The next references to cFunc() don't call it, but we still test
            // that the instructions are emitted correctly.
            brn     cFunc
            lbrn    cFunc
            bra     avoid
            bra     cFunc
            lbra    cFunc
avoid:
        }
        assert_eq(cFuncCalled, 3);
        assert_eq(condBranchToStart(), 5);
        return 0;
    }
    `,
expected => ""
},


{
title => q{Emitting code for functon only called by LBSR in asm-only function},
program => q`
    char v = 99;
    void g() { v = 42; }
    void f() { g(); }
    void asm asmOnly()
    {
        asm {
            lbsr    f
        }
    }
    int main()
    { 
        asmOnly();
        assert_eq(v, 42);
        return 0;
    }
    `,
expected => ""
},


{
title => q{Emitting code for functon only called by LBSR in non-asm-only function},
program => q`
    char v = 99;
    void g() { v = 42; }
    void f() { g(); }
    void notAsmOnly()
    {
        asm {
            lbsr    f
        }
    }
    int main()
    { 
        notAsmOnly();
        assert_eq(v, 42);
        return 0;
    }
    `,
expected => ""
},


{
title => q{ASMText::optimize16BitStackOps1},
program => q`
    struct FileDesc
    {
        byte drive;
        byte firstGran;  // 0..67
        word numBytesLastSector;  // 0..256
        word length[2];  // 32-bit word giving length of file
        byte curGran;  // 0..67, 255 means at EOF
        byte curSec;  // 1..9 (relative to current granule)
        word curGranLen;  // 0..2304
        word offset[2];  // 32-bit reading/writing offset
        word secOffset;  // 0..256: index into curSector[] (256 means beyond sector)
        byte curSector[256];
        word curSectorAvailBytes;  // number valid bytes in curSector[] (0..256)
    };
    
    int main()
    {
        word offsetInLastGranule = 0x5678;
        struct FileDesc object;
        struct FileDesc *fd = &object;
        fd->secOffset = offsetInLastGranule & 0xFF;  // statement targeted by ASMText::optimize16BitStackOps1()
        assert_eq(fd->secOffset, 0x78);
        return 0;
    }
    `,
expected => ""
},


{
title => q{ASMText::optimizeIndexedX},
program => q`
    struct Parser
    {
        const char *input;
    };
    char buf[16];
    byte iter = 0;
    void f(struct Parser *parser)
    {
        byte index = 0;  // index in buf[]
        for (;;++iter)  // accumulate non-NUL chars in buf[]
        {
            byte curChar = *parser->input;
            if (curChar)
            {
                if (index < sizeof(buf) - 1)  // if room in buf[]
                    buf[index++] = curChar;  // accept char
                ++parser->input;  // pass that char
            }
            else
                break;  // NUL seen: end of accumulation
            if (iter == 10)  // in case of infinite loop (3 iterations expected)
                break;
        }
        assert_eq(index, 3);
        assert_eq(buf[0], 'A');
        assert_eq(buf[1], 'B');
        assert_eq(buf[2], 'C');
    }
    int main()
    {
        struct Parser parser;
        const char *text = "ABC";
        parser.input = text;
        f(&parser);
        assert_eq(parser.input, text + 3); 
        return 0;
    }
    `,
expected => ""
},


{
title => q{Function pointer in member initializer of global struct instance},
program => q`
    struct S { int (*fp)(); };
    byte n = 40;
    int func() { return ++n; }
    struct S g = { func };
    int main()
    {
        assert_eq((*g.fp)(), 41);
        assert_eq(g.fp(),    42);

        struct S loc = { func };
        assert_eq((*loc.fp)(), 43);
        assert_eq(loc.fp(),    44);
        return 0;
    }
    `,
expected => ""
},


{
title => q{Adding to a function pointer},
program => q`
    byte n = 50;
    void f()
    {
        n += 10;
    }
    void call0(void (*fp)())
    {
        void (*fixed)() = fp - 42;
        (*fixed)();
    }
    void call1(void (*fp)())
    {
        void (*fixed)() = fp + 42;
        (*fixed)();
    }
    int main()
    {
        call0(f + 42);
        assert_eq(n, 60);
        call0(42 + f);
        assert_eq(n, 70);
        call1(f - 42);
        assert_eq(n, 80);
        call1(-42 + f);
        assert_eq(n, 90);
        return 0;
    }
    `,
expected => ""
},



{
title => q{Ternary expression vs. optimizer vs. LEAX},
program => q`
    int main()
    {
        asm { ldx #0 }  // robustness check
        word a = 4;
        word b = 5;
        word c = 1;
        (c ? a : b) = 6;
        assert_eq(a, 6);
        return 0;
    }
    `,
expected => ""
},


{
title => q{optimizeStackOperations1},
program => q`
    struct HeapBlock {
      struct HeapBlock *prev;
      unsigned size;
      unsigned char b;
      char free;
    };
    
    int f() { return 1; }  // must be 1 so that if() succeeds in free()
    
    void free(struct HeapBlock *ptr) {
      struct HeapBlock *block = ptr;
      struct HeapBlock *next = ptr;
      char nextIsFree = (char) f();
      if (f()) {
        block->size += nextIsFree ? next->size : (unsigned) 0;  // problematic line
      } else {  // else clause needed
        f();
      }
    }
    
    void free8(struct HeapBlock *ptr) {
      struct HeapBlock *block = ptr;
      struct HeapBlock *next = ptr;
      char nextIsFree = (char) f();
      if (f()) {
        block->b += nextIsFree ? next->b : (unsigned char) 0;
      } else {  // else clause needed
        f();
      }
    }
    
    int main() {
        struct HeapBlock hb = { 0, 1000, 77, 42 };
        assert_eq(hb.size, 1000);
        free(&hb);  // expected to add 'size' field to itself
        assert_eq(hb.size, 2000);
        
        assert_eq(hb.b, 77);
        free8(&hb);
        assert_eq(hb.b, 154);
        
        return 0;
    }
    `,
expected => ""
},


{
title => q{Local asm block labels},
program => q`
    asm unsigned f1()
    {
        asm {
@foo
        leax    @foo,pcr
        tfr     x,d
        }
    }
    asm unsigned f2()
    {
        asm {
@foo
        leax    @foo,pcr
        tfr     x,d
        }
    }
    asm void *f3()
    {
    asm {
@f1
        leax    @f1,pcr
        tfr     x,d
        }
    }
    asm void f4()
    {
        asm {
        leax    f1      // take address of function f1()
        tfr     x,d
        }
    }
    int main()
    {
        //printf("%p %p %p\n", f1(), f2(), f3());
        assert_ne(f1(), f2());
        assert_ne(f3(), f1);  // label f1 in f3() must not refer to function f1()

        f4();
        void *retval;
        asm { std :retval }
        assert_eq(retval, f1);

        return 0;
    }
    `,
expected => ""
},


{
title => q{optimize16BitStackOps1 and optimize8BitStackOps},
program => q`
    void f() {}
    char c;
    char *left = &c;
    char store99()
    {
        int x, seg = 0;
    
        f();  // this separates basic blocks
        x = 99;  // value to be put in left[0]
        f();  // this separates basic blocks
    
        // Problematic statement: the right side of this assignment
        // loaded variable x with LDB using the address of the high byte
        // instead of the low byte.
        //
        left[seg] = (char) x;
    
        return 0;
    }
    int main()
    {
        store99();
        //printf("%d\n", left[0]);
        assert_eq(left[0], 99);
        return 0;
    }
    `,
expected => ""
},


{
title => q{2016-06-08 optimization fixes},
program => q`
    struct Vect {
      int x;
    };
    
    struct BigVect {
      struct Vect x;
    };
    
    void test1() {
      int vals[10];
      int ii;
      unsigned char bvals[10];
      for(ii=0; ii<sizeof(vals)/sizeof(vals[0]); ii++) {
        vals[ii] = 0;
        bvals[ii] = 0;
      }  
      vals[5] = 1;
    
      for(ii=0; ii<sizeof(vals)/sizeof(vals[0]); ii++) {
        if (vals[ii] == 1)
          break;
      }
      bvals[ii] = (unsigned char)ii;
      assert_eq(bvals[ii], ii);
    }
    
    void test2() {
      int vals[10];
      for(int ii=0; ii<sizeof(vals)/sizeof(vals[0]); ii++) {
        vals[ii] = (ii == 2 || ii == 5) ? 10 : ii;
      }  
    
      assert_eq(vals[2], 10);
      assert_eq(vals[5], 10);
    }
    
    void test3() {
      byte zero = 0;
      byte shift_amount = 1 + *(&zero);
      byte val = ~((byte)(0xff >> shift_amount) & (byte)(0xff << shift_amount));
    
      assert_eq(val, 0x81);
    }
    
    int main() {
      test1();
      test2();
      test3();
      return 0;
    }
    `,
expected => ""
},


{
title => q{No-argument function declared with (void)},
program => q`
    int noArg(void)
    {
        return 42;
    }
    int main(void)
    {
        assert_eq(noArg(), 42);
        return 0;
    }
    `,
expected => ""
},


{
title => q{Function pointer syntax},
program => q`
    char counter;
    void unnamedArgFunc(int)
    {
        ++counter;
    }
    void unnamedPointerArgFunc(int *)
    {
        ++counter;
    }
    void unnamedArrayArgFunc(int [])
    {
        ++counter;
    }
    void unnamed2DArrayArgFunc(int [][5])
    {
        ++counter;
    }
    void unnamedFuncPtrArgFunc(int (*)())
    {
        ++counter;
    }
    struct S { int n; };
    void unnamedStructArgFunc(struct S *)
    {
        ++counter;
    }
    void unnamedStructArrayArgFunc(struct S [])
    {
        ++counter;
    }
    int mix2(int, int b)  // an unnamed arg does not prevent using a named arg
    {
        return b + 1;
    }
    int fi0()
    {
        return 42;
    }
    int fi2(int i, char c)
    {
        return i + c;
    }
    int invokeCallbackNoArgRetInt(int (*cb)())
    {
        return cb();
    }
    int invokeCallback2ArgsRetInt(int (*cb)(int, char c))
    {
        return (*cb)(1000, -5);
    }
    char fc0()
    {
        char c = '$';
        asm {
            lda #$EE    // put garbage in A
        }
        return c;  // only LDB emitted since function returns byte
    }
    long fl0() { return 1000000L; }
    int main()
    {
        int (*funcPtr0)() = fi0;
        assert_eq((*funcPtr0)(), 42);
        assert_eq(funcPtr0(), 42);
        assert_eq(invokeCallbackNoArgRetInt(fi0), 42);

        assert_eq((fi0)(), 42);  // ordinary call, but with name in ()

        int (*funcPtr2)(int i, char) = fi2;
        assert_eq((*funcPtr2)(10, 2), 12);
        assert_eq(funcPtr2(30, 7), 37);
        assert_eq(invokeCallback2ArgsRetInt(fi2), 995);
        
        char (*funcPtr0c)() = fc0;
        assert_eq(sizeof(funcPtr0c()), 1);
        assert_eq(funcPtr0c(), '$');
        
        long (*funcPtr0l)() = fl0;
        assert_eq(sizeof(funcPtr0l()), 4);
        assert(funcPtr0l() == 1000000L);
        
        // Call the unnamed argument functions:
        counter = 0;
        int i = 42;
        unnamedArgFunc(i);
        unnamedPointerArgFunc(&i);
        int a[] = { 9, 8, 7 };
        unnamedArrayArgFunc(a);
        int aa[][5] = { { 5, 4, 3, 2, 1 }, { 55, 44, 33, 22, 11 } };
        unnamed2DArrayArgFunc(aa);
        unnamedFuncPtrArgFunc(fi0);
        struct S s = { 42 };
        unnamedStructArgFunc(&s);
        struct S as[2] = { { 43 }, { 44 } };
        unnamedStructArrayArgFunc(as);

        assert_eq(counter, 7);
        
        assert_eq(mix2(4, 7), 8);
        
        return 0;
    }
    `,
expected => ""
},


{
title => q{Anonymous struct},
program => q`
    struct
    {
        int n;
    } a;
    struct S
    {
        struct
        {
            char c;
        } cc;
        struct
        {
            int w[2];
        } ww;
        int m;
    };
    int main()
    {
        a.n = 42;
        assert_eq(a.n, 42);
        
        struct S s;
        s.cc.c = '#';
        s.ww.w[0] = 2000;
        s.ww.w[1] = 3000;
        s.m = 1000;
        assert_eq(s.cc.c, '#');
        assert_eq(s.m, 1000);
        assert_eq(s.ww.w[0] + s.ww.w[1], 5000);
        
        return 0;
    }
    `,
expected => ""
},


{
title => q{Register keyword ignored},
program => q`
    int main()
    {
        register int n = 1844;
        assert_eq(n, 1844);
        return 0;
    }
    `,
expected => ""
},


{
title => q{goto statement},
program => q`
    int main()
    {
        char c = 0;
    here:
        ++c;
        if (c < 5)
            goto here;
        assert_eq(c, 5);

        goto there;
        assert(0);
    there:    
        return 0;
    }
    `,
expected => ""
},


{
title => q{Extern declaration gets ignored},
program => q`
    extern int g;
    int g;  // not seen as a redeclaration
    
    typedef unsigned T1;
    typedef T1 T2;
    extern T2 t;
    T2 t;

    typedef unsigned Addr[2];
    extern Addr addr;
    Addr addr = { 0xAAAA, 0xBBBB }; 

    int main()
    {
        g = 1234;
        assert_eq(g, 1234);

        t = 5678;
        assert_eq(t, 5678);
        
        addr[0] = 0x1111;
        addr[1] = 0x2222;
        assert_eq(addr[0] + addr[1], 0x3333);
        
        return 0;
    }
    `,
expected => ""
},


{
title => q{Static keyword ignored, declaration still processed},
program => q`
    static int g;
    
    int main()
    {
        g = 1234;
        assert_eq(g, 1234);
        return 0;
    }
    `,
expected => ""
},


{
title => q{Reference to undefined struct},
program => q`
    struct S *f();
    struct T *h();  // T is never defined
    
    struct S
    {
        int n;
    };
    
    struct S g;
    
    struct S *f()
    {
        return &g;
    }
    
    struct T *h()
    {
        return 0;
    }

    int main()
    {
        struct S *p = f();
        assert_eq(p, &g);
        assert_eq(h(), 0);
        return 0;
    }
    `,
expected => ""
},


{
title => q{Struct assignment},
program => q`
    struct S
    {
        int i;
        char c;
    };
    int main()
    {
        struct S s0 = { 1000, 10 };
        struct S s1 = s0;
        assert_eq(s1.i, s0.i);
        assert_eq(s1.c, s0.c);

        struct S s2;
        s2.i = s2.c = 88;
        s2 = s0;
        assert_eq(s2.i, s0.i);
        assert_eq(s2.c, s0.c);

        s1.i = s1.c = 77;
        s2.i = s2.c = 99;
        s2 = s1 = s0;
        assert_eq(s1.i, s0.i);
        assert_eq(s1.c, s0.c);
        assert_eq(s2.i, s0.i);
        assert_eq(s2.c, s0.c);

        return 0;
    }
    `,
expected => ""
},


{
title => q{Function prototypes},
program => q`
    int f(int, int);
    int h(int someName);
    int main()
    {
        int c = f(1, 2);
        assert_eq(c, 3);
        
        assert_eq(h(41), 42);
        
        return 0;
    }
    int f(int a, int b)
    {
        return a + b;
    }
    int h(int someOtherName)
    {
        return someOtherName + 1;
    }
    `,
expected => ""
},


{
title => q{enum},
program => q`
    enum
    {
        FOO,
        BAR,
        BAZ,  // trailing comma allowed
    };
    
    enum
    {
        QUUX,
        WALDO = 1000,
        WALDO_PLUS_ONE
    } gVar0 = QUUX;
        // Here, the 3 enumerated names must be registered via TypeManager::declareEnumerationList()
        // before the "gVar0 = QUUX" parts gets parsed, so that "= QUUX" can be recognized as a
        // reference to the enumerated name, rather than a reference to an undefined variable.

    enum A { a0, a1, aBig = 2222 };
    enum A aVar0 = a1;  // declare variable of previously defined enum

    int f0(enum A aParam) { return aParam; }  // function parameter of type enum (previously defined)
    
    enum A f1() { return a1; }

    enum B { b0 = 41, b1 } bVar0 = b0, bVar1;  // named enum with enumerated names, used to declare variables
    extern enum A aExtVar0;
    extern enum { EXT0 = 55 } aExtVar0;
    
    static enum A aStaticVar0 = aBig;
    static enum { STATIC0 = 66 } aStaticVar1 = STATIC0;

    typedef enum { T0 = 77, T1 } TEnum;
    TEnum t0 = T1;
    typedef enum A EnumA;
    
    enum Indices
    {
        ZERO, ONE, TWO, THREE, SEVEN = 7, EIGHT = 8, FIFTEEN = 15
    };
    enum Bits
    {
        B0  = 1 << ZERO,
        B1  = 1 << ONE,  
        B2  = 1 << TWO,  
        B3  = 1 << THREE,  
        B7  = 1 << SEVEN,  
        B8  = 1 << EIGHT,  
        B15 = 1 << FIFTEEN
    };
    
    enum
    {
        THIRTY_TWO = 1 << 5,
        INDEPENDENT = 5,
        DEPENDENT_ON_PREVIOUS = 1 << INDEPENDENT,
    };
    
    #define STR "foobar"
    enum
    {
        SIZE_OF_STR = sizeof(STR),
        SIZE_OF_STR_PLUS_ONE = sizeof(STR) + 1
    };    

    int main()
    {
        assert_eq(FOO, 0);
        assert_eq(BAR, 1);
        assert_eq(BAZ, 2);
        assert_eq(sizeof(FOO), 2);
        
        assert_eq(aVar0, a1);
        aVar0 = aBig;
        assert_eq(aVar0, aBig);
        assert_eq(f0(aBig), aBig);
        assert_eq(f1(), a1);
        
        assert_eq(sizeof(gVar0), 2);
        assert_eq(gVar0, QUUX);
        gVar0 = WALDO;
        assert_eq(gVar0, WALDO);
        gVar0 = WALDO_PLUS_ONE;
        assert_eq(gVar0, 1001);
        
        assert_eq(bVar0, b0);
        bVar0 = b1;
        assert_eq(bVar0, b1);
        
        assert_eq(aStaticVar0, aBig);
        assert_eq(aStaticVar1, STATIC0);
        
        assert_eq(t0, 78);
        assert_eq(sizeof(t0), 2);
        assert_eq(sizeof(TEnum), 2);
        
        EnumA a = aBig;
        assert_eq(a, aBig);
        assert_eq(sizeof(EnumA), 2);
        assert_eq(sizeof(enum A), 2);

        assert_eq(B0,  0x0001);
        assert_eq(B1,  0x0002);
        assert_eq(B2,  0x0004);
        assert_eq(B3,  0x0008);
        assert_eq(B7,  0x0080);
        assert_eq(B8,  0x0100);
        assert_eq(B15, 0x8000);
        
        assert_eq(THIRTY_TWO, 32);
        assert_eq(DEPENDENT_ON_PREVIOUS, 32);
        
        // Local variable name can hide enum name.
        char *THIRTY_TWO = 0x1234;
        assert_eq(THIRTY_TWO, 0x1234);
        
        // Hide an enumerated name with a local variable.
        assert_eq(FIFTEEN, 15);
        int FIFTEEN = 999;
        assert_eq(FIFTEEN, 999);
        
        assert_eq(SIZE_OF_STR, 7);
        assert_eq(SIZE_OF_STR_PLUS_ONE, 8);
        
        unsigned k0 = 22222, k1 = 22222;
        asm {
            ldd     #WALDO
            std     k0
            ldd     #WALDO+234
            std     k1
        }
        assert_eq(k0, 1000);
        assert_eq(k1, 1234);

        return 0;
    }
    `,
expected => ""
},


{
title => q{Multiplication of members of a struct},
program => q`
    typedef struct {
        int x, y;
    } test_t;
    int test1(test_t* v)
    {
        return v->x * v->y;  // removeUselessLdx() would mishandle this
    }
    int main()
    {
        test_t a0 = { 240, 250 };
        assert_eq(test1(&a0), 60000);
        return 0;
    }
    `,
expected => ""
},


{
title => q{atoui(), atoi(), itoa(), utoa(), ltoa(), ultoa()},
linkerModeOnly => 1,
program => q`
    int main()
    {
        assert_eq(atoui("0"),       0);
        assert_eq(atoui("1"),       1);
        assert_eq(atoui("543"),     543);
        assert_eq(atoui("65535"),   65535);
        assert_eq(atoi("0"),        0);
        assert_eq(atoi("1"),        1);
        assert_eq(atoi("543"),      543);
        assert_eq(atoi("32767"),    32767);
        assert_eq(atoi("-1"),       -1);
        assert_eq(atoi("-543"),     -543);
        assert_eq(atoi("-32768"),   -32768);

        char buf[64];

        // itoa().
        assert_eq(itoa(0, buf, 10),        buf);
        assert_eq(strcmp(buf, "0"), 0);
        assert_eq(itoa(1, buf, 10),        buf);
        assert_eq(strcmp(buf, "1"), 0);
        assert_eq(itoa(543, buf, 10),      buf);
        assert_eq(strcmp(buf, "543"), 0);
        assert_eq(itoa(32767, buf, 10),    buf);
        assert_eq(strcmp(buf, "32767"), 0);
        assert_eq(itoa(-1, buf, 10),       buf);
        assert_eq(strcmp(buf, "-1"), 0);
        assert_eq(itoa(-543, buf, 10),     buf);
        assert_eq(strcmp(buf, "-543"), 0);
        assert_eq(itoa(-32768, buf, 10),   buf);
        assert_eq(strcmp(buf, "-32768"), 0);

        // utoa().
        assert_eq(utoa(0, buf, 10),        buf);
        assert_eq(strcmp(buf, "0"), 0);
        assert_eq(utoa(1, buf, 10),        buf);
        assert_eq(strcmp(buf, "1"), 0);
        assert_eq(utoa(543, buf, 10),      buf);
        assert_eq(strcmp(buf, "543"), 0);
        assert_eq(utoa(32767, buf, 10),    buf);
        assert_eq(strcmp(buf, "32767"), 0);
        assert_eq(utoa(65535, buf, 10),       buf);
        assert_eq(strcmp(buf, "65535"), 0);
        assert_eq(utoa(64993, buf, 10),     buf);
        assert_eq(strcmp(buf, "64993"), 0);
        assert_eq(utoa(32768, buf, 10),   buf);
        assert_eq(strcmp(buf, "32768"), 0);

        // ltoa().
        assert_eq(ltoa(0, buf, 10),        buf);
        assert_eq(strcmp(buf, "0"), 0);
        assert_eq(ltoa(1, buf, 10),        buf);
        assert_eq(strcmp(buf, "1"), 0);
        assert_eq(ltoa(543, buf, 10),      buf);
        assert_eq(strcmp(buf, "543"), 0);
        assert_eq(ltoa(32767, buf, 10),    buf);
        assert_eq(strcmp(buf, "32767"), 0);
        assert_eq(ltoa(-1, buf, 10),       buf);
        assert_eq(strcmp(buf, "-1"), 0);
        assert_eq(ltoa(-543, buf, 10),     buf);
        assert_eq(strcmp(buf, "-543"), 0);
        assert_eq(ltoa(-32768, buf, 10),   buf);
        assert_eq(strcmp(buf, "-32768"), 0);
        assert_eq(ltoa(65536, buf, 10),    buf);
        assert_eq(strcmp(buf, "65536"), 0);
        assert_eq(ltoa(1000000, buf, 10),    buf);
        assert_eq(strcmp(buf, "1000000"), 0);
        assert_eq(ltoa(-2023380019, buf, 10), buf);
        assert_eq(strcmp(buf, "-2023380019"), 0);
        assert_eq(ltoa(0x8765ABCD, buf, 10), buf);
        assert_eq(strcmp(buf, "-2023380019"), 0);
        assert_eq(ltoa(0xFFFFFFFF, buf, 10), buf);
        assert_eq(strcmp(buf, "-1"), 0);

        // ultoa().
        assert_eq(ultoa(0, buf, 10),        buf);
        assert_eq(strcmp(buf, "0"), 0);
        assert_eq(ultoa(1, buf, 10),        buf);
        assert_eq(strcmp(buf, "1"), 0);
        assert_eq(ultoa(543, buf, 10),      buf);
        assert_eq(strcmp(buf, "543"), 0);
        assert_eq(ultoa(32767, buf, 10),    buf);
        assert_eq(strcmp(buf, "32767"), 0);
        assert_eq(ultoa(65535, buf, 10),    buf);
        assert_eq(strcmp(buf, "65535"), 0);
        assert_eq(ultoa(64993, buf, 10),    buf);
        assert_eq(strcmp(buf, "64993"), 0);
        assert_eq(ultoa(32768, buf, 10),    buf);
        assert_eq(strcmp(buf, "32768"), 0);
        assert_eq(ultoa(65536, buf, 10),    buf);
        assert_eq(strcmp(buf, "65536"), 0);
        assert_eq(ultoa(0x8765ABCD, buf, 10), buf);
        assert_eq(strcmp(buf, "2271587277"), 0);
        assert_eq(ultoa(0xFFFFFFFF, buf, 10), buf);
        assert_eq(strcmp(buf, "4294967295"), 0);

        return 0;
    }
    `,
expected => ""
},


{
title => q{Assignment operators on a struct field},
program => q`
    typedef struct {
        int _dummy, value;
    } WordValue;
    
    typedef struct {
        int _dummy;
        char value;
    } ByteValue;
    
    void test1div_ww(WordValue* t)
    {
        t->value /= 2;
    }

    void test1div_bw(ByteValue* t)
    {
        t->value /= 2;
    }

    void test1mod_ww(WordValue* t)
    {
        t->value %= 100;
    }

    void test1mod_bw(ByteValue* t)
    {
        t->value %= 14;
    }

    void test1sl_ww(WordValue* t)
    {
        t->value <<= 1;
    }

    void test1sl_bw(ByteValue* t)
    {
        t->value <<= 1;
    }

    unsigned one() { return 1; }
    void test1sl_nonConstRightSide(WordValue* t)
    {
        t->value <<= one();
    }

    void test1sr_ww(WordValue* t)
    {
        t->value >>= 1;
    }
    
    void test1sr_bw(ByteValue* t)
    {
        t->value >>= 1;
    }
    
    void test2div(WordValue* t)
    {
        t->value = t->value / 2;
    }

    void test2mod(WordValue* t)
    {
        t->value = t->value % 100;
    }

    void test2sl(WordValue* t)
    {
        t->value = t->value << 1;
    }

    void test2sr(WordValue* t)
    {
        t->value = t->value >> 1;
    }
    
    void testWithWordAsLeftSide()
    {
        WordValue s0 = { 1111, 0 };
        
        s0.value = 4444;
        test1div_ww(&s0);
        assert_eq(s0.value, 2222);

        s0.value = 12345;
        test1mod_ww(&s0);
        assert_eq(s0.value, 45);

        s0.value = 333;
        test1sl_ww(&s0);
        assert_eq(s0.value, 666);
        
        assert_eq(&(s0.value /= 2), &s0.value);
        assert_eq(&(s0.value %= 2), &s0.value);
        assert_eq(&(s0.value >>= 1), &s0.value);
        assert_eq(&(s0.value <<= 1), &s0.value);
        
        s0.value = 333;
        test1sl_nonConstRightSide(&s0);
        assert_eq(s0.value, 666);

        s0.value = 444;
        test1sr_ww(&s0);
        assert_eq(s0.value, 222);
        
        s0.value <<= 10;
        assert_eq(s0.value, 0x7800);
        s0.value >>= 10;
        assert_eq(s0.value, 30);

        s0.value = 4444;
        test2div(&s0);
        assert_eq(s0.value, 2222);

        s0.value = 12345;
        test2mod(&s0);
        assert_eq(s0.value, 45);

        s0.value = 333;
        test2sl(&s0);
        assert_eq(s0.value, 666);

        s0.value = 444;
        test2sr(&s0);
        assert_eq(s0.value, 222);
    }
    
    void testWithByteAsLeftSide()
    {
        ByteValue s0 = { 1111, 0 };
        
        s0.value = 44;
        test1div_bw(&s0);
        assert_eq(s0.value, 22);

        s0.value = 123;
        test1mod_bw(&s0);
        assert_eq(s0.value, 11);

        s0.value = 33;
        test1sl_bw(&s0);
        assert_eq(s0.value, 66);
        
        assert_eq(&(s0.value /= 2), &s0.value);
        assert_eq(&(s0.value %= 2), &s0.value);
        assert_eq(&(s0.value >>= 1), &s0.value);
        assert_eq(&(s0.value <<= 1), &s0.value);

        s0.value = 44;
        test1sr_bw(&s0);
        assert_eq(s0.value, 22);
        
        s0.value <<= 10;
        assert_eq(s0.value, 0);
        s0.value = 255;
        s0.value >>= 10;
        assert_eq(s0.value, -1);  // because 'value' is signed
    }

    int main()
    {
        testWithWordAsLeftSide();
        testWithByteAsLeftSide();
        return 0;
    }
    
    `,
expected => ""
},


{
title => q{Optimizaton on boolean negation and disjunction},
program => q`
    int test(void *a, void *b)
    {
        return !a || !b;
    }
    int main()
    {
        assert(test((void *) 0, (void *) 0)); 
        assert(test((void *) 1, (void *) 0));
        assert(test((void *) 0, (void *) 1));
        assert(!test((void *) 1, (void *) 1));
        return 0;
    }
    `,
expected => ""
},


{
title => q{Avoid over-optimizing when byte constant cast to word},
program => q`
    int main()
    {
        byte b = 99;
        word w = b * (word) 100;
        assert_eq(w, 9900);
        
        char sb = 99;
        int sw = sb * (int) 100;
        assert_eq(sw, 9900);

        sb = -98;
        sw = sb * (int) -100;
        assert_eq(sw, 9800);

        sb = -97;
        sw = (int) -100 * sb;
        assert_eq(sw, 9700);

        sb = -97;
        sw = (int) 100 * sb;
        assert_eq(sw, -9700);

        return 0;
    }
    `,
expected => ""
},


{
title => q{removeUselessLdx optimization},
program => q`
    struct S
    {
        word a, b, c, d;
        byte buffer[512];
    };
    void g(void *dst, void *src, word n)
    {
        //printf("%p %p %u\n", dst, src, n);
        assert_eq(dst + 256, src);
    }
    void f(struct S *data)
    {
        g(data->buffer, data->buffer + 256, data->d); 
    }
    int main()
    {
        struct S data;
        data.d = 157;
        f(&data);
        return 0;
    }
    `,
expected => ""
},


{
title => q{__FUNCTION__ and __func__},
program => q`
    char *g0 = __FUNCTION__, *g1 = __func__;
    void someFunction()
    {
        printf("%s\n", __FUNCTION__);
        assert_eq(strcmp(__FUNCTION__, "someFunction"), 0);
        assert_eq(strcmp(__func__, "someFunction"), 0);
    }
    int main()
    {
        printf("%s\n", __FUNCTION__);
        assert_eq(strcmp(__FUNCTION__, "main"), 0);
        assert_eq(strcmp(__func__, "main"), 0);
        someFunction();
        printf("%s\n", g0);
        printf("%s\n", g1);
        assert_eq(strcmp(g0, ""), 0);
        assert_eq(strcmp(g1, ""), 0);
        return 0;
    }
    `,
expected => "main\nsomeFunction\n\n\n"
},


{
title => q{Optimizer bug in ASMText::removeAndOrMulAddSub()},
program => q`
    unsigned char a[] = { 11, 22, 33 };
    int main()
    {
        unsigned char *ptr = a;
        int start = 52 + (ptr[1] << 8) + ptr[2];  // note: shift is done on 8 bits, not 16 as in Standard C
        assert_eq(start, 85);
        start = 52 + ((unsigned) ptr[1] << 8) + ptr[2];  // force shift on 16 bits
        assert_eq(start, 5717);
        return 0;
    }
    `,
expected => ""
},




{
title => q{Enumerators defined with other enumerators},
program => q`
    enum
    {
        STACK_SPACE = 2 * 1024,
        GRAPHICS_SPACE = 4 * 1536,
        RAM_END = 0xFE00,
        STACK_START = RAM_END - STACK_SPACE,
        GRAPHICS_START = STACK_START - GRAPHICS_SPACE,
    };
    int main()
    {
        assert_eq(STACK_SPACE, 0x800);
        assert_eq(GRAPHICS_SPACE, 6144);
        assert_eq(RAM_END, 0xFE00);
        assert_eq(STACK_START, 0xF600);
        assert_eq(GRAPHICS_START, 0xDE00);
        return 0;
    }
    `,
expected => ""
},


{
title => q{Process a declarator that represents a function pointer},
program => q`
    typedef char (*FuncPtrType)(char);
    struct S0
    {
        FuncPtrType funcPtr;
    };
    char f(char n) { return n + 1; }  // of type FuncPtrType
    FuncPtrType global = f;
    int main()
    {
        struct S0 s0 = { f };
        assert_eq((*s0.funcPtr)('A'), 'B');
        assert_eq((*global)('C'), 'D');
        FuncPtrType local = f;
        assert_eq((*local)('E'), 'F');
        return 0;
    }
    `,
expected => ""
},


{
title => q{Typedef of struct registers struct S as a type},
program => q`
    typedef struct S { int m; } S;
    struct A { int m; };
    typedef struct A AA;

    struct S g_s = { 42 };
    struct A g_a = { 1844 };
    AA g_aa = { 123 };

    int main()
    {
        struct S s = { 42 };
        assert_eq(s.m, 42);
        struct A a = { 1844 };
        assert_eq(a.m, 1844);
        AA aa = { 123 };
        assert_eq(aa.m, 123);

        assert_eq(g_s.m, 42);
        assert_eq(g_a.m, 1844);
        assert_eq(g_aa.m, 123);
        return 0;
    }
    `,
expected => ""
},


{
title => q{Empty struct},
program => q`
    struct Empty {};
    int main()
    {
        struct Empty e;
        assert_ne(&e, 0);  // check that 'e' really is on the stack
        return 0;
    }
    `,
expected => ""
},


{
title => q{Passing a struct or union by value to a function},
program => q`
    #include <stdarg.h>
    
    struct S { int a, b; char c; };  // size of struct is odd number
    int f(struct S s, int x, char y)
    {
        int sum = s.a + s.b + s.c + x + y;
        ++s.b;  // only modifies the local copy of the struct
        --s.c;  // ditto
        return sum;
    }
    
    union U { unsigned n; unsigned char c; };
    unsigned getInt(union U u) { return u.n; }
    unsigned getChar(union U u) { return u.c; }
    
    void v(struct S first, ...)
    {
        assert_eq(first.c, '#');
        va_list ap;
        va_start(ap, first);
        struct S s = va_arg(ap, struct S);
        assert_eq(s.c, '$');
        char charAfter = va_arg(ap, char);
        assert_eq(charAfter, '%');
        unsigned unsignedAfter = va_arg(ap, unsigned);
        assert_eq(unsignedAfter, 0xFEDC);
        va_end(ap);
    }
    
    struct OneByte { unsigned char b; };

    void receiveOneByteStruct(struct OneByte ob)
    {
        assert_eq(ob.b, 0xAA);
    }
    void receiveOneByteStructViaEllipsis(int n, ...)
    {
        assert_eq(n, 4242);
        va_list ap;
        va_start(ap, n);
        struct OneByte ob = va_arg(ap, struct OneByte); 
        assert_eq(ob.b, 0xAA);
        char c = va_arg(ap, char);
        assert_eq(c, '*');
        va_end(ap);
    }
    void receiveOneByteStructViaEllipsis2(struct OneByte first, ...)
    {
        assert_eq(first.b, '-');
        va_list ap;
        va_start(ap, first);
        struct OneByte ob = va_arg(ap, struct OneByte); 
        assert_eq(ob.b, 0xAA);
        char c = va_arg(ap, char);
        assert_eq(c, '*');
        va_end(ap);
    }
    
    int main()
    {
        struct S instance = { 1000, 2000, 30 };
        assert_eq(sizeof(struct S), 5);
        assert_eq(sizeof(instance), 5);
        int result = f(instance, 4000, 99);
        assert_eq(instance.a, 1000);
        assert_eq(instance.b, 2000);
        assert_eq(instance.c, 30);
        assert_eq(result, 7129);
        
        union U u0;
        u0.n = 0xABCD;
        assert_eq(getInt(u0), 0xABCD);
        assert_eq(getChar(u0), 0xAB);  // 6809 is big endian
        
        instance.c = '$';
        struct S first = { 1111, 2222, '#' };
        v(first, instance, '%', 0xFEDC);
        
        struct OneByte ob0 = { 0xAA };

        asm { clra }
        receiveOneByteStruct(ob0);
        
        asm { clra }
        receiveOneByteStructViaEllipsis(4242, ob0, '*');
        
        struct OneByte ob1 = { '-' };
        asm { clra }
        receiveOneByteStructViaEllipsis2(ob1, ob0, '*');

        return 0;
    }
    `,
expected => ""
},


{
title => q{Function returning a struct or union by value},
program => q`
    struct S { unsigned a, b; unsigned char c; };
    struct S makeS(unsigned a, unsigned b, unsigned char c)
    {
        struct S ret = { a, b, c };
        return ret;
    }  
    struct S inc(struct S s)
    {
        assert_eq(s.a, 1000);
        assert_eq(s.b, 2000);
        assert_eq(s.c, 'A');
        ++s.a;
        ++s.b;
        ++s.c;
        return s;
    }
    int main()
    {
        struct S s0 = makeS(1000, 2000, 'A');
        assert_eq(s0.a, 1000);
        assert_eq(s0.b, 2000);
        assert_eq(s0.c, 'A');
        unsigned b = makeS(1234, 5678, 'B').b;
        assert_eq(b, 5678);
        s0 = inc(s0);
        assert_eq(s0.a, 1001);
        assert_eq(s0.b, 2001);
        assert_eq(s0.c, 'B');
        struct S s1 = makeS(1000, 2000, 'A');
        inc(s1);  // not receiving the return value
        return 0;
    }
    `,
expected => ""
},


{
title => q{Argument too large for function parameter},
tolerateWarnings => 1,
program => q`
    char takeChar(char x) { return x; }
    unsigned char takeUnsignedChar(unsigned char x) { return x; }
    int main()
    {
        char c = takeChar(0x1234);
        assert_eq(c, 0x34);
        c = takeChar(0x12F0);
        assert_eq(c, (int) -16); 
        unsigned char uc = takeUnsignedChar(0x1256);
        assert_eq(uc, 0x56);
        uc = takeUnsignedChar(0x12F8);
        assert_eq(uc, (int) 248); 
        return 0;
    }
    `,
expected => ""
},


{
title => q{Declaration with initializer larger than variable},
tolerateWarnings => 1,
program => q`
    void f(int i, unsigned u)
    {
        char c0 = i;
        assert_eq(c0, -32);
        char c1 = u;
        assert_eq(c1, -48);
        unsigned char uc0 = i;
        assert_eq(uc0, (unsigned char) -32);
        unsigned char uc1 = u;
        assert_eq(uc1, 208);
        int i0 = u;
        assert_eq(i0, -24624);
        unsigned u0 = i;
        assert_eq(u0, 0x7fe0);
    }
    int main()
    {
        f(0x7fe0, 0x9fd0);
        return 0;
    }
    `,
expected => ""
},


{
title => q{Array of size given by enum name which is product of other enum names},
program => q`
    enum { W = 3, H = 2, NUM = W * H };
    int a[NUM] = { 4, 5, 6, 7, 8, 9 };
    int main()
    {
        assert_eq(a[0], 4);
        assert_eq(a[1], 5);
        assert_eq(a[2], 6);
        assert_eq(a[3], 7);
        assert_eq(a[4], 8);
        assert_eq(a[5], 9);
        return 0;
    }
    `,
expected => ""
},


{
title => q{Comparing a pointer with a long zero},
program => q`
    char isNullL(int *p)
    {
        if (p == 0L)
            return 1;
        return 0;
    }
    char isNullUL(int *p)
    {
        if (0UL == p)
            return 1;
        return 0;
    }
    char isGT0ULPtrAtLeft(int *p)
    {
        if (p > 0UL)
            return 1;
        return 0;
    }
    char isGT0LPtrAtRight(int *p)
    {
        if (0L < p)
            return 1;
        return 0;
    }
    int main()
    {
        assert_eq(isNullL(0), 1);
        assert_eq(isNullUL(0), 1);
        assert_eq(isGT0ULPtrAtLeft((int *) 42), 1);
        assert_eq(isGT0ULPtrAtLeft(0), 0);
        assert_eq(isGT0LPtrAtRight((int *) 42), 1);
        assert_eq(isGT0LPtrAtRight(0), 0);
        return 0;
    }
    `,
expected => ""
},


{
title => q{Enum names vs. inline assembly},
program => q`
    enum { FOO = -4242, 
           HIGHMEM = 0xEDFF,  // stack is assumed to be just below $FE00
           LOWMEM = 12        // program is assumed to start at $4000 
           };
    int main()
    {
        int n0 = 0, n1 = 0;
        void *p0 = HIGHMEM, *p1 = LOWMEM;
        asm {
            ldd #:FOO
            std :n0
            ldx #FOO
            stx :n1
            ldx :p0
            clr ,x
            inc :HIGHMEM
            inc HIGHMEM
            ldx :p1
            clr ,x
            dec <:LOWMEM    ; this test must be loaded after address $000C
            dec <LOWMEM
        }
        assert_eq(n0, -4242);
        assert_eq(n1, -4242);
        assert_eq(* (char *) HIGHMEM, 2);
        assert_eq(* (char *) LOWMEM, -2);
        return 0;
    }
    `,
expected => ""
},


{
title => q{Comma expression},
program => q`
    int main()
    {
        int x, y, z;
        x = 1111, y = 2222, z = 3333;
        assert_eq(x, 1111);
        assert_eq(y, 2222);
        assert_eq(z, 3333);
        (x = 1, y = 2) = 3;
        assert_eq(y, 3);
        ++(x = 4, y = 5);
        assert_eq(y, 6);
        return 0;
    }
    `,
expected => ""
},


{
title => q{const and volatile keywords accepted but ignored},
tolerateWarnings => 1,
program => q`
    int main()
    {
        const int c = 0;
        c = 999;
        assert_eq(c, 999);
        const char *s = "foo";
        *s = 'g';
        assert(!strcmp(s, "goo"));
        volatile int v = -1000;
        assert_eq(v, -1000);
        return 0;
    }
    `,
expected => ""
},


{
title => q{Type-safe function pointers},
program => q`
    #include <stdarg.h>
    
    typedef char *(*FuncPtr)(int, char);
    
    char *someFunc(int a, char b) { return (char *) a + b; }
    
    void check0(char *(*fp)(int, char))
    {
        assert_eq(fp(1000, 103), (char *) 1103);
        assert_eq((*fp)(1000, 104), (char *) 1104);
    }
    
    void check1(FuncPtr fp)
    {
        assert_eq(fp(1000, 105), (char *) 1105);
        assert_eq((*fp)(1000, 106), (char *) 1106);
    }
    
    struct S
    {
        char *(*m0)(int, char);
        FuncPtr m1;
    };
    
    typedef char *(*ArrayOfFuncPtrs[2])(int, char);
    
    char noArgFunc() { return 'a'; }
    
    interrupt void isr() {}
    
    void installISR(interrupt void (*newISR)()) {}
    
    typedef interrupt void (*ISR)();
    typedef void interrupt (*AltISR)();
    
    ISR returnISR(ISR theISR) { return theISR; }
    AltISR returnAltISR(AltISR theISR) { return theISR; }
    
    int variadicFunc0(unsigned n, ...)
    {
        va_list ap;
        va_start(ap, n);
        int sum = 0;
        for (unsigned i = 0; i < n; ++i)
            sum += va_arg(ap, int);
        va_end(ap);
        return sum; 
    }
    
    int receivesArrayOfFuncPtrs(int (*arrayOfFuncPtrs[])(int), int a, int b)
    {
        return arrayOfFuncPtrs[0](a) + (*arrayOfFuncPtrs[1])(b);
    }
    
    int twice(int x) { return x << 1; }
    int thrice(int x) { return x * 3; }

    int main()
    {
        char *(*p)(int, char) = someFunc;
        assert_eq(p(1000, 100), (char *) 1100);
        assert_eq((*p)(1000, 101), (char *) 1101);
        FuncPtr q = someFunc;
        assert_eq(q(1000, 102), (char *) 1102);
        check0(someFunc);
        check1(someFunc);
        struct S s = { someFunc, someFunc };
        assert_eq(s.m0(1000, 107), (char *) 1107);
        assert_eq((*s.m0)(1000, 108), (char *) 1108);
        assert_eq(s.m1(1000, 109), (char *) 1109);
        assert_eq((*s.m1)(1000, 110), (char *) 1110);

        ArrayOfFuncPtrs a = { someFunc, someFunc };
        assert_eq(a[0](1000, 111), (char *) 1111);
        assert_eq((*a[0])(1000, 112), (char *) 1112);
        assert_eq(a[1](1000, 113), (char *) 1113);
        assert_eq((*a[1])(1000, 114), (char *) 1114);
        
        void *vp = (void *) someFunc;
        assert_eq((* (char *(*)(int, char)) vp)(1000, 115), 1115);
        assert_eq(((char *(*)(int, char)) vp)(1000, 115), 1115);

        void *zaf = (void *) noArgFunc;
        assert_eq((* (char (*)()) zaf)(), 'a');
        assert_eq(((char (*)()) zaf)() + 1, 'b');
        
        installISR(isr);
        
        assert_eq(variadicFunc0(3, 1000, 2000, 3000), 6000);
        int (*vfp)(unsigned n, ...) = variadicFunc0;
        assert_eq(vfp(3, 1000, 2000, 3000), 6000);
        assert_eq((*vfp)(3, 1000, 2000, 3000), 6000);
        
        int (*arrayOfFuncPtrs[])(int) = { twice, thrice };
        assert_eq(receivesArrayOfFuncPtrs(arrayOfFuncPtrs, 1500, 2500), 1500 * 2 + 2500 * 3);
        
        ISR foo = isr;
        assert_eq(returnISR(foo), isr);
        AltISR bar = isr;
        assert_eq(returnAltISR(bar), isr);

        return 0;
    }
    `,
expected => ""
},


{
title => q{Bug fix for old ASMText::optimizeLeaxLddStd() phase},  # 2018-01-01
program => q`
    void addLetter(char *p) { *p = 'q'; }
    void getline(char *dest)
    {
        unsigned char index = 1;
        for (;;)
        {
            addLetter(dest);
            dest[index] = 0;
            break;
            printf("");
        }
        assert_eq(dest[0], 'q');
        assert_eq(dest[1], '\0');
    }
    int main()
    {
        char line[2];
        getline(line);
        return 0;
    }
    `,
expected => ""
},


{
title => q{Pointer arithmetic vs. optimizer},
program => q`
    word *f(byte *t)
    {
        word *p = (word *) t + 25;  // cast has precedence over addition
        //printf("p=%p\n", p);
        return p;
    }
    word *g(byte *t)
    {
        word *p = (word *) (t + 25);
        //printf("p=%p\n", p);
        return p;
    }
    int main()
    {
        word *res = f(1000);
        assert_eq(res, 1050);  // must have advanced by 25 words, not 25 bytes
        res = g(2000);
        assert_eq(res, 2025);  // must have advanced by 25 bytes
        return 0;
    }
    `,
expected => ""
},


{
title => q{__norts__},
program => q`
    asm __norts__ int ordinary()
    {
        asm
        {
            ldd     #11223
            rts
        }
    }
    
    int g = 0;

    asm __norts__ interrupt void isr()
    {
        asm
        {
            ldd     #$4142
            std     :g
            rti
        }
    }

    int main()
    {
        assert_eq(ordinary(), 11223);

        // Simulate an IRQ.
        asm {
            leax @returnAddr,pcr
            pshs x  // RTI will pop this into PC
            orcc #$80  // set E flag (whole environment saved)
            pshs u,y,x,dp,b,a,cc  // save rest of environment
            leax isr
            jmp ,x
@returnAddr:
        }
        assert_eq(g, 0x4142);

        // To do: Find a dependable way to test that no RTS/RTI emitted by compiler.  

        return 0;
    }
    `,
expected => ""
},


{
title => q{BSS initialized to zero bytes},
program => q`
    char a[1024];
    int b;
    int main()
    {
        for (unsigned i = 0; i < sizeof(a); ++i)
            assert_eq(a[i], 0);
        assert_eq(b, 0);
        return 0;
    }
    `,
expected => ""
},


{
title => q{_CMOC_VERSION_},
program => q`
    #ifndef _CMOC_VERSION_
    #error _CMOC_VERSION_ not defined
    #endif
    int main()
    {
        //printf("%lu\n", (unsigned long) _CMOC_VERSION_);
        assert(_CMOC_VERSION_ >= 1000);  // at least 0.1.0
        return 0;
    }
    `,
expected => ""
},


{
title => q{sizeof on array that is declared both in same file and as extern},
program => q`
    extern int v[];  // this interfered with sizeof(v) in version 0.1.48 
    int v[] = { 9, 8, 7, 6, 5, 4 };
    int main()
    {
        assert_eq(sizeof(v), 12);
        return 0;
    }
    `,
expected => ""
},


{
title => q{String literal used with array index},
program => q`
    int main()
    {
        assert_eq("FOOBAR"[3], 'B');
        return 0;
    }
    `,
expected => ""
},


{
title => q{Array of function pointers},
program => q`
    typedef int (*Adder)(char);
    void check(const Adder funcPtrArray[])
    {
        for (byte i = 0; i < 3; ++i)
        {
            int result = (*funcPtrArray[i])(i * 10);
            int added = i + 1;
            assert_eq(result, i * 10 + added);
        } 
    }
    int f1(char c) { return c + 1; }
    int f2(char c) { return c + 2; }
    int f3(char c) { return c + 3; }
    Adder globalFuncPtrArray[] = { f1, f2, f3 };
    const Adder constGlobalFuncPtrArray[] = { f1, f2, f3 };
    int (*globalFuncPtrArray2[])(char) = { f1, f2, f3 };;
    int main()
    {
        check(globalFuncPtrArray);
        check(constGlobalFuncPtrArray);
        check(globalFuncPtrArray2);
        Adder localFuncPtrArray[] = { f1, f2, f3 };
        check(localFuncPtrArray);
        const Adder constLocalFuncPtrArray[] = { f1, f2, f3 };
        check(constLocalFuncPtrArray);
        int (*localFuncPtrArray2[])(char) = { f1, f2, f3 };;
        check(localFuncPtrArray2);
        return 0;
    }
    `,
expected => ""
},


{
title => q{Call by indirection of a function name},
program => q`
    int f(int n) { return n * 3; }
    int main()
    {
        assert_eq((*f)(1000), 3000);
        return 0;
    }
    `,
expected => ""
},


{
title => q{Binary literal},
program => q`
    int main()
    {
        assert_eq(0b0, 0);
        assert_eq(0b1, 1);
        assert_eq(0b101010, 42);
        assert_eq(0b00011u, 3);
        assert_eq(0b100000000, 256);
        assert(0b10000000000000000L == 65536L);
        assert(0B1000000000000000000000000l == 16777216L);
        assert(0b11111111111111111111111111111111UL == 4294967295UL);
        assert(0b10000000000000000000000000000000UL == 2147483648UL);
        assert(0b10000000000000000000000000000000UL > 0);
        assert(0b10000000000000000000000000000000L < -2100000000L);
        printf("%lu %lu %ld\n",
                0b11111111111111111111111111111111uL,
                0b10000000000000000000000000000000Ul,
                0b10000000000000000000000000000000L);
        return 0;
    }
    `,
expected => "4294967295 2147483648 -2147483648\n"
},


{
title => q{Minimal support for bit fields},
program => q`
    struct S0
    {
        unsigned a : 1, : 2, b : 3;
        unsigned char d : 1;
        signed char e : 3;
        unsigned long f : 17, g : 24, h : 31, i : 32;
        short j : 8, k : 9, l : 16;
    };
    int main()
    {
        assert_eq(sizeof(struct S0), 2 * 2 + 1 + 1 + 4 * 4 + 3 * 2);
        struct S0 s0;
        assert_eq(sizeof(s0.a), sizeof(unsigned));
        s0.a = 1;
        assert_eq(s0.a, 1);
        s0.a = 0;
        assert_eq(s0.a, 0);
        s0.a = 1;
        assert_eq(s0.a, 1);
        s0.a = 0;
        assert_eq(s0.a, 0);
        s0.b = 7;
        assert_eq(s0.b, 7);
        s0.b = 0;
        assert_eq(s0.b, 0);
        s0.b = 3;
        assert_eq(s0.b, 3);
        s0.b = 4;
        assert_eq(s0.b, 4);
        assert_eq(sizeof(s0.d), 1);
        assert_ne(&s0.a, &s0.b);
        assert_ne(&s0.b, &s0.d);
        assert_eq(sizeof(s0.e), 1);
        s0.e = -4;
        assert_eq(s0.e, -4);
        s0.e = 3;
        assert_eq(s0.e, 3);
        assert_eq(sizeof(s0.f), 4);
        assert_eq(sizeof(s0.g), 4);
        assert_eq(sizeof(s0.h), 4);
        assert_eq(sizeof(s0.i), 4);
        assert_eq(sizeof(s0.j), 2);
        assert_eq(sizeof(s0.k), 2);
        assert_eq(sizeof(s0.l), 2);
        return 0;
    }
    `,
expected => ""
},


{
title => q{Accept prototype for main()},
program => q`
    int main();
    int main()
    {
        return 0;
    }
    `,
expected => ""
},


{
title => q{Bug fix for assignment of array variable},
program => q`
    const byte masks[3] = { 11, 22, 33 };
    struct S
    {
        const byte *maskArray;
    };
    int main()
    {
        struct S s = { 0 };
        //printf("%p %p\n", s.maskArray, masks);
        s.maskArray = masks;
        assert_eq(s.maskArray, masks);
        return 0;
    }
    `,
expected => ""
},


{
title => q{Tolerate NULL where typed pointer is expected},
program => q`
    #define NULL ((void *) 0)
    int *f() { return NULL; }
    int main()
    {
        int n = 1234;
        int *p = NULL;
        assert_eq(p, NULL);
        p = &n;
        assert_eq(*p, 1234);
        p = NULL;
        assert_eq(p, NULL);

        const char *s = NULL;
        assert_eq(s, NULL);
        s = NULL;
        assert_eq(s, NULL);

        char *t = NULL;
        assert_eq(t, NULL);
        t = NULL;
        assert_eq(t, NULL);
        return 0;
    }
    `,
expected => ""
},


{
title => q{Optimization of multiplication of two bytes giving a word},
program => q`
    // Optimization done by BinaryOpExpr::emitMulOfTypeUnsignedBytesGivingUnsignedWord().
    void f(byte m, byte n)
    {
        word p0 = m * n;
        assert_eq(p0, 4200 & 0xFF);  // difference with Standard C: mul is done in 8 bits 
        word p1 = (word) m * n;
        assert_eq(p1, 4200);
        word p2 = m * (word) n;
        assert_eq(p2, 4200);
        word p3 = (word) m * 200;
        assert_eq(p3, 8400);
        word p4 = 21 * (word) n;
        assert_eq(p4, 2100);
        word p5 = (word) 50 * n;
        assert_eq(p5, 5000);
        word p6 = m * (word) 100;
        assert_eq(p6, 4200);
        word p7 = m * (word) 1000;  // optimization does not apply here
        assert_eq(p7, 42000);
    }
    int main()
    {
        f(42, 100);
        return 0;
    }
    `,
expected => ""
},


{
title => q{Returning non-const pointer from function returning const pointer},
program => q`
    const char *f()
    {
        char *p = 0x1234;
        return p;  
    }
    int main()
    {
        assert_eq(f(), 0x1234);
        return 0;
    }
    `,
expected => ""
},


{
title => q{Allow subtraction on const T * and T *},
program => q`
    const char *s;
    char v[1];
    int main()
    {
        s = 0xFFFF;  // use address where 'v' cannot be
        unsigned d = s - v;  // must not say "subtraction of incompatible pointers"
        assert_ne(d, 0);  // check that 's' and 'v' are different
        return 0;
    }
    `,
expected => ""
},


{
title => q{USim bug fix re: indexed addressing with 8-bit accumulator},
program => q`
    // The index in the 8-bit accumulator (e.g., LEAX A,U) is SIGNED.
    // Source: L. Leventhal, "6809 Assembly Language Programming", Osborne/McGraw-Hill,
    // end of page 3-28: "the processor interprets the contents of a single accumulator
    // as an 8-bit signed twos complement number."
    //
    int main()
    {
        word aCase = 0, bCase = 0, dCase = 0;
        asm
        {
            ldx     #$7000
            lda     #-1
            leax    a,x     ; offset in A gets sign-extended before being added to X
            stx     :aCase

            ldx     #$7000
            ldb     #-2
            leax    b,x     ; offset in B gets sign-extended before being added to X
            stx     :bCase

            ldx     #$7000
            ldd     #-3
            leax    d,x
            stx     :dCase
        }
        assert_eq(aCase, 0x6FFF);
        assert_eq(bCase, 0x6FFE);
        assert_eq(dCase, 0x6FFD);
        return 0;
    }
    `,
expected => ""
},


{
title => q{Subtraction of immediate from array/pointer, subtraction of arrays/pointers},
linkerModeOnly => 1,
program => q`
    typedef struct node_t {
        struct node_t *left, *right;
        int freq;
        char c;
    } *nodePtr;
    nodePtr arrayOfPtrs[10], *ptrToPtr;
    struct EightBytes { char b[8]; };
    struct SixteenBytes { char b[16]; };
    struct ThirteenBytes { char b[13]; };
    int main()
    {
        ptrToPtr = arrayOfPtrs - 1;
        //printf("%p, %p\n", arrayOfPtrs, ptrToPtr);
        assert_eq(arrayOfPtrs - ptrToPtr, 1);
        assert_eq((char *) arrayOfPtrs - (char *) ptrToPtr, 2);
        
        long f[10];
        assert_eq(&f[9] - &f[0], 9);
        struct EightBytes e[9];
        assert_eq(&e[9] - &e[0], 9);
        struct SixteenBytes s[9];
        assert_eq(&s[9] - &s[0], 9);
        struct ThirteenBytes t[9];
        assert_eq(&t[9] - &t[0], 9);
        return 0;
    }
    `,
expected => ""
},


{
title => q{Interrupt vector code from the manual},
program => q`
    interrupt void newCoCoIRQRoutine() {}
    int main()
    {
        unsigned char machineLanguage[3];
        const unsigned w = (unsigned) machineLanguage;
        unsigned char *irqVector = * (unsigned char **) &w;
        assert_eq(irqVector, machineLanguage);
        *irqVector = 0x7E;  // extended JMP extension
        * (void **) (irqVector + 1) = (void *) newCoCoIRQRoutine;
        assert_eq(machineLanguage[0], 0x7E);
        assert_eq(machineLanguage[1], ((unsigned) newCoCoIRQRoutine) >> 8);
        assert_eq(machineLanguage[2], ((unsigned) newCoCoIRQRoutine) & 0xFF);
        return 0;
    }
    `,
expected => ""
},


{
title => q{No optimization of load of absolute address},
program => q`
    void *findByte(const void *s, int c, size_t n)
    {
        //printf("findByte(%p, %d, %u): %u\n", s, c, n, (unsigned char) c);
        for (size_t i = 0; i < n; ++i)
        {
            //printf("findByte:   %p $%02x %d\n", &((const unsigned char *) s)[i], ((const unsigned char *) s)[i],
            //                                  (((const unsigned char *) s)[i] == (unsigned char) c));
            if (((const unsigned char *) s)[i] == (unsigned char) c)
                return (void *) &((const unsigned char *) s)[i];
        }
        return 0;
    }
    enum { IO_PORT = 0x25FF };  // assuming nothing in USim below $4000 
    int main()
    {
        // Test findByte() with a NUL in the string.
        assert_eq(findByte("FOO\0BAR", 'X', 7), 0);

        asm {
before_write:  ; start of the ML code region to check
        }
        * ((unsigned char *) IO_PORT) = 42;
        if (* ((unsigned char *) IO_PORT) == 42)
        {
            asm {
after_if:  ; end of the ML code region to check
            }
            printf("42.\n");

            // Check that there is an "LDB ,X" or "LDB extended" instruction between labels "before_write" and "after_if".
            const unsigned char *begin, *end;
            asm {
                leax    before_write,pcr
                stx     :begin
                leax    after_if,pcr
                stx     :end
            }
            for (;;)
            {
                const unsigned char *instrPos = (unsigned char *) findByte(begin, 0xE6, end - begin);
                //printf("Line %d: instrPos=%p: $%02x\n", __LINE__, instrPos, *instrPos);
                if (instrPos)  // if found "LDB indexed"
                {
                    if (instrPos[1] == 0xE0)
                    {
                        begin = instrPos + 2;  // found LDB ,S: keep searching
                        continue;
                    }
                    assert_eq(instrPos[1], 0x84);  // require "LDB ,X"
                }
                else  // look for "LDB extended" instead 
                {
                    instrPos = (unsigned char *) findByte(begin, 0xF6, end - begin);
                    //printf("instrPos=%p: $%02x\n", instrPos, *instrPos);
                    assert_ne(instrPos, 0);
                    assert_eq(instrPos[1], IO_PORT >> 8);
                    assert_eq(instrPos[2], IO_PORT & 0x00FF);
                }
                break;
            }
        }
        else
            printf("Not 42.\n");
        
        if (42 != * ((unsigned char *) IO_PORT))  // test comparison with constant on left side
            printf("Failure at line %d\n", __LINE__);

        * ((unsigned char *) IO_PORT) = 255;
        if (* ((signed char *) IO_PORT) == 255)
            printf("Failure at line %d\n", __LINE__);

        return 0;
    }
    `,
expected => "42.\n"
},


{
title => q{Comparison of byte array element with byte variable},
program => q`
    // Tests ASMText::optimize8BitStackOps().
    byte arr[10];
    
    void func(byte index, byte value)
    {
        if (arr[index] == value)
            printf("");
        else
            printf("Nope\n");
    }

    int main()
    {
        arr[3] = 'A';
        func(3, (byte) 'A');
        return 0;
    }
    `,
expected => ""
},


{
title => q{Switch statement in jump mode with only empty default statement},
compilerOptions => "--switch=jump",
program => q`
    int main()
    {
        byte r = 0;
        switch (0)
        {
        default:
            r = 1;
        }
        assert_eq(r, 1);
        switch (0)
        {
        }
        switch (1) return 42;
        assert_eq(r, 1);
        return 0;
    }
    `,
expected => ""
},


{
title => q{switch() in jump mode with all case values higher than zero},
compilerOptions => "--switch=jump",
program => q`
    int f(int n)
    {
        switch (n)
        {
        case 5:
            n = 55;
            break;
        case 8:
            n = 88;
            break;
        }
        return n;
    }
    int main()
    {
        assert_eq(f(3), 3);
        assert_eq(f(5), 55);
        assert_eq(f(6), 6);
        assert_eq(f(8), 88);
        assert_eq(f(9), 9);
        return 0;
    }
    `,
expected => ""
},


{
title => q{Optimized multiplication of word by 10},
linkerModeOnly => 1,
program => q`
    int main()
    {
        word uacc = 0;
        for (word u = 0; u != 50; u += 51, uacc += 510)  // loop until wrap around
            assert_eq(u * 10, uacc);
        sword sacc = 0;
        for (sword s = 0; s != 50; s += 51, sacc += 510)
            assert_eq(10 * s, sacc);
        return 0;
    }
    `,
expected => ""
},


{
title => q{Adding integer offset to name of matrix struct member},
tolerateWarnings => 1,
program => q`
    // The crash this test checks for did not happen with a global matrix, only for a struct member.
    struct S
    {
        unsigned char matrix[50][15];
    };
    int main()
    {
        struct S s;
        s.matrix[42][0] = '*';
        unsigned char *p = s.matrix + 42;  // equivalent to s.matrix[42], i.e., 630 byte offset (42 * 15)
        assert_eq(p, ((char *) s.matrix) + 630);
        assert_eq(s.matrix + 42, s.matrix[42]);
        assert_eq(*p, '*');
        return 0;
    }
    `,
expected => ""
},


{
title => q{Optimizations on assignments of the form * (type *) (address) = ...},
program => q`
    byte vec[4];
    int main()
    {
        * (word *) vec = 0x1234;
        assert_eq(vec[0], 0x12);
        assert_eq(vec[1], 0x34);

        * (word *) (vec + 2) = 0x5678;
        assert_eq(vec[2], 0x56);
        assert_eq(vec[3], 0x78);

        byte *ptr = vec;

        * (word *) ptr = 0x9ABC;
        assert_eq(vec[0], 0x9A);
        assert_eq(vec[1], 0xBC);

        * (word *) (ptr + 2) = 0xDEF0;
        assert_eq(vec[2], 0xDE);
        assert_eq(vec[3], 0xF0);

        // Test cases where assignment is evaluated as l-value.

        word *secondWordPtr = &(* (word *) (ptr + 2) = 0x1122);
        assert_eq(vec[2], 0x11);
        assert_eq(vec[3], 0x22);
        assert_eq(secondWordPtr, ptr + 2);

        secondWordPtr = 0;  // make sure next assignment does not get optimized out
        assert_eq(secondWordPtr, 0);

        secondWordPtr = &(* (word *) (ptr + 2) = 0x3344);
        assert_eq(vec[2], 0x33);
        assert_eq(vec[3], 0x44);
        assert_eq(secondWordPtr, ptr + 2);

        return 0;
    }
    `,
expected => ""
},


{
title => q{Array indexing optimization for unsigned byte index and element size},
program => q`
    int a[10][15];
    void test(byte index)
    {
        a[index][9] = 'X';
    }
    int main()
    {
        test(7);
        assert_eq(a[7][9], 'X');
        return 0;
    }
    `,
expected => ""
},


{
title => q{Backslash sequence interpretation in global array initializer},
program => q`
    static const char vec[] = "\r\nFOO\tBAR";
    int main()
    {
        assert_eq(vec[0], 13);
        assert_eq(vec[1], 10);
        assert_eq(vec[2], 64 + 6);
        assert_eq(vec[5], 9);
        assert_eq(vec[9], 0);
        return 0;
    }
    `,
expected => ""
},


{
title => q{Inline assembly in function that receives parameter named s},
program => q`
    asm int f(int k, int s)
    {
        asm
        {
            ldd     2,s     ; k 
        }
    }
    int g(int k, int s)
    {
        int temp;
        asm
        {
            leax    4,s     // note that we can't rely on where S is at this point
            tfr     x,d     // so we do a different test
            leax    ,s
            pshs    x
            subd    ,s++
            std     :temp 
        }
        return temp;
    }
    int h(int k, int s)
    {
        int low, high;
        asm("leax", "7,s");
        asm("stx", high);
        asm("leax", ",s");
        asm("stx", low);
        return high - low;
    }
    byte v(const char *s)
    {
        byte temp;
        asm
        {
            ldb     :s[3]
            stb     :temp
        }
        return temp;
    }
    int main()
    {
        assert_eq(f(7777, 0), 7777);
        assert_eq(g(8888, 0), 4);
        assert_eq(h(9999, 0), 7);
        assert_eq(v("foobar"), 'b');
        return 0;
    }
    `,
expected => ""
},


{
title => q{First parameter passed in register},
program => q`
    int firstParamOnStack(int a, int b)
    {
        assert_eq(&a + 1, &b);  // both params are contiguous above the return address, so one "int" of difference
        return a + b;
    } 
    _CMOC_fpir_ int firstParamInReg(int a, int b)
    {
        assert(&a <= &b - 2);  // a is below return address, so at least two "ints" of difference
        return a + b;
    }
    typedef _CMOC_fpir_ int (*FuncPtrType_firstParamInReg)(int a, int b);
    struct S { int x; };
    _CMOC_fpir_ struct S returnsStruct(int a, int b)
    {
        struct S s = { a + b };
        return s; 
    } 
    _CMOC_fpir_ struct S returnsStructNoArgs()
    {
        struct S s = { -9876 };
        return s; 
    } 
    _CMOC_fpir_ long returnsLong(unsigned a, unsigned b)
    {
        return (long) a + (long) b; 
    }
    _CMOC_fpir_ long takesLongButNotAsFirstParam(int a, long b)
    {
        return a + b;
    }
    int main()
    {
        assert_eq(firstParamOnStack(1000, 2000), 3000);
        assert_eq(firstParamInReg(4000, 5000), 9000);

        FuncPtrType_firstParamInReg fp = firstParamInReg;
        assert_eq((*fp)(6000, 7000), 13000);

        struct S s = returnsStruct(500, 600);
        assert_eq(s.x, 1100);
        s = returnsStructNoArgs();
        assert_eq(s.x, -9876);
        assert(returnsLong(50000, 60000) == 110000L);
        assert(takesLongButNotAsFirstParam(333, 70000L) == 70333L);
        return 0;
    }
    `,
expected => ""
},


{
title => q{Optimizations on pointer arithmetic},
program => q`
    int main()
    {
        byte DR0TRK[4];
        * (word *) DR0TRK = 0;
        * (word *) (DR0TRK + 2) = 0;
        asm { nop }  // Keep optimizer from using previous lines to optimize next one. 
        word *ptr = &(* (word *) (DR0TRK + 2) = 0);
        assert_eq(ptr, DR0TRK + 2);
        *ptr = 0xABCD;
        assert_eq(DR0TRK[2], 0xAB);
        assert_eq(DR0TRK[3], 0xCD);
        return 0;
    }
    `,
expected => ""
},


{
title => q{qsort()},
linkerModeOnly => 1,
program => q`
    int compareInts(const void *a, const void *b)
    {
        int intA = * (int *) a, intB = * (int *) b;
        return intA == intB ? 0 : (intA < intB ? -1 : 1);
    }
    int compareSignedBytes(const void *a, const void *b)
    {
        char charA = * (char *) a, charB = * (char *) b;
        return charA == charB ? 0 : (charA < charB ? -1 : 1);
    }
    typedef struct S { unsigned long x, y, z; } S;
    int compareYFields(const void *a, const void *b)
    {
        const S *aStruct = (const S *) a;
        const S *bStruct = (const S *) b;
        unsigned long aULong = aStruct->y, bULong = bStruct->y;
        return aULong == bULong ? 0 : (aULong < bULong ? -1 : 1);
    }

    char signedBytes[] =  // Defining this in main() would increase -O2 compile time...
    {
         118,  -91,  -67,  107,   35,    8,  -71,  115,  -60,    6,   57,  -78,  -67,   83,   25,  -12, 
         -10, -117,  -35,   32,   64,  -43,   64,   52, -108,  123,   98,   12, -101,   61, -107,  100, 
          40,  -19,   75,  -83, -100,    7,  -32,   30,  -14,  -96, -106,   54,   55,  -21,  -83,  -41, 
         120,  -84,  -28,  -37,  -69,  -39, -111,  -50,    9,    5,  -46,  -70,   40,   51,  -65,  124, 
          71, -124,   17,   66,   -1,   86,   71,  119,   73,   52,  -40,    3,  -26,   96,   65, -116, 
         -86,  -37,  124,  -98,   82,   26, -123,  -73,   40,   95,  127,  -64,   93,  110,  -99,  -74, 
          13, -128,  -89,    0,  119,   51,   95,  -61,  -72,  117,  -74,  -42,  121,   73,   10,   19, 
          -3,  119,  -32,  -77,  -11,  -17,  -77,  -30,    2,   -7,   87,  -60,   65,  -31,  -86,  -95, 
        -125,   53,   71,  -28,  -31,  124,  -93,   67,    8,   23,   15,  -95,  108,   40,  -80, -116, 
         -75,    1,  -55,  101,   -4,  -93, -112, -109,  -31,  -57,   12,   66,  123,   96,   82,  -58, 
         114,   92,  101,   16,  -22,    6,   30,    3,   99,    3,   41,   20,  -54,   70,  -54,  -93, 
         114, -102,   20,  -96,  123,  -78,   -2, -124,   48,   97,  -63,  -69,   33,   -8,  -72, -115, 
          21,   24,   17,   28,   64,  -33, -108,  119,   59, -118,  115, -111,  -84,  -83, -114,   -2, 
         -83,  -80, -128,   13,  -16,   54,  -84,   29,    2,  106,  110, -120,  125,  -99,  -12,   81, 
          17,   97,  -98,   43,   86,    1,  -54,  113,   25,   23,   35,   10,  -29,  -86,   62,   29, 
         -53,   69,  -17,   61,  -62,    4,  -32,   77,  -56,  -77,  100,  -30,   71,  -87,   87,   44
     };

    int main()
    {
        int numbers[] = { 3000, 1000, 4000, 1000, 6000 };

        qsort(numbers, 5, 0, compareInts);  // does nothing
        assert_eq(numbers[0], 3000);
        assert_eq(numbers[1], 1000);
        assert_eq(numbers[2], 4000);
        assert_eq(numbers[3], 1000);
        assert_eq(numbers[4], 6000);

        qsort(numbers, 5, sizeof(int), compareInts);
        assert_eq(numbers[0], 1000);
        assert_eq(numbers[1], 1000);
        assert_eq(numbers[2], 3000);
        assert_eq(numbers[3], 4000);
        assert_eq(numbers[4], 6000);

        qsort(numbers, 0, sizeof(int), compareInts);  // empty array: does nothing
        qsort(numbers, 1, sizeof(int), compareInts);  // single element: ditto 

        // Signed bytes:
        char pair[2] = { 42, -128 };
        qsort(pair, 2, 1, compareSignedBytes);
        assert_eq(pair[0], -128);
        assert_eq(pair[1], 42);
        char otherPair[2] = { -91, -12 };
        qsort(otherPair, 2, 1, compareSignedBytes);
        assert_eq(otherPair[0], -91);
        assert_eq(otherPair[1], -12);
        qsort(signedBytes, sizeof(signedBytes), 1, compareSignedBytes);
        #if 0
        printf("Sorted signedBytes:"); 
        for (size_t i = 0; i < sizeof(signedBytes); ++i)
            printf(" %3d", signedBytes[i]);
        printf("\n");
        #endif
        for (size_t i = 1; i < sizeof(signedBytes); ++i)
            if (signedBytes[i - 1] > signedBytes[i])
            {
                printf("ERROR: signedBytes: byte at %u is %d but byte at %u is %d (wrong order)\n",
                        i - 1, signedBytes[i - 1], i, signedBytes[i]);
                assert(0);
            }

        S structs[] = {
            {  230331361UL, 1993560328UL, 0UL },
            { 1785893287UL,   63785561UL, 0UL },
            { 2097741729UL, 1847324532UL, 0UL },
            { 2436699535UL, 1665995008UL, 0UL },
            {  840325061UL,  501404609UL, 0UL },
            { 1922976238UL, 2272223599UL, 0UL },
            {   50515110UL,    7890734UL, 0UL },
            { 3249839944UL, 4177823591UL, 0UL },
            { 3017066355UL, 4150307908UL, 0UL },
            { 2844733157UL, 1957227465UL, 0UL },
        };
        const size_t numElements = sizeof(structs) / sizeof(structs[0]);
        // Set z field to x + y on each struct.
        for (byte i = 0; i < numElements; ++i)
            structs[i].z = structs[i].x + structs[i].y;
        // Sort on y field, i.e., middle column.
        qsort(structs, numElements, sizeof(S), compareYFields);
        for (byte i = 0; i < numElements; ++i)
        {
            // Check that members other than y are still the same.
            if (structs[i].z != structs[i].x + structs[i].y)
            {
                printf("ERROR: structs: z is %lu at %u but should be %lu + %lu = %lu\n",
                        structs[i].z, i, structs[i].x, structs[i].y, structs[i].x + structs[i].y);
                assert(0);
            }
            // Check for expected order of y fields.
            if (i > 0 && structs[i - 1].y > structs[i].y)
            {
                printf("ERROR: structs: y field at %u is %lu but y field at %u is %lu (wrong order)\n",
                        i - 1, structs[i - 1].y, i, structs[i].y);
                assert(0);
            }
        }
        return 0;
    }
    `,
expected => ""
},


{
title => q{bsearch()},
linkerModeOnly => 1,
program => q`
    typedef struct S
    {
        const char *value;
        int key;
    } S;
    const S entries[] =  // must be sorted by 'key' field:
    {
        { "foo", 1000 },
        { "bar", 2000 },
        { "baz", 3000 },
        { "quux", 4000 },
        { "waldo", 5000 },
    };
    int compareKeyToS(const void *keyPtr, const void *elementPtr)
    {
        int key = * (const int *) keyPtr, elementKey = ((const S *) elementPtr)->key;
        return key == elementKey ? 0 : (key < elementKey ? -1 : 1);
    }
    int main()
    {
        size_t numEntries = sizeof(entries) / sizeof(entries[0]);
        for (size_t i = 0; i < numEntries; ++i)
        { 
            int targetKey = (i + 1) * 1000;
            void *ret = bsearch(&targetKey, entries, numEntries, sizeof(entries[0]), compareKeyToS);
            assert_eq(ret, &entries[i]);
            //printf("%s %d\n", ((S *) ret)->value, ((S *) ret)->key);
            
            targetKey += 333;  // inexistent key
            ret = bsearch(&targetKey, entries, numEntries, sizeof(entries[0]), compareKeyToS);
            assert_eq(ret, NULL);
        }
        
        int targetKey = entries[0].key - 444;  // lower than smallest key
        void *ret = bsearch(&targetKey, entries, numEntries, sizeof(entries[0]), compareKeyToS);
        assert_eq(ret, NULL);

        // Empty array.
        ret = bsearch(&targetKey, entries, 0, sizeof(entries[0]), compareKeyToS);
        assert_eq(ret, NULL);
        return 0;
    }
    `,
expected => ""
},


{
title => q{Optimization stripOpToDeadReg vs. TFR CC,B},
program => q`
    int main()
    {
        byte b = 0xAA;
        byte *p = &b;
        byte mask = 0x02;
        byte isBitReset = !(*p & mask);
        assert_eq(isBitReset, 0);
        byte isBitSet = !!(*p & mask);
        assert_eq(isBitSet, 1);
        return 0;
    }
    `,
expected => ""
},


{
title => q{Optimization removeUselessLeax vs. TFR X,D},
program => q`
    int main()
    {
        byte m[4] = { 0x10, 0x11, 0x12, 0x13 };
        * (word *) m = * (word *) (m + 2) = 0;
        assert_eq(m[0], 0);
        assert_eq(m[1], 0);
        assert_eq(m[2], 0);
        assert_eq(m[3], 0); 
        return 0;
    }
    `,
expected => ""
},


{
title => q{Two increments in a for() loop},
program => q`
    void f(size_t n, const void *m)
    {
        for (size_t i = 0; i < n; ++i, m += 256)
            printf("%p\n", m);
    }
    int main()
    {
        f(5, 1024);
        return 0;
    }
    `,
expected => "\$0400\n\$0500\n\$0600\n\$0700\n\$0800\n"
},


{
title => q{Function parameters of types based on array typedefs},
program => q`
    typedef short Vec[8];
    typedef short Vec2[3][5];
    void f(Vec x)
    {
        //printf("x at %p\n", &x);
        for (byte i = 0; i < 8; ++i)
        {
            //printf("x[%u] at %p\n", i, &x[i]);
            x[i] = 100 + i;
        }
    }
    void g(Vec2 a)
    {
        a[2][4] = 5555;
    }
    void h(Vec b[3])
    {
        b[2][7] = 4444;
    }
    void p(Vec c[7][9])
    {
        c[6][8][7] = 6666;
    }
    int main()
    {
        assert_eq(sizeof(Vec), 16);
        Vec y;
        assert_eq(sizeof(y), 16);
        assert_eq(sizeof(y[0]), 2);
        f(y);
        for (byte i = 0; i < 8; ++i)
            assert_eq(y[i], 100 + i);

        Vec2 aa;
        g(aa);
        assert_eq(aa[2][4], 5555);
        
        Vec bb[3];
        h(bb);
        assert_eq(bb[2][7], 4444);

        Vec cc[7][9];
        p(cc);
        assert_eq(cc[6][8][7], 6666);

        return 0;
    }
    `,
expected => ""
},


{
title => q{Extern declaration in a function},
program => q`
    int n;
    int *getAddressOfGlobal()
    {
        return &n;
    }
    int main()
    {
        extern int n;
        //printf("%p %p\n", &n, getAddressOfGlobal());
        assert_eq(&n, getAddressOfGlobal());
        return 0;
    }
    `,
expected => ""
},


{
title => q{Struct initialized with static values},
program => q`
    struct S
    {
        char c;
        int i;
        long l;
    };
    struct S instance = { 'c', 1000, 1000000L };
    int main()
    {
        assert_eq(instance.c, 'c');
        assert_eq(instance.i, 1000);
        assert_eq(instance.l, 1000000L);
        return 0;
    }
    `,
expected => ""
},


{
title => q{Constant cast to long is not considered compile-time constant (as of 0.1.61)},
program => q`
    void test(void)
    {
        unsigned long a;
        a = (unsigned long) 1 << 30;
        assert_eq(a, 0x40000000UL);
    }
    int main()
    {
        test();
        return 0;
    }
    `,
expected => ""
},


{
title => q{Bitwise not on long},
program => q`
    int main()
    {
        long n = 1024;
        long m = ~n;
        assert_eq(m, 0xfffffbffUL);
        return 0;
    }
    `,
expected => ""
},


{
title => q{Using a long as argument of a logical OR or AND},
program => q`
    int main()
    {
        long m = 3;
        char x = 5;
        int r0 = (x || m);
        assert_eq(r0, 1);
        int r1 = (m || x);
        assert_eq(r1, 1);
        int r2 = (x && m);
        assert_eq(r2, 1);
        int r3 = (m && x);
        assert_eq(r3, 1);
        return 0;
    }
    `,
expected => ""
},


{
title => q{Long comparisons and casting a long to a short},
program => q`
    int main()
    {
        // u16 vs u32:

        unsigned int  a = 1;
        unsigned long b = 2;
        assert(a < b);
        assert(a < (unsigned int) b);
        assert((unsigned int) a < b);
        assert((unsigned int) a < (unsigned int) b);

        a = 3;
        assert(a > b);
        assert(a > (unsigned int) b);
        assert((unsigned int) a > b);
        assert((unsigned int) a > (unsigned int) b);

        // s16 vs s32:

        signed int  d = 1;
        signed long e = 2;
        assert(d < e);
        assert(d < (unsigned int) e);
        assert((unsigned int) d < e);
        assert((unsigned int) d < (unsigned int) e);

        d = 3;
        assert(d > e);
        assert(d > (signed int) e);
        assert((signed int) d > e);
        assert((signed int) d > (signed int) e);

        // u8 vs u32:

        unsigned char c = 1;
        assert(c < b);
        assert(c < (unsigned int) b);
        assert((unsigned int) c < b);
        assert((unsigned int) c < (unsigned int) b);

        c = 3;
        assert(c > b);
        assert(c > (unsigned char) b);
        assert((unsigned char) c > b);
        assert((unsigned char) c > (unsigned char) b);

        // s8 vs s32:

        signed char f = 1;
        assert(f < b);
        assert(f < (signed int) b);
        assert((signed int) f < b);
        assert((signed int) f < (signed int) b);

        f = 3;
        assert(f > b);
        assert(f > (signed char) b);
        assert((signed char) f > b);
        assert((signed char) f > (signed char) b);

        return 0;
    }
    `,
expected => ""
},


{
title => q{Enumerated name initialized with sizeof expression, and later array initialized from enum name},
program => q`
    const int v[] = { 10, 20, 30 };

    enum
    {
        N = sizeof(v),
        Q = sizeof(v) / sizeof(v[0])
    };

    const int w[N];

    int main()
    {
        assert_eq(sizeof(v), 6);
        assert_eq(sizeof(v[0]), 2);
        assert_eq(N, 6);
        assert_eq(Q, 3);
        assert_eq(sizeof(w), 12); // 6 * sizeof(int)
        return 0;
    }
    `,
expected => ""
},


{
title => q{Or-assign operator inside an if() statement},
program => q`
    long a = 0xab0defL, b = 0x00c000L;
    int c = 1;
    int main()
    {
        if (c)
            a |= b;
        assert_eq(a, 0xabcdefL);
        if (c)
            a &= b;
        assert_eq(a, 0x00c000L);
        return 0;
        if (c)
            a ^= b;
        assert_eq(a, 0x000000L);
        return 0;
    }
   `,
expected => ""
},


{
title => q{Array reference with add on left side},
program => q`
    int main()
    {
        char buf[2] = "X";
        char i = (buf + 0)[0];
        assert_eq(i, 'X');
        return 0;
    }
   `,
expected => ""
},


{
title => q{Global initializer that depends on global array name},
program => q`
    struct S
    {
        int m;
    };
    struct S a[5];
    struct S *e = &a[5];
    struct S mat[5][3];
    struct S *e1 = &mat[4][2];
    int main()
    {
        assert_eq(e, a + 5);
        assert_eq(sizeof(mat), 5 * 3 * 2);
        assert_eq(e1, (byte *) mat + (4 * 3 + 2) * 2);
        return 0;
    }
    `,
expected => ""
},


{
title => q{Shifting an unsigned long by 8, 16 or 24 bits},
program => q`
    unsigned long lShift8(unsigned long u)
    {
        return u << 8;
    }
    unsigned long lShift16(unsigned long u)
    {
        return u << 16;
    }
    unsigned long lShift24(unsigned long u)
    {
        return u << 24;
    }
    unsigned long rShift8(unsigned long u)
    {
        return u >> 8;
    }
    unsigned long rShift16(unsigned long u)
    {
        return u >> 16;
    }
    unsigned long rShift24(unsigned long u)
    {
        return u >> 24;
    }
    int main()
    {
        assert_eq(lShift8(0xDEADBEEFul),  0xADBEEF00ul);
        assert_eq(lShift16(0xDEADBEEFul), 0xBEEF0000ul);
        assert_eq(lShift24(0xDEADBEEFul), 0xEF000000ul);
        assert_eq(rShift8(0xDEADBEEFul),  0x00DEADBEul);
        assert_eq(rShift16(0xDEADBEEFul), 0x0000DEADul);
        assert_eq(rShift24(0xDEADBEEFul), 0x000000DEul);
        return 0;
    }
    `,
expected => ""
},


{
title => q{While loop with comma expression as condition},
program => q`
    byte numDummyCalls = 0;
    byte dummy()
    {
        ++numDummyCalls;
        return 42;
    }
    byte f(word x0, byte y0, word x1, byte y1)
    {
        byte iterCounter = 0;
        while (dummy(), dummy(), dummy(), x0 != x1 || y0 != y1)
        {
            //printf("in loop: %u %u %u %u\n", x0, x1, y0, y1);
            ++x0;
            ++y0;
            ++iterCounter;
        }
        return iterCounter;
    }
    int main()
    {
        assert_eq(f(5, 15, 10, 20), 5);
        assert_eq(numDummyCalls, 3 * (1 + 5));
        return 0;
    }
    `,
expected => ""
},


#{
#title => q{Sample test},
#program => q`
#    int main()
#    {
#        return 0;
#    }
#    `,
#expected => ""
#},


);  # End of test case list.


###############################################################################

sub usage
{
    print <<__EOF__;
Usage: $0 [options] SRCDIR

Options:
--nocleanup       Do not delete the intermediate files after running.
--only=NUM        Only run test #NUM, with no clean up. Implies --nocleanup.
--last            Only run the last test. Implies --nocleanup.
--start=NUM       Start at test #NUM. The first test has number zero.
--stop-on-fail    Stop right after a test has failed instead of continuing
                  to the end of the test list. Implies --nocleanup.
--titles[=STRING] Dump test titles (with numbers) to standard output.
                  If STRING specified, only dumps titles that contain STRING.
--coco            Generate test####.bin files and suite###.bin files to be run
                  on a CoCo.
--compile-as-c    Check that each program is accepted by the local C compiler.
--load-offset=D   Tell 6809 simulator to load program with specified HEX offset.
--optims=L        Compile with level L optimizations (default is $optimLevel).

__EOF__

    exit $_[0];
}

###############################################################################

my $helpWanted = 0;
my $noCleanUp = 0;
my $onlyArg;
my $onlyLast;
my $firstTestToRun;
my $loadOffsetArg;
my $titleDumpWanted;

if (!GetOptions(
        "help" => \$helpWanted,
        "only=s" => \$onlyArg,
        "start=i" => \$firstTestToRun,
        "last" => \$onlyLast,
        "nocleanup" => \$noCleanUp,
        "stop-on-fail" => \$stopOnFail,
        "coco" => \$generateCoCoBinary,
        "compile-as-c" => \$testCCompilability,
        "load-offset=s" => \$loadOffsetArg,
        "titles:s" => \$titleDumpWanted,  # the ':' means argument is optional
        "optims=i" => \$optimLevel,
        ))
{
    usage(1);
}

usage(0) if $helpWanted;

if (defined $titleDumpWanted)
{
    my $testIndex = 0;
    for my $rhTest (@testCaseList)
    {
        my $title = $rhTest->{title};
        die unless defined $title;
        if (indexi($title, $titleDumpWanted) != -1)  # if title contains --titles argument (empty string matches all)
        {
            printf("%6u\t%s\n", $testIndex, $title);
        }
        ++$testIndex;
    }
    exit 0;
}

if (defined $loadOffsetArg)
{
    if ($loadOffsetArg !~ /^[0-9a-f]+$/ || length($loadOffsetArg) > 4)
    {
        print "$0: ERROR: invalid load offset: $loadOffsetArg\n\n";
        usage(1);
    }
    $hexLoadOffset = hex($loadOffsetArg);
}

my $cleanUp = !$noCleanUp && !defined $onlyArg && !defined $onlyLast && !$stopOnFail;

my @testNumbers;  # numbers (indexes in @testCaseList) of the tests to be run

if (defined $onlyArg)  # --only=#,#,#
{
    if ($onlyArg =~ /^[\d+,]+$/)  # if only numbers and commas
    {
        @testNumbers = split /,/, $onlyArg;
    }
    else
    {
        @testNumbers = $onlyArg;
    }

    for my $n (@testNumbers)
    {
        unless ($n =~ /^\d+$/)
        {
            # $n may be a substring that matches parts of test titles.
            my $raMatches = findMatchingTestNumbers($n);
            if (@$raMatches == 0)
            {
                print "Invalid test number '$n'\n\n";
                usage(1);
            }

            # If $n matches more than one test title, list matches and fail.
            if (@$raMatches > 1)
            {
                print "Ambiguous test name '$n':\n";
                for my $match (@$raMatches)
                {
                    printf("%4u  %s\n", $match->{num}, $match->{title});
                }
                print "\n";
                usage(1);
            }

            # Only one match: take the test number and continue.
            $n = $raMatches->[0]->{num};
        }
    }
}
elsif (defined $onlyLast)  # --last
{
    @testNumbers = $#testCaseList;
}
elsif (defined $firstTestToRun)  # --start=N
{
    @testNumbers = $firstTestToRun .. $#testCaseList;
}
else  # all
{
    @testNumbers = 0 .. $#testCaseList;
}

if ($optimLevel < 0 || $optimLevel > 2)
{
    print "$0: Invalid --optims level $optimLevel\n\n";
    usage(1);
}

my $srcdir = shift || ".";

my $assemblerFilename = "$srcdir/a09";
my @includeDirList;
my $usimFilename = "./usim-0.91-cmoc/usim";  # not in srcdir

$ENV{PATH} = $srcdir . ":" . $ENV{PATH};  # allows a09 to find intelhex2cocobin


###############################################################################


sub indexi
{
    my ($haystack, $needle) = @_;
    
    return index(lc($haystack), lc($needle));
}


# Searches the {title} field in @testCaseList.
#
sub findMatchingTestNumbers
{
    my ($testSubName) = @_;

    my @matchingNumbers;
    my $testNum = 0;
    for my $rhTestCase (@testCaseList)
    {
        my $title = $rhTestCase->{title} or die;
        if (indexi($title, $testSubName) != -1)
        {
            push @matchingNumbers, { num => $testNum, title => $title };
        }
        ++$testNum;
    }
    return \@matchingNumbers;
}


sub compileAsC($$)
{
    my ($program, $cFilename) = @_;

    my $cmd = "gcc -o ,tempprog -I . -x c -std=c99 '$cFilename'";
    if (system($cmd) != 0)
    {
        print "$0: ERROR: failed to compile program as C\n";
        return 0;
    }

    if (!unlink(",tempprog"))
    {
        print "$0: ERROR: failed to delete temporary executable ',tempprog'\n";
        return 0;
    }

    return 1;
}


# Returns the output of the simulator, or undef if $generateCoCoBinary is true,
# in which case a test####.bin file is created in the current directory.
#
# $org: Hex string.
#
sub testProgram($$$$$$)
{
    my ($testNo, $program, $cFilename, $execFilename, $rhTestCase, $org) = @_;

    my $cFile;
    if (!open($cFile, "> $cFilename"))
    {
        print "$0: ERROR: failed to create source file $cFilename: $!\n";
        return undef;
    }

    print $cFile $program;
    if (!close($cFile))
    {
        print "$0: ERROR: failed to close source file $cFilename: $!\n";
        return undef;
    }

    if ($testCCompilability && !compileAsC($program, $cFilename))
    {
        return undef;
    }

    my $compCmd = "./cmoc --usim --verbose -nostdinc -O$optimLevel --org=$org --intermediate";
    
    $compCmd .= " -Lstdlib/ -L float";

    for my $includeDir (@includeDirList)
    {
        $compCmd .= " -I '$includeDir'";
    }

    if (!defined $rhTestCase->{tolerateWarnings})
    {
        $compCmd .= " -Werror";
    }

    if (defined $rhTestCase->{compilerOptions})
    {
        $compCmd .= " " . $rhTestCase->{compilerOptions};
    }

    if ($generateCoCoBinary)
    {
        $compCmd .= " --coco";
    }
    
    if (defined $ENV{CMOC_TEST_OPTIONS})
    {
        $compCmd .= " " . $ENV{CMOC_TEST_OPTIONS};
    }
    
    $compCmd .= " $cFilename";
    print "--- Compilation command: $compCmd\n";
    my $fh;
    if (!open($fh, "$compCmd 2>&1 |"))
    {
        print "$0: ERROR: failed to compile preceding program with command $compCmd: $!\n";
        return undef;
    }
    my $line;
    while ($line = <$fh>)
    {
        $line =~ s/: warning:/: __warning__:/g;  # mask warning to keep auto build system from complaining 
        print $line;
    }

    if (!close($fh))
    {
        print "$0: ERROR: failed to close pipe to command $compCmd: $!\n";
        return undef;
    }

    if ($generateCoCoBinary)
    {
        my $binFilename = sprintf("test%04u.bin", $testNo);
        print "Writing $binFilename\n";
        my $org = $rhTestCase->{org};
        die unless defined $org;
        system("intelhex2cocobin --entry=$org < $execFilename > $binFilename") == 0
            or exit 1;

        return undef;  # no execution output to return
    }

    my $usimCmd = sprintf("'%s' %s %x", $usimFilename, $execFilename, $hexLoadOffset);
    print "--- Execution command: $usimCmd\n";
    my $usim;
    if (!open($usim, "$usimCmd 2>&1 |"))
    {
        print "$0: ERROR: usim execution failed ($usimCmd): $!\n";
        return undef;
    }

    # Read and return all output from 6809 simulator.
    #
    my $temp = $/;
    $/ = undef;
    my $output = <$usim>;
    $/ = $temp;
    return $output;
}


# If $generateCoCoBinary is true, does not run the test, but generates
# a CoCo DECB .BIN file instead.
#
sub runTestNumber($)
{
    my ($i) = @_;

    if ($i < 0 || $i >= @testCaseList)
    {
        print "$0: ERROR: no test case #$i\n";
        return 0;
    }
    my $rhTestCase = $testCaseList[$i];
    die unless defined $rhTestCase;

    my $program = $rhTestCase->{program};
    
    die unless defined $program;
    die unless defined $rhTestCase->{title};

    # Prepend the program with a definition of an assert() macro, etc.
    # We avoid the stringification operator (#) because it may not be supported
    # by the system's preprocessor.
    #
    my $preamble = <<__EOF__;
#ifdef __GNUC__
#include <stdio.h>
#else
#include <cmoc.h>
#endif

typedef unsigned char byte;
typedef signed char sbyte;
typedef unsigned int word;
typedef signed int sword;
#define assert(cond) do { if (!(cond)) printf("ERROR: assert failed: line %d\\n", __LINE__); } while (0)
#define assert_eq(actual, expected) \\
do { if ((actual) != (expected)) \\
         printf("ERROR: assert_eq failed: line %d: should be equal: got %u (\$%x), expected %u (\$%x)\\n", \\
                __LINE__, (word) (actual), (word) (actual), (word) (expected), (word) (expected)); \\
   } while (0)
#define assert_ne(actual, expected) \\
do { if ((actual) == (expected)) \\
         printf("ERROR: assert_ne failed: line %d: should be different: got %u (\$%x), expected %u (\$%x)\\n", \\
                __LINE__, (word) (actual), (word) (actual), (word) (expected), (word) (expected)); \\
   } while (0)
#define assert_range(actual, expectedMin, expectedMax) \\
do { if ((actual) < (expectedMin) && (actual) > (expectedMax)) \\
         printf("ERROR: assert_range failed: line %d: out of range: got %u (\$%x), expected %u..%u (\$%x..\$%x)\\n", \\
                __LINE__, (word) (actual), (word) (actual), (word) (expectedMin), (word) (expectedMax), (word) (expectedMin), (word) (expectedMax)); \\
   } while (0)
__EOF__

    my $org = "4000";

    unless (defined $rhTestCase->{noPreamble})
    {
        $program = $preamble . $program;
    }

    $rhTestCase->{org} = $org;

    if (!$generateCoCoBinary)
    {
        print "\n";
        print "-" x 80, "\n";
        print "--- Program # $i: ", $rhTestCase->{title}, "\n";

        my $lineNum = 0;
        for my $line (split /\n/, $program)
        {
            printf("%3u%5s%s\n", ++$lineNum, "", $line);
        }
    }

    if (defined $rhTestCase->{suspended})
    {
        print "\n";
        print "This test is suspended.\n";
        print "\n";
        return 1;
    }

    my $cFilename = ",check-prog.c";
    my $execFilename = ",check-prog.srec";
    
    my $actualOutput = testProgram($i, $program, $cFilename, $execFilename, $rhTestCase, $org);

    if ($generateCoCoBinary)
    {
        return 1;
    }

    if (defined $rhTestCase->{expected})
    {
        print "--- Expected output:\n", $rhTestCase->{expected};
    }

    if (!defined $actualOutput)
    {
        print "$0: ERROR: program #$i: no output\n";
        return 0;
    }

    print "--- Actual output:\n", $actualOutput, "-" x 80, " \n";

    if (defined $rhTestCase->{expected} && $actualOutput ne $rhTestCase->{expected})
    {
        print "$0: ERROR: program #$i: actual output differs from expected output\n";
        return 0;
    }

    if (!defined $rhTestCase->{expected} && $actualOutput !~ /^(|.*?\n)success\n$/s)
    {
        print "$0: ERROR: program #$i: actual output does not end with 'success' line\n";
        return 0;
    }

    return 1;
}


sub cleanUp($)
{
    my ($basename) = @_;

    print "\n";
    print "\n";
    print "Cleaning up:\n";
    my $success = 1;
    for my $ext (qw(c asm s i lst hex bin srec map link o))
    {
        my $fn = "$basename.$ext";
        if (-f $fn)
        {
            print "  erasing $fn\n";
            if (!unlink($fn))
            {
                print "$0: ERROR: failed to delete temporary file $fn: $!\n";
                $success = 0;
            }
        }
    }
    print "\n";
    return $success;
}


sub createEmptyDSKFile($)
{
    my ($dskFilename) = @_;

    die unless defined $dskFilename;

    my $file;
    if (!open($file, "> $dskFilename"))
    {
        die;
    }
    print $file chr(255) x (35*18*256);
    if (!close($file))
    {
        die;
    }

    return 1;
}


sub writeBasicDriver($$$)
{
    my ($dskFilename, $firstTestNum, $lastTestNum) = @_;

    die unless $firstTestNum <= $lastTestNum;

    print "\n";
    print "Writing Basic driver for tests $firstTestNum to $lastTestNum.\n";

    my $basFile;
    if (!open($basFile, "> runtests.bas"))
    {
        die;
    }

    my $contents = <<__EOF__;  # initial empty line is required by DECB's LOAD

10 ONERR GOTO140
20 POKE111,254 'STDOUT TO SERIAL
30 FOR I=$firstTestNum TO $lastTestNum
40 F\$=MID\$(STR\$(I),2)
50 F\$="TEST"+RIGHT\$("000"+F\$,4)+".BIN"
60 PRINT:PRINT"======== "F\$
70 POKE111,254 'PRINTING SOMETHING RESTORES STDOUT TO SCREEN, SOMEHOW
80 LOADM F\$
90 POKE111,254 'SO DOES LOADM
100 EXEC
110 NEXTI
120 PRINT:PRINT"******** NORMAL END"
130 GOTO150
140 PRINT:PRINT"******** ERROR"
150 POKE111,0 'STDOUT TO SCREEN
160 END
__EOF__

    print "\n";
    print $contents;
    print "\n";

    #$contents =~ s/\n/\r/gs;  # not needed: --format=ascii makes this conversion

    print $basFile $contents;

    if (!close($basFile))
    {
        die;
    }

    my $cmd = sprintf("./writecocofile --format=ascii %s runtests.bas", $dskFilename);
    print "$cmd\n";
    system($cmd) == 0 or die;
}


###############################################################################


push @includeDirList, "$srcdir/stdlib";


print "$0: ", scalar(@testCaseList), " programs to test.\n";
print "\n";

my @failedTestNumbers = ();

for my $i (@testNumbers)
{
    if (!runTestNumber($i))
    {
        push @failedTestNumbers, $i;
        last if $stopOnFail;
    }
}

if ($cleanUp)
{
    cleanUp(",check-prog") or exit 1;
}

if ($generateCoCoBinary)
{
    # Create .DSK images from the TEST*.BIN files.
    #
    print "Creating .dsk images from the test*.bin files.\n";
    my $granulesPerDisk = 68;
    my $granuleSizeInBytes = 9 * 256;
    my $granulesForTests = $granulesPerDisk - 1;  # leave 1 granule for Basic driver
    my $numTests = 0;
    my $numDisks = 0;
    my $dskFilename;
    my $firstTestNumCurDisk = $testNumbers[0];
    my $granulesAvail = 0;
    for my $i (@testNumbers)
    {
        my $testFilename = sprintf("test%04u.bin", $i);
        my $testFileSizeInBytes = -s $testFilename;
        die unless $testFileSizeInBytes > 0;
        my $testFileSizeInGranules = ceil($testFileSizeInBytes / $granuleSizeInBytes);

        if ($granulesAvail < $testFileSizeInGranules)  # if current disk full
        {
            if (defined $dskFilename)  # if disk image exists
            {
                writeBasicDriver($dskFilename, $firstTestNumCurDisk, $i - 1);
            }

            $dskFilename = sprintf("suite%03u.dsk", $numDisks);
            print "\n";
            print "Creating disk image: $dskFilename\n";
            createEmptyDSKFile($dskFilename) or die;

            ++$numDisks;
            $granulesAvail = $granulesForTests;
            $firstTestNumCurDisk = $i;
        }

        my $cmd = sprintf("./writecocofile %s %s", $dskFilename, $testFilename);
        print "$cmd\n";
        system($cmd) == 0 or die;

        $granulesAvail -= $testFileSizeInGranules;

        ++$numTests;
    }

    if ($dskFilename)
    {
        # Create new .DSK image.
        $dskFilename = sprintf("suite%03u.dsk", $numDisks - 1);
        writeBasicDriver($dskFilename, $firstTestNumCurDisk, $#testNumbers);
    }
}
else
{
    if (@failedTestNumbers > 0)
    {
        print "$0: ", scalar(@failedTestNumbers), " tests (#",
              join(", #", @failedTestNumbers),
              ") failed out of ", scalar(@testNumbers), "\n";
        exit 1;
    }

    print "$0: ALL ", scalar(@testNumbers), " tests PASSED\n";

    print <<__EOF__;

         ###   #   #   ###    ###   #####   ###    ###
        #   #  #   #  #   #  #   #  #      #   #  #   #
        #      #   #  #      #      #      #      #
         ###   #   #  #      #      ###     ###    ###
            #  #   #  #      #      #          #      #
        #   #  #   #  #   #  #   #  #      #   #  #   #
         ###    ###    ###    ###   #####   ###    ###

__EOF__
}


exit 0;
