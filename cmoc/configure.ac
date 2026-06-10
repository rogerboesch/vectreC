AC_PREREQ(2.54)

AC_INIT(cmoc, 0.1.98)
AC_CONFIG_SRCDIR(src/lexer.ll)
AC_CONFIG_MACRO_DIRS([macros])
AC_CANONICAL_HOST
AM_INIT_AUTOMAKE

# Detect a Windows (MinGW/MSYS2) build. Used to exclude the usim 6809
# simulator, which requires termios and is only needed for self-tests.
#
case $host_os in
    mingw*|msys*) windows_build=yes ;;
    *)            windows_build=no ;;
esac
AC_MSG_NOTICE([Windows (MinGW/MSYS) build: $windows_build])
AM_CONDITIONAL([WINDOWS_BUILD], [test "$windows_build" = yes])

PACKAGE_FULL_NAME="CMOC"
PACKAGE_SUMMARY_EN="6809-generating cross-compiler for a subset of the C language"
PACKAGE_SUMMARY_FR="Compilateur C partiel pour le 6809"  # UTF-8
AC_SUBST(PACKAGE_FULL_NAME)
AC_SUBST(PACKAGE_SUMMARY_EN)
AC_SUBST(PACKAGE_SUMMARY_FR)

MANUAL_DATE_EN="March 5th, 2026"; AC_SUBST(MANUAL_DATE_EN)

dnl  Adds an --enable-debug option to configure.
dnl  Must be invoked before AC_PROG_CXX.
dnl
AX_CHECK_ENABLE_DEBUG()

AC_PROG_CXX
AC_LANG_CPLUSPLUS

dnl  The CMOC source code relies the C++11 standard.
dnl  It also uses some POSIX functions that are not declared by default on all platforms.
dnl  We pass [ext] to AX_CXX_COMPILE_STDCXX to use -std=gnu++11 if available,
dnl  instead of -std=c++11.
dnl  The latter does not necessarily define strdup(), fileno() and popen() on Cygwin as of Jan. 2023.
dnl
AX_CXX_COMPILE_STDCXX([11], [ext], [mandatory])

AC_COMPILE_WARNINGS


# Try to add -Wsuggest-override. See macros/cxx_suggest_override.m4.
#
CXX_SUGGEST_OVERRIDE


AC_CHECK_PROG(GREP, grep, grep)
AC_CHECK_PROG(EGREP, egrep, egrep)
AC_CHECK_PROG(ENV, env, env)
AC_CHECK_PROG(HEAD, head, head)
AC_CHECK_PROG(TAIL, tail, tail)
AC_CHECK_PROG(HEXDUMP, hexdump, hexdump)
AC_CHECK_PROG(STRINGS, strings, strings)

# The test suites are Perl scripts.
#
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
	# (Using 2>&1 instead of >& for portability, the latter being specific to GNU Bash.)
	if lwasm --pragma=forwardrefmax --output=/dev/null 2>&1 > /dev/null; then
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


# Determine if Floatable, a not-ready-for-prime-time floating point library,
# gets distributed.
#
distribute_floatable=no
AC_ARG_WITH([floatable],
[  --with-floatable        build and distribute the Floatable floating point library (default: no)],
[
	if test "$withval" = no; then
		distribute_floatable=no
	else
		distribute_floatable=yes
	fi
],
[
	distribute_floatable=no
])
AC_MSG_RESULT([building and distributing Floatable floating point library: $distribute_floatable])
AM_CONDITIONAL(DISTRIBUTE_FLOATABLE, [test "$distribute_floatable" = yes])


# Create a subdirectory in the build tree, in the case of an out-of-source-tree build.
# This directive must appear before AC_CONFIG_FILES and AC_OUTPUT.
#
AC_CONFIG_COMMANDS([create-float-native-dir], [], [$MKDIR_P src/float/native])


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
