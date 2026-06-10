/*  assert-impl.h - Assert implementation for CMOC

    By Pierre Sarrazin <http://sarrazip.com/>.
    This file is in the public domain.
*/

#ifndef _ASSERT_IMPL_H
#define _ASSERT_IMPL_H


typedef void (*_FailedAssertHandler)(const char *file, int lineno, const char *condition);

void _OnFailedAssert(const char *file, int lineno, const char *condition);

#define assert(cond) do { if (!(cond)) _OnFailedAssert(__FILE__, __LINE__, #cond); } while (0)

void _FreezeOnFailedAssert(int freeze);

void _SetFailedAssertHandler(_FailedAssertHandler newHandler);


#endif  /* _ASSERT_IMPL_H */
