#!/usr/bin/perl -w

use strict;
use Getopt::Long;
use File::Basename;


my $cFilename = ",check-prog.c";
my $numDebuggingLines = 0;
my $monolithMode = 0;  # true means old, non-linker mode


my @testCaseList =
(


{
title => q{Empty source file},
suspended => 1,  # Suspended b/c Mac OS X cpp appears to generate 1-line output, so error line no is 2 instead of 1.  
program => q!!,
expected => [
    qq!lwlink: __error__: External symbol program_start not found!,
    qq!lwlink: __error__: Cannot resolve exec address 'program_start'!,
    ]
},


{
title => q{Using a local variable before declaring it},
program => q!
    int main()
    {
        a = 1;
        int a;
        return 0;
    }
!,
expected => [
    qq!$cFilename:4: __error__: undeclared identifier `a'!,
    qq!$cFilename:4: __error__: left side of operator = is of type void!,
    ]
},


{
title => q{Using a global variable before declaring it},
program => q!
    void f()
    {
        g = 1;
    }
    int g;
    int main()
    {
        f();
        return g;
    }
!,
expected => [
    qq!,check-prog.c:4: __error__: global variable `g' undeclared!,
    ]
},


{
title => q{Non-void not returning a value},
program => q!
    int main() { return 0; }
    int f() {}
    int g() { f(); }
    void h() { return 42; }
    int i() { return; }
    asm int a() { asm { ldd #1844 } }  // OK because function is asm only 
    !,
expected => [ qq!,check-prog.c:3: __warning__: function 'f' is not void but does not have any return statement!,
              qq!,check-prog.c:4: __warning__: function 'g' is not void but does not have any return statement!,
              qq!,check-prog.c:5: __error__: returning expression of type `int', which differs from function's return type (`void')!,
              qq!,check-prog.c:6: __error__: return without argument in a non-void function!,
            ]
},


{
title => q{Initializing pointer from non-zero constant},
program => q!
    int main() 
    { 
        char *p0 = -42; 
        char *p1 = 42;
        char *p2 = f();
        return 0; 
    }
    int f() { return 0; }
!,
expected => [
    qq!,check-prog.c:4: __warning__: initializing pointer 'p0' from negative constant!,
    qq!,check-prog.c:6: __warning__: initializing pointer 'p2' from integer expression!,
    qq!,check-prog.c:6: __error__: calling undeclared function f()!,
    ]
},


{
title => q{Declaring a variable with a name that is already used},
program => q!
    int foo;
    int foo;
    int main()
    {
        char a;
        int a;
        return 0;
    }
    void f0(int n)
    {
        int n = 42;  // fails b/c C views function param as part of function scope, not separate scope
    }
    void f1(int n)
    {
        {
            int n = 42;  // OK b/c inner braces create separate scope
        }
    }
    !,
expected => [ qq!,check-prog.c:3: __error__: global variable `foo' already declared at global scope at ,check-prog.c:2!,
              qq!,check-prog.c:7: __error__: variable `a' already declared in this scope at ,check-prog.c:6!,
              qq!,check-prog.c:12: __error__: variable `n' already declared in this scope at ,check-prog.c:10!,
            ]
},


{
title => q{Detect invalid subtractions},
program => q!
    void f(int x) {}
    int main()
    {
        char *p;
        f((int) (4 - p));
        return 0;
    }
    !,
expected => [ qq!,check-prog.c:6: __error__: subtraction of pointer or array from integral! ]
},


{
title => q{Illegal array operations},
program => q!
    int main()
    {
        int w[2];
        w = 0;
        w += 0;
        ++w;
        w--;
        *w = 0;
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:5: __error__: cannot assign to array name!,
    qq!,check-prog.c:6: __error__: cannot assign to array name!,
    qq!,check-prog.c:7: __error__: cannot increment array name!,
    qq!,check-prog.c:8: __error__: cannot decrement array name!,
    ]
},


{
title => q{Passing undefined struct by value to a function},
program => q!
    struct Foobar { char b; };
    void f(struct Foobar s)
    {
    }
    void f1(struct Foobar)
    {
    }
    void f2(struct Undefined)
    {
    }
    struct Quux {}; union U { char u; };
    void f3(struct Quux) {}
    int main()
    {
        struct Foobar foobar;
        f(foobar);
        f1(foobar);
        f3(foobar);
        int n = foobar;
        n += foobar;
        n = -foobar;
        union U u0;
        n = -u0; 
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:9: __error__: argument 1 of f2() receives undefined `struct Undefined' by value!,
    qq!,check-prog.c:19: __error__: `struct Foobar' used as parameter 1 of function f3() which is `struct Quux'!,
    qq!,check-prog.c:21: __error__: assigning `struct Foobar' to `int'!,
    qq!,check-prog.c:21: __error__: invalid use of += on a struct or union!,
    qq!,check-prog.c:22: __error__: invalid use of arithmetic negation on a struct!,
    qq!,check-prog.c:24: __error__: invalid use of arithmetic negation on a union!,
    qq!,check-prog.c:20: __error__: using `struct Foobar' to initialize `int'!,
    ]
},


{
title => q{Forbid requesting a struct by value from va_arg(). Detect use of unknown struct name},
program => q!

    #include <stdarg.h>

    struct S { char b; };
    void f(char *a, ...)
    {
        va_list ap;
        va_start(ap, a);  // ok
        va_arg(ap, struct S);  // passes but will not work
        va_arg(ap, struct S *);  // ok
        va_arg(ap, struct Unknown *);  // ok because sizeof gives 2, regard of unknown struct name
        va_arg(ap, void);  // error
        va_arg(ap, char);  // ok
        va_arg(ap, char *);  // ok
        va_arg(ap, char **);  // ok
        va_arg(ap, char *****);  // ok
        va_end(ap);
    }
    int main()
    {
        f(0);
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:13: __error__: indirection of a pointer to void!,
    ]
},


{
title => q{Do-while lacks terminating semi-colon..},
program => q!
    void f()
    {
        do {} while (1)
    }
    int main()
    {
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:5: __error__: syntax error: }!,
    ]
},


{
title => q{Global variable too large},
program => q!
    int w[50000];
    int main()
    {
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:2: __error__: invalid dimensions for array `w'!,
    ]
},


{
title => q{Local variable too large},
program => q!
    int main()
    {
        char b[40000];
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:4: __error__: invalid dimensions for array `b'!,
    ]
},


{
title => q{Number of arguments of built-in functions},
program => q!
    #include <cmoc.h>
    int main()
    {
        putstr("foo");
        int w;
        toupper(w);
        char *s0;
        strchr(s0, 256 + 'f');
        printf();
        printf(s0);
        sprintf();
        sprintf("x");
        sprintf("%d %d", 1, 2);
        sprintf(w, "x");
        char temp[64];
        sprintf(temp, "%d %d", 1);
        sprintf(temp, "%d %d", 1, 2);  // OK
        sprintf(temp, "%d %d", 1, 2, 3);
        printf("%d %d", 1);
        printf("%d %d", 1, 2);  // OK
        printf("%d %d", 1, 2, 3);
        printf(w);
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:5: __error__: call to putstr() passes 1 argument(s) but function expects 2!,
    qq!,check-prog.c:10: __error__: call to printf() passes 0 argument(s) but function expects at least 1!,
    qq!,check-prog.c:11: __warning__: format argument of printf() is not a string literal!,
    qq!,check-prog.c:12: __error__: call to sprintf() passes 0 argument(s) but function expects at least 2!,
    qq!,check-prog.c:13: __error__: call to sprintf() passes 1 argument(s) but function expects at least 2!,
    qq!,check-prog.c:14: __warning__: `const char[]' used as parameter 1 (dest) of function sprintf() which is `char *' (not const-correct)!,
    qq!,check-prog.c:14: __warning__: first argument of sprintf() is a string literal!,
    qq!,check-prog.c:15: __warning__: passing non-pointer/array (int) as parameter 1 (dest) of function sprintf(), which is `char *`!,
    qq!,check-prog.c:15: __warning__: first argument of sprintf() should be pointer or array instead of `int'!,
    qq!,check-prog.c:17: __warning__: not enough arguments to sprintf() to match its format string!,
    qq!,check-prog.c:19: __warning__: too many arguments for sprintf() format string!,
    qq!,check-prog.c:20: __warning__: not enough arguments to printf() to match its format string!,
    qq!,check-prog.c:22: __warning__: too many arguments for printf() format string!,
    qq!,check-prog.c:23: __warning__: passing non-pointer/array (int) as parameter 1 (format) of function printf(), which is `const char *`!,
    qq!,check-prog.c:23: __warning__: format argument of printf() is not a string literal!,
    ]
},


{
title => q{Detect change in signedness when passing an argument to a function},
program => q!
    #include <cmoc.h>
    void fsb(char x) {}
    void fsw(int x) {}
    void fub(unsigned char x) {}
    void fuw(unsigned x) {}
    int main()
    {
        char sb;
        int sw;
        unsigned char ub;
        unsigned uw;
        fsb(ub);
        fsw(uw);
        fub(sb);
        fuw(sw);
        memset((unsigned char *) 0, sb, sw);
        fsw(sw + ub);  // no warning because sum is signed int (see ExpressionTypeSetter::processBinOp())
        return 0;
    }
    !,
expected => [
    #qq!,check-prog.c:14: __warning__: passing unsigned argument as signed parameter 1 of function fsw()!,
    #qq!,check-prog.c:16: __warning__: passing signed argument as unsigned parameter 1 of function fuw()!,
    #qq!,check-prog.c:17: __warning__: passing signed argument as unsigned parameter 3 of function memset()!,
    ]
},


{
title => q{Return value type vs. function's return type},
program => q!
    unsigned char *b1()     { return 1; }
    unsigned char *bm1()    { return -1; }
    unsigned char *b1000()  { return 1000; }
    unsigned char *bm1000() { return -1000; }
    unsigned *w1()     { return 1; }
    unsigned *wm1()    { return -1; }
    unsigned *w1000()  { return 1000; }
    unsigned *wm1000() { return -1000; }
    void *v0() { return (char *) 0; }  // OK
    void *v1() { return (const char *) 0; }  // bad: not const-correct
    char *v2() { return (const char *) 0; }  // bad: not const-correct
    const char *v3() { return (char *) 0; }  // OK
    int main()
    {
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:2: __error__: returning expression of type `int', which differs from function's return type (`unsigned char *')!,
    qq!,check-prog.c:3: __error__: returning expression of type `int', which differs from function's return type (`unsigned char *')!,
    qq!,check-prog.c:4: __error__: returning expression of type `int', which differs from function's return type (`unsigned char *')!,
    qq!,check-prog.c:5: __error__: returning expression of type `int', which differs from function's return type (`unsigned char *')!,
    qq!,check-prog.c:6: __error__: returning expression of type `int', which differs from function's return type (`unsigned int *')!,
    qq!,check-prog.c:7: __error__: returning expression of type `int', which differs from function's return type (`unsigned int *')!,
    qq!,check-prog.c:8: __error__: returning expression of type `int', which differs from function's return type (`unsigned int *')!,
    qq!,check-prog.c:9: __error__: returning expression of type `int', which differs from function's return type (`unsigned int *')!,
    qq!,check-prog.c:11: __error__: returning expression of type `const char *', which differs from function's return type (`void *')!,
    qq!,check-prog.c:12: __error__: returning expression of type `const char *', which differs from function's return type (`char *')!,
    ]
},


{
title => q{Return type of main() must be int},
program => q!
    void main()
    {
    }
    !,
expected => [
    qq!,check-prog.c:2: __warning__: return type of main() must be int!,
    ]
},


{
title => q{printf() format warnings},
program => q!
    #include <cmoc.h>
    int main()
    {
        int sw;
        unsigned uw;
        char sb;
        unsigned char ub;
        unsigned char *bp;
        printf("%u %d %5u %% %d %u %3d %p\n", sw, uw, sb, ub, bp, bp);
        printf("%");
        printf("foo %123");
        printf("%u %u", 1, 2, 3);
        printf("");
        obligatory_error;
        printf("%d", -1);  // no warning expected
        printf("%u", 1);   // no warning expected
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:15: __error__: undeclared identifier `obligatory_error'!,
    qq!,check-prog.c:10: __warning__: not enough arguments to printf() to match its format string!,
    qq!,check-prog.c:11: __warning__: no letter follows last % in printf() format string!,
    qq!,check-prog.c:12: __warning__: no letter follows last % in printf() format string!,
    qq!,check-prog.c:13: __warning__: too many arguments for printf() format string!,
    ]
},


{
title => q{Declaration type vs. initializer type},
program => q!
    int main()
    {
        unsigned w = 1844;
        unsigned *pw0 = &w;
        unsigned *pw1 = &pw0;  // unsigned ** assigned to unsigned *
        char b0 = -129;  // does not fit
        char b1 = -128;  // ok
        char b2 = 255;  // ok
        char b3 = 256;  // does not fit
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:6: __error__: using `unsigned int **' to initialize `unsigned int *'!,
    qq!,check-prog.c:7: __warning__: initializer of type `int' is too large for `char`!,
    qq!,check-prog.c:10: __warning__: initializer of type `int' is too large for `char`!,
    ]
},


{
title => q{Byte array initialized with non-byte expressions},
program => q!
    char strings[] = { "foo", "bar", "baz" };
    int main()
    {
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:2: __error__: using `const char[]' to initialize `char'!,
    qq!,check-prog.c:2: __error__: using `const char[]' to initialize `char'!,
    qq!,check-prog.c:2: __error__: using `const char[]' to initialize `char'!,
    ]
},


{
title => q{Invalid array initializer},
program => q!
    struct A { char byteField; };
    struct A structArray[3] = { { 42 }, "foo", { 44 } };
    int v0[4] = "foo";
    int v1[] = { 9999 };
    int v2[] = &v1;
    int main()
    {
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:3: __error__: initializer for struct A is of type `const char[]': must be list, or struct of same type!,
    qq!,check-prog.c:4: __error__: initializer for array `v0' is invalid!,
    qq!,check-prog.c:6: __error__: initializer for array `v2' is invalid!,
    ]
},


{
title => q{Array whose initializer contains expressions of types that do not match the element type},
program => q!
    struct A { char byteField; };
    struct A structArray[3] = { { 42 }, "foo", { 44 } };
    int v0[4] = "foo";
    int main()
    {
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:3: __error__: initializer for struct A is of type `const char[]': must be list, or struct of same type!,
    qq!,check-prog.c:4: __error__: initializer for array `v0' is invalid!,
    ]
},


{
title => q{Re: Declaration::checkClassInitializer()},
program => q!
    struct A
    {
        char b;
    }; 
    struct A a = { "foo" };
    struct B
    {
        struct A aMember;
    };
    struct B b = { { "bar" } };
    struct C
    {
        struct A aMemberArray[2];
    };
    struct C c = { {
                    { "baz"  },
                    { "quux" }
                 } };
    int main()
    {
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:6: __error__: using `const char[]' to initialize `char'!,
    qq!,check-prog.c:11: __error__: using `const char[]' to initialize `char'!,
    qq!,check-prog.c:17: __error__: using `const char[]' to initialize `char'!,
    qq!,check-prog.c:18: __error__: using `const char[]' to initialize `char'!,
    ]
},


{
title => q{var_decl containing a type_specifier but no declarator_list, unless it defines a struct},
program => q!
    struct S { char b; };  // ok
    unsigned n = 42;  // ok
    unsigned x;  // ok: declarator without initializer
    unsigned;  // fail
    int main()
    {
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:5: __error__: declaration specifies a type but no declarator name!,
    ]
},


{
title => q{Reject returning an array from an int function},
program => q!
    int i = 0;
    int f(int *p)
    {
        return p;
    }
    int g(int p[][3])
    {
        return p[i];
    }
    int h(int p[][3][3])
    {
        return p[i][i];
    }
    int h2(int p[][3][3])
    {
        return p[i];
    }
    int main()
    {
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:5: __error__: returning expression of type `int *', which differs from function's return type (`int')!,
    qq!,check-prog.c:9: __error__: returning expression of type `int[]', which differs from function's return type (`int')!,
    qq!,check-prog.c:13: __error__: returning expression of type `int[]', which differs from function's return type (`int')!,
    qq!,check-prog.c:17: __error__: returning expression of type `int[][]', which differs from function's return type (`int')!,
    ]
},


{
title => q{Calling an undeclared function, mismatching function prototype, wrong number of parameters, etc.},
program => q!
    void f2();
    void f3(int n);
    void f3();
    void f4();
    int main()
    {
        f0();
        f1();


        return 0;
    }
    void f1()
    {
    }
    void f4(int n)
    {
    }
    void f5(int a);
    void f5(int a, ...);
    !,
expected => [
    qq!,check-prog.c:4: __error__: formal parameters for f3() are different from previously declared at ,check-prog.c:3!,
    qq!,check-prog.c:17: __error__: formal parameters for f4() are different from previously declared at ,check-prog.c:5!,
    qq!,check-prog.c:21: __error__: formal parameters for f5() are different from previously declared at ,check-prog.c:20!,
    qq!,check-prog.c:8: __error__: undeclared identifier `f0'!,
    qq!,check-prog.c:8: __error__: calling undeclared function f0()!,
    qq!,check-prog.c:9: __error__: calling undeclared function f1()!,
    ]
},


{
title => q{Unknown member of a struct},
program => q!
    struct S0
    {
        int a;
    };    
    int main()
    {
        struct S0 s0;
        s0.b = 1;
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:9: __error__: struct S0 has no member named b!,
    qq!,check-prog.c:9: __error__: left side of operator = is of type void!,
    ]
},


{
title => q{Warnings on dubious function calls},
program => q!
    void f() {}
    int main()
    {
        int f;
        f();
        int pf = f;
        pf();
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:6: __warning__: calling `f', which is both a variable and a function name!,
    qq!,check-prog.c:8: __error__: function pointer call through expression of invalid type (`int')!,
    ]
},


{
title => q{switch()},
program => q!
    int f() { return 99; }
    int main()
    {

        switch (1) { case f(): return 0; }
        switch (1) { return 1; case 5: return 0; }
        char c = 1;
        switch (c) { case 500: return 1; }
        unsigned char uc = 1;
        switch (uc) { case 500: return 1; }
        switch (1) { default: break; case 5: break; default: ; } 
        switch (1) { case 5: continue; }
        switch (1)
        {
            case 3:
                ;
            case 3:  // duplicate case value
                ;
        }
        switch ((long) 0) { default: ; }
        switch ((float) 0) { default: ; }
        switch ((double) 0) { default: ; }
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:22: __warning__: floating-point arithmetic is not supported on this platform!,
    qq!,check-prog.c:23: __warning__: `double' is an alias for `float' for this compiler!,
    qq!,check-prog.c:6: __error__: case statement has a variable expression!,
    qq!,check-prog.c:7: __error__: statement in switch precedes first `case' or `default' statement!,
    qq!,check-prog.c:9: __warning__: switch expression is signed char but case value is not in range -128..127!,
    qq!,check-prog.c:11: __warning__: switch expression is unsigned char but case value is not in range 0..255!,
    qq!,check-prog.c:12: __error__: more than one default statement in switch!,
    qq!,check-prog.c:18: __error__: duplicate case value (first used at ,check-prog.c:16)!,
    qq!,check-prog.c:21: __error__: switch() expression of type `long' is not supported!,
    qq!,check-prog.c:22: __error__: switch() expression of type `float' is not supported!,
    qq!,check-prog.c:23: __error__: switch() expression of type `float' is not supported!,
    ]
},


{
title => q{Continue in a switch (not supported)},
program => q!
    int f() { return 42; }
    int main()
    {
        switch (1) { case 1: continue; }  // error
        switch (1) { case 1: while (1) continue; }  // ok
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:5: __error__: continue statement is not supported in a switch!,
    ]
},


{
title => q{Case or default statement outside of a switch()},
program => q!
    int main()
    {
        case 42: ;
        for (;;)
            default: ;
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:4: __error__: case label not within a switch statement!,
    qq!,check-prog.c:6: __error__: default label not within a switch statement!,
    ]
},



{
title => q{Invalid usage of "#pragma org"},
program => q!
    #pragma org pizza
    int main()
    {
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:2: __error__: invalid pragma directive: org pizza!,
    ]
},


{
title => q{Passing a wrong type to a function expecting a non-void pointer},
program => q!
    struct S {};
    void f(struct S *p) {}
    struct T {}; union U {};
    int main()
    {
        f(0);  // OK: null pointer
        f(3);  // tolerated, unless -Wpass-const-for-func-pointer is passed
        f(-4);  // ditto
        int n;
        f(&n);  // error: wrong type of pointer
        struct T t;
        f(&t);  // error: ditto
        struct S s;
        f(&s);  // OK
        union U u;
        f(&u);
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:11: __error__: `int *' used as parameter 1 (p) of function f() which is `struct S *'!,
    qq!,check-prog.c:13: __error__: `struct T *' used as parameter 1 (p) of function f() which is `struct S *'!,
    qq!,check-prog.c:17: __error__: `union U *' used as parameter 1 (p) of function f() which is `struct S *'!,
    ]
},


{
title => q{#pragma const_data},
suspended => 1,
program => q!
    int f() { return 42; }
    #pragma const_data start
    int lacksInitializer;
    int nonConstInitializer = f();
    int ok = 6809;
    #pragma const_data end
    int main()
    {
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:4: __error__: global variable 'lacksInitializer' defined as constant but has no initializer!,
    qq!,check-prog.c:5: __error__: global variable 'nonConstInitializer' defined as constant but has a run-time initializer!,
    ]
},


{
title => q{Referring to a member of an undefined struct},
program => q!
    int main()
    {
        struct S *p = 0;
        p->n = 42;
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:5: __error__: reference to member `n' of undefined class `S'!,
    qq!,check-prog.c:5: __error__: left side of operator = is of type void!,
    ]
},


{
title => q{Indirection of a pointer to a struct used as an r-value},
program => q!
    struct S { int field; };
    struct T { struct S *ps; };
    int main()
    {
        struct S *ps = (struct S *) 100;
        if (*ps)  // bad
            ;
        if ((*ps).field == 0)  // ok
            ;
        struct T *pt = (struct T *) 200;
        if (*(*pt).ps)  // bad
            ;
        if ((*(*pt).ps).field)  // ok
            ;
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:7: __error__: invalid use of struct as condition of if statement!,
    qq!,check-prog.c:12: __error__: invalid use of struct as condition of if statement!,
    ]
},


{
title => q{Initializing from incompatible pointer type},
options => "-Wpass-const-for-func-pointer",
program => q!
    int a[5][7][13];
    char n;
    int main()
    {
        int *p0 = a;
        p0 = a;
        int *p1 = a + n;
        p1 = a + n;
        int *p2 = a + 1;
        p2 = a + 1;
        int *p2 = n;
        p0 = n;
        p0 = -1;
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:12: __error__: variable `p2' already declared in this scope at ,check-prog.c:10!,
    qq!,check-prog.c:7: __error__: assigning `int[][][]' to `int *'!,
    qq!,check-prog.c:9: __error__: assigning `int[][][]' to `int *'!,
    qq!,check-prog.c:11: __error__: assigning `int[][][]' to `int *'!,
    qq!,check-prog.c:13: __warning__: assigning non-pointer/array (char) to `int *'!,
    qq!,check-prog.c:14: __warning__: assigning non-zero numeric constant to `int *'!,
    qq!,check-prog.c:6: __warning__: initializing `int *' (p0) from incompatible `int[][][]'!,
    qq!,check-prog.c:8: __warning__: initializing `int *' (p1) from incompatible `int[][][]'!,
    qq!,check-prog.c:10: __warning__: initializing `int *' (p2) from incompatible `int[][][]'!,
    qq!,check-prog.c:12: __warning__: initializing pointer 'p2' from integer expression!,
    ]
},


{
title => q{Assembly-only function containing non-asm statement},
program => q!
    int asm f()
    {
        int foo;  // only asm{} allowed in asm function
        asm {}
    }
    int asm g()
    {
        asm {}
        return 42;  // only asm{} allowed in asm function
    }
    int asm h(int m, char n, char *s)
    {
        asm {
            ldb n
        }
        asm {
            ldb s[12]
            ldd m
            ldb n
            ldd m
        }
        asm {
            tfr s,x     // OK: s refers to register, not to C variable
        }
        asm("clr", n);
    }
    int global0 = 42;
    void q()
    {
        asm("ldd", global0);  // OK: variable is global
        asm {
            ldd global0  // ditto
        }
    }
    int main()
    {
        int z;
        asm { nop }  // OK: no 'asm' modifier on this function
        asm("clr", z);  // ditto
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:4: __error__: body of function f() contains statement(s) other than inline assembly!,
    qq!,check-prog.c:10: __error__: body of function g() contains statement(s) other than inline assembly!,
    qq!,check-prog.c:14: __error__: assembly-only function refers to local C variable `n'!,
    qq!,check-prog.c:17: __error__: assembly-only function refers to local C variables `m', `n', `s'!,
    qq!,check-prog.c:26: __error__: assembly-only function refers to local C variable `n'!,
    ]
},


{
title => q{asm or interrupt modifiers used on non-function declarations},
program => q!
    int asm globalVar0;
    asm int globalVar1;
    int interrupt globalVar2;
    interrupt int globalVar3;
    typedef int asm Bad0;
    typedef asm int Bad1;
    typedef int interrupt Bad2;
    typedef interrupt int Bad3;
    int main()
    {
        int asm localVar0;
        asm int localVar1;
        int interrupt localVar2;
        interrupt int localVar3;
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:2: __error__: modifier `asm' cannot be used on declaration of variable `globalVar0'!,
    qq!,check-prog.c:3: __error__: modifier `asm' cannot be used on declaration of variable `globalVar1'!,
    qq!,check-prog.c:4: __error__: modifier `interrupt' used on declaration of variable `globalVar2'!,
    qq!,check-prog.c:5: __error__: modifier `interrupt' used on declaration of variable `globalVar3'!,
    qq!,check-prog.c:6: __error__: modifier `asm' cannot be used on typedef!,
    qq!,check-prog.c:7: __error__: modifier `asm' cannot be used on typedef!,
    qq!,check-prog.c:8: __error__: modifier `interrupt' cannot be used on typedef!,
    qq!,check-prog.c:9: __error__: modifier `interrupt' cannot be used on typedef!,
    qq!,check-prog.c:12: __error__: modifier `asm' cannot be used on declaration of variable `localVar0'!,
    qq!,check-prog.c:13: __error__: modifier `asm' cannot be used on declaration of variable `localVar1'!,
    qq!,check-prog.c:14: __error__: modifier `interrupt' used on declaration of variable `localVar2'!,
    qq!,check-prog.c:15: __error__: modifier `interrupt' used on declaration of variable `localVar3'!,
    ]
},


{
title => q{Illegal modifier used on declaration of variable},
program => q!
    int interrupt globalVar1;
    interrupt void isr() {} 
    int main()
    {
        int interrupt localVar;
        isr();
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:2: __error__: modifier `interrupt' used on declaration of variable `globalVar1'!,
    qq!,check-prog.c:6: __error__: modifier `interrupt' used on declaration of variable `localVar'!,
    ]
},


{
title => q{Calling interrupt service routine},
program => q!
    interrupt void isr() {} 
    int main()
    {
        isr();
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:5: __error__: calling function isr() is forbidden because it is an interrupt service routine!,
    ]
},


{
title => q{Struct with field of undefined struct},
program => q!
    struct Outer
    {
        struct Inner i;
    }; 
    int main()
    {
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:4: __error__: member `i' of `Outer' is of undefined type `Inner'!,
    ]
},


{
title => q{union vs. struct},
program => q!
    union U
    {
        int i;
    };
    struct S
    {
        int i;
    };
    struct Cell;
    struct List
    {
        struct Cell *a, *b;
    };
    union UndefUnion;
    struct Cell fc();
    int main()
    {
        struct U u;
        union S s;
        struct Cell c;
        union UndefUnion uu;
        struct Cell *pc;  // OK
        union UndefUnion *puu;  // OK
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:16: __error__: declaring `fc' of undefined type struct `Cell'!,
    qq!,check-prog.c:19: __error__: referring to U as a struct, but it is a union!,
    qq!,check-prog.c:20: __error__: referring to S as a union, but it is a struct!,
    qq!,check-prog.c:21: __error__: declaring `c' of undefined type struct `Cell'!,
    qq!,check-prog.c:22: __error__: declaring `uu' of undefined type struct `UndefUnion'!,
    ]
},


{
title => q{Receiving an undefined aggregate type by value},
program => q!
    struct Cell;
    union U;
    void fc1(struct Cell c);
    void fc2(struct Cell c) {}
    void fc3(union U u);
    void fc4(union U u) {}
    int main()
    {
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:4: __error__: argument 1 of fc1() receives undefined `struct Cell' by value!,
    qq!,check-prog.c:5: __error__: argument 1 of fc2() receives undefined `struct Cell' by value!,
    qq!,check-prog.c:6: __error__: argument 1 of fc3() receives undefined `union U' by value!,
    qq!,check-prog.c:7: __error__: argument 1 of fc4() receives undefined `union U' by value!,
    ]
},


{
title => q{Forbid --org and --data when targeting OS-9},
target => "os9",
linkerModeOnly => 1,
options => "--org=0x300 --data=0x400",
program => q!
    int main()
    {
        return 0;
    }
    !,
expected => [
    qq!cmoc: --org and --data are not permitted when targetting OS-9!,
    ]
},


{
title => q{Forbid --org and --data when targeting Vectrex},
target => "vectrex",
linkerModeOnly => 1,
options => "--org=0x300 --data=0x400",
program => q!
    int main()
    {
        return 0;
    }
    !,
expected => [
    qq!cmoc: --org and --data are not permitted when targetting Vectrex!,
    ]
},


{
title => q{Forbid code and data positioning pragmas when targeting Vectrex},
target => "vectrex",
linkerModeOnly => 1,
program => q!
    #pragma org  0x100
    #pragma data 0x200
    int main()
    {
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:2: __error__: #pragma org is not permitted for Vectrex!,
    qq!,check-prog.c:3: __error__: #pragma data is not permitted for Vectrex!,
    ]
},


{
title => q{Function prototype local to a function body},
program => q!
    void f() {}
    int g()
    {
        char *f();
        return 1;  // check that this JumpStmt is seen as inside g()
    }
    int main()
    {
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:5: __error__: invalid declaration!,
    ]
},


{
title => q{Correct line number in error message when using function address to initialize char},
program => q!
    char f() { return 42; }
    int main()
    {
        char b = f;
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:5: __error__: using `char (*)()' to initialize `char'!,
    ]
},


{
title => q{sizeof on an unknown struct name},
program => q!
    struct S {}; 
    unsigned g0 = sizeof(struct Foo);
    unsigned g1 = sizeof(struct S);
    int main()
    {
        unsigned a = sizeof(struct Foo);
        unsigned b;
        b = sizeof(struct Foo);
        b = sizeof(struct S);
        b = sizeof(struct Bar);
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:3: __error__: cannot take sizeof unknown struct or union 'Foo'!,
    qq!,check-prog.c:7: __error__: cannot take sizeof unknown struct or union 'Foo'!,
    qq!,check-prog.c:9: __error__: cannot take sizeof unknown struct or union 'Foo'!,
    qq!,check-prog.c:11: __error__: cannot take sizeof unknown struct or union 'Bar'!,
    ]
},


{
title => q{sizeof on an unknown array name, used twice in a binary expression},
program => q!
    int main()
    {
        unsigned n = sizeof(a) / sizeof(a[0]);
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:4: __error__: undeclared identifier `a'!,
    qq!,check-prog.c:4: __error__: undeclared identifier `a'!,
    qq!,check-prog.c:4: __error__: argument of sizeof operator is of type void!,
    qq!,check-prog.c:4: __error__: left side of operator [] is of type void!,
    qq!,check-prog.c:4: __error__: array reference on non array or pointer!,
    qq!,check-prog.c:4: __error__: argument of sizeof operator is of type void!,
    qq!,check-prog.c:4: __error__: l-value required as left operand of array reference!,
    ]
},


{
title => q{Passing integer for pointer parameter},
program => q!
    int g() { return 42; }
    void f(int *p) {}
    int main()
    {
        f(g());
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:6: __warning__: passing non-pointer/array (int) as parameter 1 (p) of function f(), which is `int *`!,
    ]
},


{
title => q{Invalid numerical constant},
suspended => 1,
program => q!
    unsigned f0() { return 0777777; }
    unsigned f1() { return 0xFFFFF; }
    unsigned f2() { return 99999; }
    int f3() { return -99999; }
    int main()
    {
        return (int) (f0() + f1() + f2() + f3());
    }
    !,
expected => [
    qq!,check-prog.c:2: __error__: invalid numerical constant 262143.000000 (must be 16-bit integer)!,
    qq!,check-prog.c:3: __error__: invalid numerical constant 1048575.000000 (must be 16-bit integer)!,
    qq!,check-prog.c:4: __error__: invalid numerical constant 99999.000000 (must be 16-bit integer)!,
    qq!,check-prog.c:5: __error__: invalid numerical constant 99999.000000 (must be 16-bit integer)!,
    ]
},


{
title => q{Dereferencing a void pointer},
program => q!
    void g(int n) {}
    void f(void *p)
    {
        g(*p);
    }
    int main()
    {
        f(0);
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:5: __error__: `void' used as parameter 1 (n) of function g() which is `int'!,
    qq!,check-prog.c:5: __error__: indirection of a pointer to void!,
    ]
},


{
title => q{Use of a struct as an r-value},
program => q`
    struct Inner { int n; };
    struct Outer { struct Inner i; };
    int main()
    {
        struct Outer obj;
        if (obj)
            return 1;
        while (obj)
            return 4;
        for ( ; obj ; ) {}
        if (obj.i)
            return 2;
        while (obj.i)
            return 5;
        if (!obj.i)
            return 3;
        while (!obj.i)
            return 6;
        return 0;
    }
    `,
expected => [
    qq!,check-prog.c:16: __error__: invalid use of boolean negation on a struct!,
    qq!,check-prog.c:18: __error__: invalid use of boolean negation on a struct!,
    qq!,check-prog.c:7: __error__: invalid use of struct as condition of if statement!,
    qq!,check-prog.c:9: __error__: invalid use of struct as condition of while statement!,
    qq!,check-prog.c:11: __error__: invalid use of struct as condition of for statement!,
    qq!,check-prog.c:12: __error__: invalid use of struct as condition of if statement!,
    qq!,check-prog.c:14: __error__: invalid use of struct as condition of while statement!,
    ]
},


{
title => q{Anonymous struct},
program => q!
    struct { int n; } a;
    int main()
    {
        int *pi = &a;
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:5: __error__: using `struct AnonStruct_,check-prog.c:2 *' to initialize `int *'!,
    ]
},


{
title => q{goto and ID-labeled statements},
program => q!
    void f()
    {
    foo: ; // ok
    bar: ;
    }
    int main()
    {
    foo:
    foo:
        f();
        goto bar;
        goto imaginary;
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:10: __error__: label `foo' already defined at ,check-prog.c:9!,
    qq!,check-prog.c:12: __error__: goto targets label `bar' which is unknown to function main()!,
    qq!,check-prog.c:13: __error__: goto targets label `imaginary' which is unknown to function main()!,
    ]
},


{
title => q{Conditional with incompatible types},
program => q!
    void g(int) {}
    void h(long) {}
    int main()
    {
        char *p;
        int i;
        g(i > 0 ? p : i);
        g(i > 0 ? (char) 0x12 : i);
        unsigned char ub = 42;
        g(i > 0 ? 256 : ub);  // warning because 256 is signed
        unsigned ui = 4242;        
        g(i > 0 ? 256U : ui);  // no warning
        long dw;
        h(i ? dw : 0);
        return 0;
    }
    char a[] = { 1, 2, 3 };
    char *getArray() { return a; }
    char f(char condition)
    {
        char *p = (condition ? getArray() : a);  // OK because char * and char[] are close enough
        return *p; 
    }
    !,
expected => [
    qq!,check-prog.c:8: __error__: true and false expressions of conditional are of incompatible types (char * vs int)!,
    qq!,check-prog.c:9: __warning__: true and false expressions of conditional are not of the same type (char vs int); result is of type int!,
    qq!,check-prog.c:11: __warning__: true and false expressions of conditional are not of the same type (int vs unsigned char); result is of type int!,
    qq!,check-prog.c:15: __warning__: true and false expressions of conditional are not of the same type (long vs int); result is of type long!,
    ]
},


{
title => q{Automatic type conversions},
program => q!
    unsigned char returnByte(unsigned char b)
    {
        return b;
    }
    int main()
    {
        int i = 1000;
        char c;
        c = i;
        c = 2000;
        unsigned u = 1001;
        unsigned char uc;
        uc = u;
        uc = 2001; 

        u = 100;
        u = (u > 99 ? 50 : u);
        unsigned char b = 12;
        u = (u > 66 ? u : returnByte(b));
        u = 999;
        u = (u < 2000 ? u : returnByte(b));
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:10: __warning__: assigning to `char' from larger type `int'!,
    qq!,check-prog.c:11: __warning__: assigning to `char' from larger type `int'!,
    qq!,check-prog.c:14: __warning__: assigning to `unsigned char' from larger type `unsigned int'!,
    qq!,check-prog.c:15: __warning__: assigning to `unsigned char' from larger type `int'!,
    qq!,check-prog.c:18: __warning__: true and false expressions of conditional are not of the same type (int vs unsigned int); result is of type int!,
    qq!,check-prog.c:20: __warning__: true and false expressions of conditional are not of the same type (unsigned int vs unsigned char); result is of type unsigned int!,
    qq!,check-prog.c:22: __warning__: true and false expressions of conditional are not of the same type (unsigned int vs unsigned char); result is of type unsigned int!,
    ]
},


{
title => q{Wrong use of assignment operator with structs},
program => q!
    struct S { int i; };
    struct Other {} other;
    void f(const char *, ...) {}
    int main()
    {
        struct S s, t;
        int i;
        s = 'x';
        s = i;
        s = &i;
        i = s;
        s += t;
        f("", s = t);
        struct S u = 42;
        struct S v = other;
        struct S w = &other;
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:9: __error__: assigning `char' to `struct S'!,
    qq!,check-prog.c:10: __error__: assigning `int' to `struct S'!,
    qq!,check-prog.c:11: __error__: assigning `int *' to `struct S'!,
    qq!,check-prog.c:12: __error__: assigning `struct S' to `int'!,
    qq!,check-prog.c:13: __error__: invalid use of += on a struct or union!,
    qq!,check-prog.c:15: __error__: initializer for struct S is of type `int': must be list, or struct of same type!,
    qq!,check-prog.c:16: __error__: initializer for struct S is of type `struct Other': must be list, or struct of same type!,
    qq!,check-prog.c:17: __error__: initializer for struct S is of type `struct Other *': must be list, or struct of same type!,
    ]
},


{
title => q{Static keyword},
program => q!
    static int g0;
    int g0;  // duplicates previous declaration
    int g1;
    static int g1;  // duplicates previous declaration
    static int g2;
    extern int g2;
    int main()
    {
        extern int g3;
        static int g4;  // local static not supported
        g4 = 1;
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:3: __error__: global variable `g0' already declared at global scope at ,check-prog.c:2!,
    qq!,check-prog.c:5: __error__: global variable `g1' already declared at global scope at ,check-prog.c:4!,
    qq!,check-prog.c:11: __error__: local static variables are not supported!,
    ]
},


{
title => q{More than one formal parameter with same name},
program => q!
    void f1(int a, int b, int a);
    void f2(int a, int b, int a) {}
    void f3(int a, int, char a) {}
    void f4(int, int, char);  // OK
    int main()
    {
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:2: __error__: function f1() has more than one formal parameter named 'a'!,
    qq!,check-prog.c:3: __error__: function f2() has more than one formal parameter named 'a'!,
    qq!,check-prog.c:4: __error__: function f3() has more than one formal parameter named 'a'!,
    ]
},


{
title => q{Returning wrong struct pointer type},
program => q{
    struct A {};
    struct B {} b;
    struct A *f()
    {
        return &b;
    }
    int main()
    {
        return f() != 0;
    }
    },
expected => [
    qq!,check-prog.c:6: __error__: returning expression of type `struct B *', which differs from function's return type (`struct A *')!,
    ]
},


{
title => q{Bad type specifier combinations},
program => q!
    int main()
    {
        int char x;
        signed void p;
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:4: __error__: combining type specifiers is not supported!,
    qq!,check-prog.c:5: __error__: signed and unsigned modifiers can only be applied to integral type!,
    ]
},


{
title => q{Typedef local to a function},
suspended => 1,  # This test is suspended because the compiler does not detect this error.
program => q!
    int main()
    {
        typedef int Integer;
        Integer n = 42;
        return n;
    }
    !,
expected => [
    qq!!,
    ]
},


{
title => q{enum},
program => q!
    enum A { A0 };
    enum A { A1 };  // error: 'A' already used

    enum B { DuplicateEnumeratedName };


    void f0(enum { EnumNameInFunctionParam } e) {}  // error: enumerator in formal param not supported
    void f5(enum { OtherEnumNameInFunctionParam } e);  // error: same, but on prototype

    enum { EnumNameInReturnType } f1() { return 0; }  // error: enumerator in return type
    enum { OtherEnumNameInReturnType } f2();  // error: same, but on prototype

    enum D { D0 };
    enum D f3() { return D0; }  // ok
    enum D f4();  // ok

    signed enum F { F0 } f0;
    
    void funcTakingA(enum A a) {}

    int main()
    {
        enum { X } localEnumVar;
        enum E { E0 } otherLocalEnumVar;
        funcTakingA(A0);  // ok
        funcTakingA(5);   // error: 5 not member of enum A
        funcTakingA(D0);  // error: D0 not member of enum A
        return 0;
    }
    enum C { DuplicateEnumeratedName };  // error re: enum B
    !,
expected => [
    qq!,check-prog.c:3: __error__: enum `A' already defined at ,check-prog.c:2!,
    qq!,check-prog.c:8: __error__: enum with enumerated names is not supported in a function's formal parameter!,
    qq!,check-prog.c:9: __error__: enum with enumerated names is not supported in a function's formal parameter!,
    qq!,check-prog.c:11: __error__: enum with enumerated names is not supported in a function's return type!,
    qq!,check-prog.c:12: __error__: enum with enumerated names is not supported in a function prototype's return type!,
    qq!,check-prog.c:18: __error__: signed and unsigned modifiers cannot be applied to an enum!,
    qq!,check-prog.c:31: __error__: enumerated name `DuplicateEnumeratedName' already defined at ,check-prog.c:5!,
    ]
},


{
title => q{enum, bis},
program => q!
    enum A { A0 };

    enum D { D0 };
    enum D f3() { return D0; }  // ok
    enum D f4();  // ok

    void funcTakingA(enum A a) {}

    int main()
    {
        enum { X } localEnumVar;
        enum E { E0 } otherLocalEnumVar;
        funcTakingA(A0);  // ok
        funcTakingA(5);   // error: 5 not member of enum A
        funcTakingA(D0);  // error: D0 not member of enum A
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:15: __error__: parameter 1 of function funcTakingA() must be a member of enum A!,
    qq!,check-prog.c:16: __error__: `D0' used as parameter 1 of function funcTakingA() but is not a member of enum A!,
    qq!,check-prog.c:12: __error__: non-global enum not supported!,
    qq!,check-prog.c:13: __error__: non-global enum not supported!,
    ]
},


{
title => q{Declaring an enum variable without the enum keyword},
program => q!
    enum E { E0 };
    E e;  // error
    int main()
    {
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:3: __error__: syntax error: E!,
    ]
},


{
title => q{Using an enum name, defined by multiplying two integers, before having declared that name},
program => q!
    enum
    {
        S = 75,
        E = 1000,
        B = E,
        T = B - S,
        G = T - P,
        P = 4 * 2000
    };
    int main()
    {
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:8: __error__: right side of operator - is of type void!,
        # Not a good error message: we should mention that "P" is undeclared.
    ]
},


{
title => q{String numerical escape sequence out of range},
program => q!
    int main()
    {
        const char *s0 = "\x80bomber";  // error: $80b > 255
        const char *s1 = "zzz\0777zzz";  // error: 0777 > 255
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:4: __warning__: hex escape sequence out of range!,
    qq!,check-prog.c:5: __warning__: octal escape sequence out of range!,
    ]
},


{
title => q{Named argument required before ellipsis of variadic function declaration or definition},
program => q!
    void functionWithNoNamedArg(...) {}
    void prototypeWithNoNamedArg(...);
    int main()
    {
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:2: __error__: function functionWithNoNamedArg() uses `...' but has no named argument before it!,
    qq!,check-prog.c:3: __error__: prototype prototypeWithNoNamedArg() uses `...' but has no named argument before it!,
    ]
},


{
title => q{Named argument required before ellipsis of variadic function pointer variable},
program => q!
    int main()
    {
        void (*fp)(...);
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:4: __error__: named argument is required before `...'!,
    ]
},


{
title => q{l-value required for some operators},
program => q!
    void f(unsigned *n) {}
    char g() { return 1; }
    int *h() { return 0; }
    int main()
    {
        unsigned n = 891;
        f(&(n >> 1));
        ++(n >> 1);
        --(n >> 1);
        (n >> 1)++;
        (n >> 1)--;
        char a, b;
        (g() ? a : b) = 42;  // OK because both alternatives are l-values
        (g() ? a : 99) = 42;
        (g() ? 99 : b) = 42;
        h()[0] = 0;  // OK
        *h() = 0;  // OK
        ((int *) 0x400)[32] = 0;  // OK
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:8: __error__: l-value required as operand of address-of!,
    qq!,check-prog.c:9: __error__: l-value required as operand of pre-increment!,
    qq!,check-prog.c:10: __error__: l-value required as operand of pre-decrement!,
    qq!,check-prog.c:11: __error__: l-value required as operand of post-increment!,
    qq!,check-prog.c:12: __error__: l-value required as operand of post-decrement!,
    qq!,check-prog.c:15: __error__: l-value required as left operand of assignment!,
    qq!,check-prog.c:16: __error__: l-value required as left operand of assignment!,
    ]
},


{
title => q{Various array errors},
program => q!
    int main()
    {
        int a0[];
        int a1[5][];
        int a3["foo"];
        int n = 5;
        int a4[n];
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:4: __warning__: array `a0' assumed to have one element!,
    qq!,check-prog.c:5: __error__: array a1: dimension other than first one is unspecified!,
    qq!,check-prog.c:6: __error__: pointer or array expression used for size of array `a3'!,
    qq!,check-prog.c:8: __error__: invalid size expression for dimension 1 of array `a4'!,
    ]
},


{
title => q{-Wsign-compare},
options => "-Wsign-compare",
program => q!
    char f(unsigned char uc, signed char sc)
    {
        if (uc < sc)
            return 1;
        if (uc <= sc)
            return 2;
        if (uc > sc)
            return 3;
        if (uc >= sc)
            return 4;
        if (sc < uc)
            return 1;
        if (sc <= uc)
            return 2;
        if (sc > uc)
            return 3;
        if (sc >= uc)
            return 4;
        if (uc >= uc)
            return 5;
        if (sc >= sc)
            return 5;
        return 0;
    }
    int main()
    {
        f(0, 0);
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:4: __warning__: comparison of integers of different signs (`unsigned char' vs `char'); using unsigned comparison!,
    qq!,check-prog.c:6: __warning__: comparison of integers of different signs (`unsigned char' vs `char'); using unsigned comparison!,
    qq!,check-prog.c:8: __warning__: comparison of integers of different signs (`unsigned char' vs `char'); using unsigned comparison!,
    qq!,check-prog.c:10: __warning__: comparison of integers of different signs (`unsigned char' vs `char'); using unsigned comparison!,
    qq!,check-prog.c:12: __warning__: comparison of integers of different signs (`char' vs `unsigned char'); using unsigned comparison!,
    qq!,check-prog.c:14: __warning__: comparison of integers of different signs (`char' vs `unsigned char'); using unsigned comparison!,
    qq!,check-prog.c:16: __warning__: comparison of integers of different signs (`char' vs `unsigned char'); using unsigned comparison!,
    qq!,check-prog.c:18: __warning__: comparison of integers of different signs (`char' vs `unsigned char'); using unsigned comparison!,
    ]
},


{
title => q{Without -Wsign-compare},
program => q!
    char f(unsigned char uc, signed char sc)
    {
        if (uc < sc)
            return 1;
        if (uc <= sc)
            return 2;
        if (uc > sc)
            return 3;
        if (uc >= sc)
            return 4;
        return 0;
    }
    int main()
    {
        f(0, 0);
        return 0;
    }
    !,
expected => [
    ]
},


{
title => q{Invalid use of void expression},
program => q!
    void f(int n) {}
    int main()
    {
        char *cp;
        cp[5] = 0;
        void *vp;
        vp[5] = 0;
        *vp = 0;
        f(vp[7]);
        f(*vp);
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:8: __error__: left side of operator = is of type void!,
    qq!,check-prog.c:9: __error__: left side of operator = is of type void!,
    qq!,check-prog.c:10: __error__: `void' used as parameter 1 (n) of function f() which is `int'!,
    qq!,check-prog.c:11: __error__: `void' used as parameter 1 (n) of function f() which is `int'!,
    qq!,check-prog.c:8: __error__: invalid use of void expression!,
    qq!,check-prog.c:9: __error__: indirection of a pointer to void!,
    qq!,check-prog.c:10: __error__: invalid use of void expression!,
    qq!,check-prog.c:11: __error__: indirection of a pointer to void!,
    ]
},


{
title => q{Too many or not enough elements in array initializer},
program => q!
    struct S { int a[3]; char c; };
    int gEmpty[] = {};
    int main()
    {
        int a0[3] = { 99, 88, 77, 66 };
        int a1[3] = { 99, 88 }; 
        char a2[3] = "foo";  // no room for terminating \0: error
        char a3[3] = "foobar";  // worse
        char a4[3] = "x";  // v2[2] not initialized: no warning
        struct S s0 = { { 55, 44, 33, 22 }, '$' };  // too many ints: error
        struct S s1 = { { 55, 44 }, '$' };  // not enough ints: warning
        int empty[] = {};
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:3: __error__: invalid dimensions for array `gEmpty'!,
    qq!,check-prog.c:6: __error__: too many elements (4) in initializer for array of 3 element(s)!,
    qq!,check-prog.c:7: __warning__: only 2 element(s) in initializer for array of 3 element(s)!,
    qq!,check-prog.c:8: __error__: too many characters (4) in string literal initializer for array of 3 character(s)!,
    qq!,check-prog.c:9: __error__: too many characters (7) in string literal initializer for array of 3 character(s)!,
    qq!,check-prog.c:11: __error__: too many elements (4) in initializer for array of 3 element(s)!,
    qq!,check-prog.c:12: __warning__: only 2 element(s) in initializer for array of 3 element(s)!,
    qq!,check-prog.c:13: __error__: invalid dimensions for array `empty'!,
    ]
},


{
title => q{Warning option for when assigning a numerical constant for a function's pointer parameter},
options => "-Wpass-const-for-func-pointer",
program => q!
    void f(char *p) {}
    int main()
    {
        f(42);
        f(0);  // no warning
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:5: __warning__: passing non-zero numeric constant as parameter 1 (p) of function f(), which is `char *'!,
    ]
},


{
title => q{No warning by default when assing a numerical constant for a function's pointer parameter},
program => q!
    void f(char *p) {}
    int main()
    {
        f(42);
        f(0);
        return 0;
    }
    !,
expected => [
    ]
},


{
title => q{Unknown enumerator name used to define an enumerator},
program => q!
    enum { B = A + 1 };
    int main()
    {
        return B;
    }
    !,
expected => [
    qq!,check-prog.c:2: __error__: left side of operator + is of type void!,
        # Not a good error message: we should mention that "A" is undeclared.
    ]
},


{
title => q{Return value of a void function must be ignored},
program => q`
    void f() {}
    int main()
    {
        if (f() == 0)
            return 1;
        if (0 == f())
            return 1;
        if (- f())
            return 1;
        if (! f())
            return 1;
        if (* f())
            return 1;
        if (& f())
            return 1;
        if (~ f())
            return 1;
        return f();
    }
    `,
expected => [
    qq!,check-prog.c:5: __error__: left side of operator == is of type void!,
    qq!,check-prog.c:7: __error__: right side of operator == is of type void!,
    qq!,check-prog.c:9: __error__: argument of arithmetic negation operator is of type void!,
    qq!,check-prog.c:11: __error__: argument of boolean negation operator is of type void!,
    qq!,check-prog.c:13: __error__: argument of indirection operator is of type void!,
    qq!,check-prog.c:15: __error__: argument of address-of operator is of type void!,
    qq!,check-prog.c:17: __error__: argument of bitwise not operator is of type void!,
    qq!,check-prog.c:19: __error__: returning expression of type `void', which differs from function's return type (`int')!,
    ]
},


{
title => q{Function name used where function pointer pointer expected},
program => q`
    typedef char (*FuncPtrType)();
    struct S1
    {
        FuncPtrType *funcPtrPtr;  // pointer to pointer to function
    };
    char f() { return 0; }  // of type FuncPtrType
    int main()
    {
        struct S1 s1 = { f };  // error: assigning void * to pointer to pointer to function
        return 0;
    }
    `,
expected => [
    qq!,check-prog.c:10: __error__: using `char (*)()' to initialize `char (*)()*'!,
    ]
},


{
title => q{Array of function pointers},
program => q`
    char (*g0[])();
    char (*g1[2])();
    char (*g2[2][3])();
    int main()
    {
        return 0;
    }
    `,
expected => [
    qq!,check-prog.c:2: __warning__: array `g0' assumed to have one element!,
    ]
},


{
title => q{Instruction argument refers to undeclared variable or enumerator},
program => q!
    int main()
    {
        for (;;)  // check that asm{} works in a sub-scope of the function scope
        {
            char *pc;
            asm {
                stu :y      // error: y not defined
                stu :foo    // error: foo not defined
                std :pc     // no error: variable 'pc' declared
                inc :       // error: variable name is empty 
            }
            break;
        }
        asm { clr :ch }  // error: ch not declared yet
        char ch;
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:7: __error__: undeclared identifier `' in assembly language statement!,
    qq!,check-prog.c:7: __error__: undeclared identifier `foo' in assembly language statement!,
    qq!,check-prog.c:7: __error__: undeclared identifier `y' in assembly language statement!,
    qq!,check-prog.c:15: __error__: undeclared identifier `ch' in assembly language statement!,
    ]
},


{
title => q{Function without formal parameters},
program => q!
    void f {} 
    int main
    {
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:2: __error__: function f() has no formal parameter list!,
    qq!,check-prog.c:3: __error__: function main() has no formal parameter list!,
    ]
},


{
title => q{Argument too large for function parameter},
program => q!
    char takeChar(char x) { return x; }
    unsigned char takeUnsignedChar(unsigned char x) { return x; }
    int main()
    {
        char c = takeChar(0x1234);
        c = takeChar(0x12F0);
        unsigned char uc = takeUnsignedChar(0x1256);
        uc = takeUnsignedChar(0x12F8);
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:6: __warning__: `int' argument is too large for parameter 1 (x) of function takeChar(), which is `char`!,
    qq!,check-prog.c:7: __warning__: `int' argument is too large for parameter 1 (x) of function takeChar(), which is `char`!,
    qq!,check-prog.c:8: __warning__: `int' argument is too large for parameter 1 (x) of function takeUnsignedChar(), which is `unsigned char`!,
    qq!,check-prog.c:9: __warning__: `int' argument is too large for parameter 1 (x) of function takeUnsignedChar(), which is `unsigned char`!,
    ]
},


{
title => q{Float},
program => q!
    int main()
    {
        float f;  // warning on 'float' when compiling for USim
        double d;
        float *pf;
        f = pf;
        pf = f;
        f = (float) pf;
        pf = (float *) f;
        int i;
        i = f;
        long l;
        l = f;
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:4: __warning__: floating-point arithmetic is not supported on this platform!,
    qq!,check-prog.c:5: __warning__: `double' is an alias for `float' for this compiler!,
    qq!,check-prog.c:7: __error__: assigning `float *' to `float'!,
    qq!,check-prog.c:8: __warning__: assigning non-pointer/array (float) to `float *'!,
    qq!,check-prog.c:9: __error__: cannot cast `float *' to `float'!,
    qq!,check-prog.c:10: __error__: cannot cast `float' to `float *'!,
    qq!,check-prog.c:12: __warning__: assigning real type `float' to `int`!,
    qq!,check-prog.c:14: __warning__: assigning real type `float' to `long`!,
    ]
},


{
title => q{Floating-point arithmetic},
program => q!
    int main()
    {
        signed float sf;
        unsigned float uf;
        signed double sd;
        unsigned double ud;
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:4: __warning__: floating-point arithmetic is not supported on this platform!,
    qq!,check-prog.c:4: __error__: signed and unsigned modifiers can only be applied to integral type!,
    qq!,check-prog.c:5: __error__: signed and unsigned modifiers can only be applied to integral type!,
    qq!,check-prog.c:6: __warning__: `double' is an alias for `float' for this compiler!,
    qq!,check-prog.c:6: __error__: signed and unsigned modifiers can only be applied to integral type!,
    qq!,check-prog.c:7: __error__: signed and unsigned modifiers can only be applied to integral type!,
    ]
},


{
title => q{Uncalled static function},
linkerModeOnly => 1,
program => q!
    static void used() {}
    static void unused() {}  // warning expected
    void usedButExported() {}  // no warning expected
    int main()
    {
        used();
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:3: __warning__: static function unused() is not called!,
    ]
},


{
title => q{Function with both static and extern modifiers},
program => q!
    extern static void f() {}
    int main()
    {
        f();
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:2: __error__: function definition must not be both static and extern!,
    ]
},


{
title => q{Global variable both initialized and declared extern},
linkerModeOnly => 1,
program => q!
    extern int a = 42;
    int b = 42;  // ok
    extern int c;  // ok
    int main()
    {
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:2: __warning__: `a' initialized and declared `extern'!,
    ]
},


{
title => q{static main()},
program => q!
    static int main()
    {
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:2: __error__: main() must not be static!,
    ]
},


{
title => q{Comparison between pointer and integer},
program => q!
    int main()
    {
        int a[] = { 44 };
        if (a == 0L) 
            return 1;
        if (a == 0UL)
            return 1;
        if (a == 1L) 
            return 1;
        if (a == 1UL)
            return 1;
        if (a == 0.0f)
            return 1;
        if (a == 1.5f)
            return 1;
        int *b; 
        if (0UL == b)
            return 1;
        if (-4L == b)
            return 1;
        if (b == 1.5f)
            return 1;
        if (1.5f == b)
            return 1;
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:9: __error__: comparison between pointer (int[]) and integer (long)!,
    qq!,check-prog.c:11: __error__: comparison between pointer (int[]) and integer (unsigned long)!,
    qq!,check-prog.c:13: __error__: comparison between pointer (int[]) and integer (float)!,
    qq!,check-prog.c:15: __error__: comparison between pointer (int[]) and integer (float)!,
    qq!,check-prog.c:20: __error__: comparison between pointer (int *) and integer (long)!,
    qq!,check-prog.c:22: __error__: comparison between pointer (int *) and integer (float)!,
    qq!,check-prog.c:24: __error__: comparison between pointer (int *) and integer (float)!,
    ]
},


{
title => q{const and volatile},
program => q!




    void print(const char *s) {}

    void f()
    {
        {
           const int i = 5;  
           i = 10;   // Error  
           i++;   // Error  
        }
        
        {  
           char *mybuf = 0, *yourbuf;  
           char *const aptr = mybuf;  
           *aptr = 'a';
           aptr = yourbuf;   // Error
        }  
        
        {  
           const char *mybuf = "test";  
           char *yourbuf = "test2";  // Error
           yourbuf = "test3";  // Error
           print(mybuf);
          
           const char *bptr = mybuf;   // Pointer to constant data
           *bptr = 'a';   // Error
        }  
    }
    
    void takesNonConstIntPtr(int *p) {}

    void g(const int *p, const int n)
    {
        ++n;  // Error
        int m = n;
        *p = 42;  // Error
        int *q = p;  // Error
        int *q0;
        q0 = p;  // Error
        int *r = (int *) p;
        *r = 1000;
        * (int *) p = 1500;
        const int *s = r;
        *s = 2000;  // Error
        const int *t = (const int *) r;
        * (const int *) r = 2500;  // Error
        ++*((const int *) r);  // Error
        takesNonConstIntPtr(p);  // Error
        takesNonConstIntPtr(&n);  // Error
        const int k = 42;
        takesNonConstIntPtr(&k);  // Error
    }
    
    struct S
    {
        char *str;
        const int n;
    };
    
    void testStruct()
    {
        struct S s = { "foo", 1000 };
        s.str = "x";  // Error
        s.n = 1001;
        ++s.n;
    }
    
    typedef const int ConstInt;
    typedef const int *PtrToConstInt;
    
    void testTypedef()
    {
        ConstInt ci = 333;
        ci = 444;
        PtrToConstInt pci = 0;
        int *p = pci;
    }

    const char *returnsConstCharPtr() { return 0; }

    int main()
    {
        f();
        int n = 999;
        g(&n, n);
        testStruct();
        testTypedef();
        char *cp = returnsConstCharPtr();
        volatile int v = -1000;
        volatile char w;
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:93: __warning__: the `volatile' keyword is not supported by this compiler!,
    qq!,check-prog.c:12: __warning__: assigning `int' to `const int' is not const-correct!,
    qq!,check-prog.c:20: __warning__: assigning `char *' to `char * const' is not const-correct!,
    qq!,check-prog.c:26: __warning__: assigning `const char[]' to `char *' is not const-correct!,
    qq!,check-prog.c:30: __warning__: assigning `char' to `const char' is not const-correct!,
    qq!,check-prog.c:13: __warning__: incrementing a constant expression (type is `const int')!,
    qq!,check-prog.c:25: __warning__: initializing non-constant `char *' (yourbuf) from `const char[]'!,
    qq!,check-prog.c:40: __warning__: assigning `int' to `const int' is not const-correct!,
    qq!,check-prog.c:43: __warning__: assigning `const int *' to `int *' is not const-correct!,
    qq!,check-prog.c:48: __warning__: assigning `int' to `const int' is not const-correct!,
    qq!,check-prog.c:50: __warning__: assigning `int' to `const int' is not const-correct!,
    qq!,check-prog.c:52: __warning__: `const int *' used as parameter 1 (p) of function takesNonConstIntPtr() which is `int *' (not const-correct)!,
    qq!,check-prog.c:53: __warning__: `const int *' used as parameter 1 (p) of function takesNonConstIntPtr() which is `int *' (not const-correct)!,
    qq!,check-prog.c:55: __warning__: `const int *' used as parameter 1 (p) of function takesNonConstIntPtr() which is `int *' (not const-correct)!,
    qq!,check-prog.c:38: __warning__: incrementing a constant expression (type is `const int')!,
    qq!,check-prog.c:41: __warning__: using `const int *' to initialize `int *' is not const-correct!,
    qq!,check-prog.c:51: __warning__: incrementing a constant expression (type is `const int')!,
    qq!,check-prog.c:67: __warning__: assigning `const char[]' to `char *' is not const-correct!,
    qq!,check-prog.c:68: __warning__: assigning `int' to `const int' is not const-correct!,
    qq!,check-prog.c:66: __warning__: initializing non-constant `char *' (s) from `const char[]'!,
    qq!,check-prog.c:69: __warning__: incrementing a constant expression (type is `const int')!,
    qq!,check-prog.c:78: __warning__: assigning `int' to `const int' is not const-correct!,
    qq!,check-prog.c:80: __warning__: using `const int *' to initialize `int *' is not const-correct!,
    qq!,check-prog.c:92: __warning__: using `const char *' to initialize `char *' is not const-correct!,
    ]
},


{
title => q{-Wno-const},
options => "-Wno-const",
program => q!
    void f(char *) {}
    void g(const int *p)
    {
        int *q = p;
    }
    int main()
    {
        char *s = "x";  // N.B.: silenced warning is different than the one for 'q' because "x" is an array
        const char *cs = "y";
        s = cs;
        f(cs);
        return 0;
    }
    !,
expected => [
    ]
},
    

{
title => q{Indirection of non-pointer},
program => q!
    int main()
    {
        return * (int) 1000;
    }
    !,
expected => [
    qq!,check-prog.c:4: __error__: indirection using `int' as pointer (assuming `void *')!,
    qq!,check-prog.c:4: __error__: returning expression of type `void *', which differs from function's return type (`int')!,
    ]
},


{
title => q{Mixing interrupt/non-interrupt function pointer types},
program => q!
    interrupt void isr() {}
    void regular() {}
    int main()
    {
        void (*pfRegular)() = isr;
        (*pfRegular)();
        interrupt void (*pfISR)() = isr;
        (*pfISR)();
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:9: __error__: calling an interrupt service routine is forbidden!,
    qq!,check-prog.c:6: __error__: using `interrupt void (*)()' to initialize `void (*)()'!,
    ]
},


{
title => q{Passing a struct to a function expecting a numerical parameter},
program => q!
    struct S {} s;
    void f(int n) {}
    void g(struct S x) {}
    int main()
    {
        f(s);
        g(42);
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:7: __error__: `struct S' used as parameter 1 (n) of function f() which is `int'!,
    qq!,check-prog.c:8: __error__: `int' used as parameter 1 (x) of function g() which is `struct S'!,
    ]
},


{
title => q{Function pointer type safety},
program => q!
    struct S {} s;
    int main()
    {
        void *vp = (void *) 1000;
        ((void (*)(char, int)) vp)('a', s);  // wrong type for 2nd arg
        ((void (*)(char, int)) vp)('a');  // wrong number of args
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:6: __error__: `struct S' used as parameter 2 of call through function pointer which is `int'!,
    qq!,check-prog.c:7: __error__: call through function pointer passes 1 argument(s) but function expects 2!,
    ]
},


{
title => q{Function argument of wrong signedness},
program => q!
    void takesCharPtr(char* b) {}
    int main()
    {
        takesCharPtr((unsigned char *) 0);
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:5: __warning__: `unsigned char *' used as parameter 1 (b) of function takesCharPtr() which is `char *' (different signedness)!,
    ]
},


{
title => q{Declaring a pointer to a function returning an array},
program => q!
    char (*z0)()[];
    char (*z1)()[2];
    char (*z2)()[2][3];
    struct S
    {
        char (*m0)()[];
        int (*m1)(char, long)[42];
    };
    void f(char (*fParam)()[]) {}
    void g(int (*gParam)(char, long)[42]) {}
    int main()
    {
        char (*f0)()[];
        int (*f1)(char, long)[42];
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:2: __error__: `z0' declared as function returning an array!,
    qq!,check-prog.c:3: __error__: `z1' declared as function returning an array!,
    qq!,check-prog.c:4: __error__: `z2' declared as function returning an array!,
    qq!,check-prog.c:7: __error__: `m0' declared as function returning an array!,
    qq!,check-prog.c:8: __error__: `m1' declared as function returning an array!,
    qq!,check-prog.c:10: __error__: `fParam' declared as function returning an array!,
    qq!,check-prog.c:11: __error__: `gParam' declared as function returning an array!,
    qq!,check-prog.c:14: __error__: `f0' declared as function returning an array!,
    qq!,check-prog.c:15: __error__: `f1' declared as function returning an array!,
    ]
},


{
title => q{Pointer to variadic function},
program => q!
    void variadicFunc0(char *fmt, ...) {}
    int main()
    {
        void (*good)(char *, ...) = variadicFunc0;
        void *bad1 = variadicFunc0;
        void (*bad2)(char *) = variadicFunc0;
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:6: __warning__: assigning function pointer `void (*)(char *, ...)' to `void *`!,
    qq!,check-prog.c:7: __error__: using `void (*)(char *, ...)' to initialize `void (*)(char *)'!,
    ]
},


{
title => q{main() parameters when not OS-9},
program => q!
    int main(int argc, char *argv[])
    {
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:2: __warning__: main() does not receive parameters when targeting this platform!,
    ]
},


{
title => q{wrong main() parameters},
target => "os9",
linkerModeOnly => 1,
program => q!
    int main(int bogus)
    {
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:2: __error__: main() must receive (int, char **) or no parameters!,
    ]
},


{
title => q{__norts__ without asm},
program => q!
    __norts__ void f() {}
    int main()
    {
        f();
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:2: __error__: `__norts__' must be used with `asm' when defining an asm-only function!,
    ]
},


{
title => q{sizeof on incomplete type, warning on size of dimensionless array},
linkerModeOnly => 1,
program => q!
    extern int n;
    extern int v[];
    extern int w[3];
    int x[];
    int main()
    {
        int sn = sizeof(n);
        int sv = sizeof(v);
        int sw = sizeof(w);
        int sx = sizeof(x);
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:5: __warning__: array `x' assumed to have one element!,
    qq!,check-prog.c:9: __error__: invalid application of `sizeof' to incomplete type `int[]'!,
    qq!,check-prog.c:11: __error__: invalid application of `sizeof' to incomplete type `int[][]'!,
    ]
},


{
title => q{Assignment to const function pointer},
program => q!
    void f() {}
    typedef void (*FuncPtr)();
    const FuncPtr funcPtr = f;
    const FuncPtr fpArray[] = { f };
    typedef void *VoidPtr;
    const VoidPtr vpArray[] = { 0 };
    int main()
    {
        funcPtr = f;
        fpArray[0] = f;
        vpArray[0] = (VoidPtr) 0;
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:10: __warning__: assigning `void (*)()' to `void (* const)()' is not const-correct!,
    qq!,check-prog.c:11: __warning__: assigning `void (*)()' to `void (* const)()' is not const-correct!,
    qq!,check-prog.c:12: __warning__: assigning `void *' to `void * const' is not const-correct!,
    ]
},


{
title => q{Declaration or typedef with empty declarator name},
program => q!
    int a, , b, , c;
    typedef int ;
    typedef int , ;
    typedef int A, , B , ;
    void f(int, char *) {}  // unnamed function parameters are OK
    int main()
    {
        int , , ;
        char p, *, *q, **, [];
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:2: __error__: empty declarator name!,
    qq!,check-prog.c:2: __error__: empty declarator name!,
    qq!,check-prog.c:3: __error__: empty typename!,
    qq!,check-prog.c:4: __error__: empty typename!,
    qq!,check-prog.c:4: __error__: empty typename!,
    qq!,check-prog.c:5: __error__: empty typename!,
    qq!,check-prog.c:5: __error__: empty typename!,
    qq!,check-prog.c:9: __error__: empty declarator name!,
    qq!,check-prog.c:9: __error__: empty declarator name!,
    qq!,check-prog.c:9: __error__: empty declarator name!,
    qq!,check-prog.c:10: __error__: empty declarator name!,
    qq!,check-prog.c:10: __error__: empty declarator name!,
    qq!,check-prog.c:10: __error__: empty declarator name!,
    ]
},


{
title => q{Invalid character in hexadecimal literal},
program => q!
    int main()
    {
        return 0xF3Q;
    }
    !,
expected => [
    qq!,check-prog.c:4: __error__: syntax error: Q!,
    ]
},


{
title => q{Unsupported bit field width},
program => q!
    int g;
    struct S0
    {
        unsigned empty : 0;
        unsigned long tooWideForLong : 33;
        unsigned short tooWideForShort : 17;
        unsigned alsoTooWideForShort : 17;
        char tooWideForChar : 9;  // bad: width of field exceeds its type
        char negativeWidth : -1;
        float f : 5;
        int z : g;
    };
    int main()
    {
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:5: __error__: zero width for bit-field `empty'!,
    qq!,check-prog.c:6: __error__: width of `tooWideForLong' exceeds its type (`unsigned long')!,
    qq!,check-prog.c:7: __error__: width of `tooWideForShort' exceeds its type (`unsigned int')!,
    qq!,check-prog.c:8: __error__: width of `alsoTooWideForShort' exceeds its type (`unsigned int')!,
    qq!,check-prog.c:9: __error__: width of `tooWideForChar' exceeds its type (`char')!,
    qq!,check-prog.c:10: __error__: negative width in bit-field `negativeWidth'!,
    qq!,check-prog.c:11: __warning__: floating-point arithmetic is not supported on this platform!,
    qq!,check-prog.c:11: __error__: bit-field `f' has invalid type (`float')!,
    qq!,check-prog.c:12: __error__: invalid width in bit-field `z'!,
    ]
},


{
title => q{Multiple bodies for a function},
program => q!
    int main() { return 0; }
    int main() { return 1; }
    void f() {}
    void f() { return; }
    !,
expected => [
    qq!,check-prog.c:3: __error__: main() already has a body at ,check-prog.c:2!,
    qq!,check-prog.c:5: __error__: f() already has a body at ,check-prog.c:4!,
    ]
},


{
title => q{Invalid function typedef},
program => q!
    typedef void F(int, int);  // right syntax would be (*F) instead of F
    F pf = 0;
    int main()
    {
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:2: __error__: invalid function typedef!,
    ]
},


{
title => q{Array subscript is not an integer},
program => q!
    int main()
    {
        int *a;
        return a[2.0f] + a[-3L] + a[4UL];
    }
    !,
expected => [
    qq!,check-prog.c:5: __error__: array subscript is not an integer (`float')!,
    qq!,check-prog.c:5: __warning__: array subscript is long (only low 16 bits used)!,
    qq!,check-prog.c:5: __warning__: array subscript is unsigned long (only low 16 bits used)!,
    ]
},


{
title => q{extern without const followed by definition with const},
linkerModeOnly => 1,
program => q!
    extern char a[];
    const char a[] = { 'X' };  // bad: const was not in extern declaration
    int n;
    extern int n;  // OK
    int main()
    {
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:3: __error__: global variable `a' declared with type `const char[]' at `,check-prog.c:3' but with type `char[]' at `,check-prog.c:2'!,
    ]
},


{
title => q{Subtraction of incompatible pointers},
program => q!
    int main()
    {
        int *a;
        char *b;
        unsigned d = a - b;
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:6: __error__: subtraction of incompatible pointers (int * vs char *)!,
    ]
},


{
title => q{L-value required as left operand of assignment},
program => q!
    int main()
    {
        unsigned char *p;
        (void *) p = 0;
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:5: __error__: l-value required as left operand of assignment!,
    ]
},


{
title => q{First parameter too large to be received in register},
program => q!
    _CMOC_fpir_ void takesLong1(long a, int b)
    {
    }
    struct S { int x; };
    _CMOC_fpir_ void takesLong2(struct S a, int b)
    {
    }
    int main()
    {
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:2: __error__: _CMOC_fpir_ not allowed on function whose first parameter is struct, union or larger than 2 bytes!,
    qq!,check-prog.c:6: __error__: _CMOC_fpir_ not allowed on function whose first parameter is struct, union or larger than 2 bytes!,
    ]
},


{
title => q{Assigning/incrementing/decrementing const member of struct},
program => q!
    typedef struct Couple { int x, y; } Couple;
    Couple curvePoints[2][8];
    struct S { const Couple *pc; };
    int main()
    {
        const Couple *curCouple = curvePoints[1];
        curCouple->x = 42;  // bad: *curCouple is const
        struct S s = { curvePoints[1] };
        s.pc->y = 999;  // bad: *s.pc is const (even though s is not const)
        s.pc->y += 999;  // bad: *s.pc is const (even though s is not const)
        const Couple c = { 44, 55 };
        c.x = 888;  // bad
        ++c.y;  // bad
        c.y--;  // bad
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:8: __error__: assigning to member `x' of `const struct Couple' is not const-correct!,
    qq!,check-prog.c:10: __error__: assigning to member `y' of `const struct Couple' is not const-correct!,
    qq!,check-prog.c:11: __error__: assigning to member `y' of `const struct Couple' is not const-correct!,
    qq!,check-prog.c:13: __error__: assigning to member `x' of `const struct Couple' is not const-correct!,
    qq!,check-prog.c:14: __error__: incrementing member `y' of `const struct Couple' is not const-correct!,
    qq!,check-prog.c:15: __error__: decrementing member `y' of `const struct Couple' is not const-correct!,
    ]
},


{
title => q{Implicit cast of void pointer},
program => q!
    void f(unsigned char *puc) {} 
    int main()
    {
        void *pv;
        unsigned char *puc = pv;
        puc = pv;
        f(pv);
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:7: __warning__: assigning `void *' to `unsigned char *' (implicit cast of void pointer)!,
    qq!,check-prog.c:8: __warning__: passing `void *' for parameter of type `unsigned char *' (implicit cast of void pointer)!,
    qq!,check-prog.c:6: __warning__: using `void *' to initialize `unsigned char *' (implicit cast of void pointer)!,
    ]
},


{
title => q{Warning about binary operation on byte-sized arguments},
options => "-Wgives-byte",
program => q!
    int main()
    {
        char a = 100, b = 200;
        int p0 = a * b;  // yields byte under CMOC
        int p1 = (int) a * b;
        int p2 = a * (int) b;
        int p3 = (int) a * (int) b;
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:5: __warning__: operator `multiplication' on two byte-sized arguments gives byte under CMOC, unlike under Standard C!,
    ]
},


{
title => q{Initializer element is not constant},
program => q!
    int f();
    int g = f();
    const int a[] = { 0 };
    const int *p = a + (sizeof a)/(sizeof(int));  // OK
    const int n = sizeof(a) + 1;  // OK
    const int k = n;
    const int *p1 = a + (sizeof a)/(sizeof(int)) + 5;  // OK
    const int *p2 = a + ((sizeof a)/(sizeof(int)) + 5);  // OK
    const int *p3 = 7 + a;  // OK
    struct S { int m; };
    struct S v[5];
    struct S *e = &v[5];  // OK
    struct S mat[5][3];
    struct S *e1 = &mat[5][3];  // OK
    int j;
    struct S *e2 = &mat[5][j];  // bad
    struct S *e3 = &mat[j][3];  // bad
    struct S *e4 = &mat[j][j];  // bad
    int main()
    {
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:3: __error__: initializer element is not constant!,
    qq!,check-prog.c:7: __error__: initializer element is not constant!,
    qq!,check-prog.c:17: __error__: initializer element is not constant!,
    qq!,check-prog.c:18: __error__: initializer element is not constant!,
    qq!,check-prog.c:19: __error__: initializer element is not constant!,
    ]
},


{
title => q{Warn when a local variable hides another one},
options => "-Wlocal-var-hiding",
program => q!
    int g;
    int main()
    {
        int local;
        if (1)
        {
            int local;  // hides previous 'local'
            int g;  // OK to hide global 'g'
        }
        return 0;
    }
    !,
expected => [
    qq!,check-prog.c:8: __warning__: Local variable `local' hides local variable `local' declared at ,check-prog.c:5!,
    ]
},


#{
#title => q{Sample test},
#program => q!
#    int main()
#    {
#        return 0;
#    }
#    !,
#expected => [
#    qq!!,
#    ]
#},


);


###############################################################################


my $program = basename($0);
my $version = "0.0.0";
my $numErrors = 0;
my $numWarnings = 0;

my $srcdir;
my $assemblerFilename;
my @includeDirList;
my $stopOnFail = 0;


sub usage
{
    my ($exitStatus) = @_;

    print <<__EOF__;
Usage: $program [options]

--nocleanup       Do not delete the intermediate files after running.
--only=NUM        Only run test #NUM, with no clean up. Implies --nocleanup.
--last            Only run the last test. Implies --nocleanup.
--start=NUM       Start at test #NUM. The first test has number zero.
--stop-on-fail    Stop right after a test has failed instead of continuing
                  to the end of the test list. Implies --nocleanup.
--titles[=STRING] Dump to test titles (with numbers) to standard output.
                  If STRING specified, only dumps titles that contain STRING.

__EOF__

    exit($exitStatus);
}


sub errmsg
{
    print "$program: ERROR: ";
    printf @_;
    print "\n";

    ++$numErrors;
}


sub warnmsg
{
    print "$program: Warning: ";
    printf @_;
    print "\n";

    ++$numWarnings;
}


sub indexi
{
    my ($haystack, $needle) = @_;
    
    return index(lc($haystack), lc($needle));
}


sub compileProgram
{
    my ($program, $extraCompilerOptions, $target) = @_;

    $target = $target || "usim"; 

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

    my $compCmd = "./cmoc --$target";
    if (! $monolithMode)
    {
        $compCmd .= " -Lstdlib -Lfloat";
    }
    else
    {
        $compCmd .= " --monolith --a09='$assemblerFilename'";
    }

    for my $includeDir (@includeDirList)
    {
        $compCmd .= " -I '$includeDir'";
    }
    if (defined $extraCompilerOptions)
    {
        $compCmd .= " $extraCompilerOptions";
    }
    $compCmd .= " $cFilename";  # compile for usim
    print "--- Compilation command: $compCmd\n";
    print "--- Actual compilation errors:\n";
    my $fh;
    if (!open($fh, "$compCmd 2>&1 |"))
    {
        print "$0: ERROR: failed to start compilation command $compCmd: $!\n";
        return undef;
    }
    
    # Accumulate compiler output lines in a list.
    #
    my @compOutput;
    my $line;
    while ($line = <$fh>)
    {
        next if $line =~ /^# /;  # ignore debugging traces

        $line =~ s/\s+$//s;
        
        if (0 && $line =~ /^#/)  # activate this to ignore debugging output lines
        {
            $numDebuggingLines++;
            next;
        }

        # Mask errors and warnings to keep auto build system from complaining.
        $line =~ s/: error:/: __error__:/gi;
        $line =~ s/: warning:/: __warning__:/g;

        push @compOutput, $line;
        printf("%6u  %s\n", scalar(@compOutput), $line);
    }

    if (!close($fh) && $!)  # close() expected to fail because compiler gave errors;
                            # real close() error is when $! non null.
    {
        print "$0: ERROR: failed to close pipe to command $compCmd: $!\n";
        return undef;
    }

    return \@compOutput;
}


sub runTestNumber
{
    my ($i) = @_;

    if ($i < 0 || $i >= @testCaseList)
    {
        print "$0: ERROR: no test case #$i\n";
        return 0;
    }
    my $rhTestCase = $testCaseList[$i];
    die unless defined $rhTestCase;

    print "\n";
    print "-" x 80, "\n";
    print "--- Program # $i: ", $rhTestCase->{title}, "\n";
    
    if (defined $rhTestCase->{linkerModeOnly} && $monolithMode)
    {
        print "Test skipped because excluded from monolith mode\n";
        return 1;
    }
    
    my $lineNum = 0;
    for my $line (split /\n/, $rhTestCase->{program})
    {
        printf("%3u%5s%s\n", ++$lineNum, "", $line);
    }

    if (defined $rhTestCase->{suspended})
    {
        print "\n";
        print "This test is suspended.\n";
        print "\n";
        return 1;
    }

    my $raActualOutput = compileProgram($rhTestCase->{program}, $rhTestCase->{options}, $rhTestCase->{target});

    my $raExpected = $rhTestCase->{expected};
    if (defined $raExpected)
    {
        print "--- Expected compilation errors:\n";
        my $ctr = 0;
        for my $exp (@$raExpected)
        {
            $exp =~ s/: error:/: __error__:/gi;  # mask error b/c not real error
            $exp =~ s/: warning:/: __warning__:/g;
            printf("%6u  %s\n", ++$ctr, $exp);
        }
    }
    else
    {
        print "$0: ERROR: test #$i does not specify the expected compilation errors\n";
        return 0;
    }

    if (!defined $raActualOutput)
    {
        print "$0: ERROR: program #$i: no output\n";
        return 0;
    }

    # Compare each line.
    #
    my $success = 1;
    for (my $j = 0; $j < @$raActualOutput; ++$j)
    {
        last if $j >= @$raExpected;
        my $act = $raActualOutput->[$j];
        my $exp = $raExpected->[$j];
        if ($act ne $exp)
        {
            print "$0: ERROR: program #$i: actual output differs at line ",
                    $j + 1, " from expected output\n";
            $success = 0;
            last;
        }
    }

    if (@$raActualOutput < @$raExpected)
    {
        print "$0: ERROR: program #$i: got fewer actual errors than expected\n";
        $success = 0;
    }
    elsif (@$raActualOutput > @$raExpected)
    {
        print "$0: ERROR: program #$i: got more actual errors than expected\n";
        $success = 0;
    }

    return $success;
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


###############################################################################


my $showUsage = 0;
my $noCleanUp = 0;
my $onlyArg;
my $onlyLast;
my $firstTestToRun;
my $titleDumpWanted;

if (!GetOptions(
    "help" => \$showUsage,
    "only=s" => \$onlyArg,
    "start=i" => \$firstTestToRun,
    "last" => \$onlyLast,
    "nocleanup" => \$noCleanUp,
    "stop-on-fail" => \$stopOnFail,
    "titles:s" => \$titleDumpWanted,  # the ':' means argument is optional
    "monolith" => \$monolithMode,
    ))
{
    exit 1;
}

usage(0) if $showUsage;

$| = 1;  # no buffering on STDOUT

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

$srcdir = shift || ".";
$assemblerFilename = "$srcdir/a09";
if (! $monolithMode)
{
    @includeDirList = ("$srcdir/stdlib");
}
else
{
    @includeDirList = ("$srcdir/support");
}

my $cleanUp = !$noCleanUp && !defined $onlyArg && !defined $onlyLast && !$stopOnFail;

my @testNumbers = 0 .. $#testCaseList;  # numbers of the tests to be run
if (defined $onlyArg)
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

$ENV{PATH} = $srcdir . ":" . $ENV{PATH};  # allows a09 to find intelhex2cocobin

my @failedTestNumbers = ();

for my $i (@testNumbers)
{
    if (!runTestNumber($i))
    {
        push @failedTestNumbers, $i;
        last if $stopOnFail;
    }
    print "### ls ,check-prog.*:\n";
    system("ls ,check-prog.*");
    print "###\n";
}

if ($cleanUp)
{
    print "\n";
    print "Cleaning up:\n";
    my $success = 1;
    for my $ext (qw(c asm s i lst hex bin srec map link o))
    {
        my $fn = ",check-prog.$ext";
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
    exit 1 unless $success;
}

print "\n";

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

if ($numDebuggingLines > 0)
{
    warnmsg("ignored %u debugging output lines", $numDebuggingLines);
}

print "$program: $numErrors error(s), $numWarnings warnings(s).\n";
exit($numErrors > 0);
