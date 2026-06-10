/*  setjmp.h - Nonlocal goto.

    By Pierre Sarrazin <http://sarrazip.com/>.
    This file is in the public domain.
*/

#ifndef _setjmp_h
#define _setjmp_h


typedef unsigned char jmp_buf[6];


int setjmp(jmp_buf env);


/*  value: Must not be 0.
*/
void longjmp(jmp_buf env, int value);


#endif
