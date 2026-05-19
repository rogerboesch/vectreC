# $Id: configure.ac,v 1.132 2020/06/07 14:53:53 sarrazip Exp $

AC_PREREQ(2.54)

AC_INIT(cmoc, 0.1.67)
AC_CONFIG_SRCDIR(src/lexer.ll)
AM_INIT_AUTOMAKE

PACKAGE_FULL_NAME="CMOC"
PACKAGE_SUMMARY_EN="6809-generating cross-compiler for a subset of the C language"
PACKAGE_SUMMARY_FR="Compilateur C partiel pour le 6809"  # UTF-8
AC_SUBST(PACKAGE_FULL_NAME)
AC_SUBST(PACKAGE_SUMMARY_EN)
AC_SUBST(PACKAGE_SUMMARY_FR)

MANUAL_DATE_EN="June 7th, 2020"; AC_SUBST(MANUAL_DATE_EN)

AC_PROG_CXX
AC_LANG_CPLUSPLUS
AC_COMPILE_WARNINGS

AC_CHECK_PROG(GREP, grep, grep)
AC_CHECK_PROG(EGREP, egrep, egrep)
AC_CHECK_PROG(ENV, env, env)
AC_CHECK_PROG(HEAD, head, head)
AC_CHECK_PROG(TAIL, tail, tail)
AC_CHECK_PROG(HEXDUMP, hexdump, hexdump)
AC_CHECK_PROG(STRINGS, strings, strings)

AC_PROG_PERL_MODULES(strict)


# Check for Mac OS X.
#
AC_MSG_CHECKING([for Mac OS X])
if which uname 2>&1 > /dev/null; then
    if uname -a | $PERL -ne '/^Darwin .*?:xnu-/ or exit 1'; then
        AC_MSG_RESULT([yes])
        CPPFLAGS="$CPPFLAGS -D__unix"
        LDFLAGS="$LDFLAGS -framework CoreFoundation"
    else
        AC_MSG_RESULT([no: uname -a not recognized as Darwin])
    fi
else
    AC_MSG_RESULT([no: uname command not found in path])
fi


# Check for LWTOOLS.
#
AC_CHECK_PROG(LWASM, lwasm, lwasm, no)
if test "$LWASM" = no; then
	AC_MSG_ERROR([The LWTOOLS lwasm assembler (>= 4.11) is required. See the CMOC home page.])
else
	AC_MSG_CHECKING([lwasm --version])
	lwasm --version

	AC_MSG_CHECKING([if lwasm accepts --pragma=forwardrefmax])
	# Passing --output=/dev/null to keep lwasm from creating a.out in current directory.
	if lwasm --pragma=forwardrefmax --output=/dev/null >& /dev/null; then
		AC_MSG_RESULT([yes])
	else
		AC_MSG_RESULT([no])
		AC_MSG_ERROR([The LWTOOLS lwasm assembler (>= 4.11) is required: must accept --pragma=forwardrefmax])
	fi
fi

AC_CHECK_PROG(LWAR, lwar, lwar, no)
if test "$LWAR" = no; then
	AC_MSG_ERROR([The LWTOOLS lwar archiver (>= 4.11) is required. See the CMOC home page.])
else
	AC_MSG_CHECKING([lwar --version])
	lwar --version
fi

AC_CHECK_PROG(LWLINK, lwlink, lwlink, no)
if test "$LWLINK" = no; then
	AC_MSG_ERROR([The LWTOOLS lwlink archiver (>= 4.11) is required. See the CMOC home page.])
else
	AC_MSG_CHECKING([lwlink --version])
	lwlink --version
fi


# Check or lex and yacc (e.g., GNU Flex and GNU Bison).
#
AC_PROG_YACC
if test "$YACC" = yacc; then
    if ! which yacc 2>&1 > /dev/null; then
            AC_MSG_ERROR([Command yacc not found. Installing GNU Bison is recommended.])
    fi
fi

AC_PROG_LEX
if test "$LEX" = ":"; then
        AC_MSG_ERROR([Did not find flex or lex. Installing GNU Flex is recommended.])
fi


# Check for strcasecmp(), which is a POSIX.1 case-insensitive
# version of strcmp().
#
AC_MSG_CHECKING([for strcasecmp() while including <strings.h>])
AC_TRY_LINK(
    [#include <strings.h>],
    [int n = strcasecmp("foo", "bar"); return n != 0;],
    [AC_MSG_RESULT([found])],
    [AC_MSG_ERROR([not found])]
    )


# Allow the user to avoid compiling writecocofile.
#
build_writecocofile=no
AC_ARG_WITH([writecocofile],
[  --with-writecocofile    build the writecocofile disk image tool (default: yes)],
[
	if test "$withval" = no; then
		build_writecocofile=no
	else
		build_writecocofile=yes
	fi
],
[
	build_writecocofile=yes
])
AC_MSG_RESULT([building writecocofile disk image tool: $build_writecocofile])
AM_CONDITIONAL(BUILD_WRITECOCOFILE, [test "$build_writecocofile" = yes])


AC_CONFIG_FILES([
	Makefile
	src/Makefile
	src/stdlib/Makefile
	src/float/Makefile
	src/usim-0.91-cmoc/Makefile
	macros/Makefile
	doc/Makefile
	doc/cmoc.1
])
AC_OUTPUT
