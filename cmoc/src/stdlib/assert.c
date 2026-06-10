/*  assert.c - Assert implementation for CMOC

    By Pierre Sarrazin <http://sarrazip.com/>.
    This file is in the public domain.
*/

// Include <assert-impl.h>, whose declarations do not depend on NDEBUG,
// instead of <assert.h> which depends on NDEBUG.
// With NDEBUG, _SetFailedAssertHandler() would get defined to an empty string.
//
#include <assert-impl.h>

#include <cmoc.h>

#ifdef VECTREX
#define putchar(c)
#endif


static unsigned char freezeOnFailedAssert = 0;

static _FailedAssertHandler failedAssertHandler = NULL;


static void
printAsciiZ(const char *s)
{
    for ( ; *s; ++s)
        putchar(*s);
}


void
_OnFailedAssert(const char *file, int lineno, const char *condition)
{
    if (failedAssertHandler)
    {
        (*failedAssertHandler)(file, lineno, condition);
        return;
    }

    // Print error message w/o printf(), to avoid linking with it, b/c it is large.
    printAsciiZ("***ASSERT FAILED: ");
    printAsciiZ(file);
    putchar(':');
    char tmp[6];
    printAsciiZ(itoa10(lineno, tmp));
    printAsciiZ(": ");
    printAsciiZ(condition);
    putchar('\n');

    if (!freezeOnFailedAssert)
        exit(1);
    for (;;)
        ;
}


void
_FreezeOnFailedAssert(int freeze)
{
    freezeOnFailedAssert = (freeze != 0);
}


void _SetFailedAssertHandler(_FailedAssertHandler newHandler)
{
    failedAssertHandler = newHandler;
}
