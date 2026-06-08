/* bench.h - fixed-width types, no library/header dependency.
 * On all three 6809 C compilers (cmoc, gcc6809, vbcc): int=16-bit, long=32-bit.
 * char signedness is left explicit to keep codegen identical across compilers. */
#ifndef BENCH_H
#define BENCH_H

typedef signed char    s8;
typedef unsigned char  u8;
typedef int            s16;
typedef unsigned int   u16;
typedef long           s32;
typedef unsigned long  u32;

#endif
