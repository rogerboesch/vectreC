# Try to add option -Wsuggest-override to get a warning when an overriding virtual function
# is missing the override keyword.
# If the option is supported, $(W_SUGGEST_OVERRIDE) can be used in a *_CXXFLAGS variable
# of a Makefile.am file.
#
AC_DEFUN([CXX_SUGGEST_OVERRIDE], [

AC_MSG_CHECKING([if C++ compiler supports -Wsuggest-override])
saved_CXXFLAGS="$CXXFLAGS"
CXXFLAGS="-Wsuggest-override"
AC_COMPILE_IFELSE(
        [AC_LANG_SOURCE([[int main() { return 0; }]])],
        [
            AC_MSG_RESULT(yes)
            W_SUGGEST_OVERRIDE=-Wsuggest-override
            AC_SUBST(W_SUGGEST_OVERRIDE)
        ],
        [
            AC_MSG_RESULT(no)
        ])
CXXFLAGS="$saved_CXXFLAGS"

])
