
dnl --------------------------------
dnl Initialization macros.
dnl --------------------------------

AC_INIT(encoder/encmain.cpp)

dnl -----------------------------------------------
dnl Package name and version number (user defined)
dnl -----------------------------------------------

GENERIC_LIBRARY_NAME=dirac

#release versioning
GENERIC_MAJOR_VERSION=0
GENERIC_MINOR_VERSION=3
GENERIC_MICRO_VERSION=1

#shared library versioning
GENERIC_LIBRARY_VERSION=0:3:1
#                       | | |
#                +------+ | +---+
#                |        |     |
#             current:revision:age
#                |        |     |
#                |        |     +- increment if interfaces have been added
#                |        |        set to zero if interfaces have been removed
#                                  or changed
#                |        +- increment if source code has changed
#                |           set to zero if current is incremented
#                +- increment if interfaces have been added, removed or changed


dnl --------------------------------
dnl Package name and version number
dnl --------------------------------

AC_SUBST(GENERIC_LIBRARY_VERSION)

PACKAGE=$GENERIC_LIBRARY_NAME
AC_SUBST(GENERIC_LIBRARY_NAME)

GENERIC_VERSION=$GENERIC_MAJOR_VERSION.$GENERIC_MINOR_VERSION.$GENERIC_MICRO_VERSION
GENERIC_RELEASE=$GENERIC_MAJOR_VERSION.$GENERIC_MINOR_VERSION
AC_SUBST(GENERIC_RELEASE)
AC_SUBST(GENERIC_VERSION)

VERSION=$GENERIC_VERSION

AM_INIT_AUTOMAKE($PACKAGE, $VERSION, no-define)

dnl -----------------------------------------------
dnl Checks for required programs.
dnl -----------------------------------------------
AC_PROG_CXX
AC_PROG_RANLIB
AM_SANITY_CHECK

dnl -----------------------------------------------
dnl Checks for required headers
dnl -----------------------------------------------

AC_HEADER_STDC
AC_HEADER_STDBOOL

dnl ** We probably need this line, but not now
dnl AC_CHECK_HEADERS(stdint.h unistd.h fcntl.h)

dnl -----------------------------------------------
dnl Checks for libraries.
dnl -----------------------------------------------

dnl -----------------------------------------------
dnl Checks for optional programs.
dnl -----------------------------------------------
AC_CHECK_PROG(HAVE_DOXYGEN, doxygen, true, false)
	if test $HAVE_DOXYGEN = "false"; then
		AC_MSG_WARN([*** doxygen not found, code documentation will not be built])
	fi
AM_CONDITIONAL(HAVE_DOXYGEN, $HAVE_DOXYGEN)
AC_CHECK_PROG(HAVE_LATEX, latex, true, false)
	if test $HAVE_LATEX = "false"; then
		AC_MSG_WARN([*** latex not found, algorithm documentation will not be built])
	fi
AM_CONDITIONAL(HAVE_LATEX, $HAVE_LATEX)

AC_ARG_ENABLE(overlay,
[ --enable-overlay Turns onDirectory instrumentation],
[case "${enableval}" in
	yes) overlay=true ;;
	no) overlay=false ;;
	*) AC_MSG_ERROR(bad value ${enableval} for --enable-overlay) ;;
esac],[overlay=false])
AM_CONDITIONAL(OVERLAY, test x$overlay = xtrue)

dnl -----------------------------------------------
dnl Checks for optional files in doc directory.
dnl algorithm documentation is not distributed in
dnl the release. It is available only from CVS
dnl -----------------------------------------------
AC_CHECK_FILE(doc/algorithm/algorithm.htm,[HAVE_ALGO="true"], [HAVE_ALGO="false"])
AM_CONDITIONAL(HAVE_ALGO, $HAVE_ALGO)


dnl -----------------------------------------------
dnl Setup for the testsuite
dnl -----------------------------------------------
AC_CONFIG_TESTDIR([tests], [tests:picheader:encoder:decoder:unit_tests])
AC_CONFIG_FILES([tests/Makefile])
AM_MISSING_PROG([AUTOM4TE], [autom4te])

dnl ----------------------------------------------
dnl enable debug flags
dnl -----------------------------------------------
AC_MSG_CHECKING([whether extra debug flags are to be used when compiling])
AC_ARG_ENABLE(debug, AC_HELP_STRING([--enable-debug], [enable extra debug flags (no)]),
[
if test x"${enableval}" = x"yes" ; then
	AC_MSG_RESULT(yes)
	CXXFLAGS=`echo $CXXFLAGS | sed 's/-O[[0-9]]*//g'`
	CXXFLAGS="-DDIRAC_DEBUG -g -O0 $CXXFLAGS"
	case "$CXX" in
	    g++)
		    # report only errors and warnings as errors
		    CXXFLAGS="$CXXFLAGS -pedantic -Wall -Werror -Woverloaded-virtual"
		    ;;
	    icc)
		    # report only errors and warnings as errors
		    CXXFLAGS="$CXXFLAGS -w1 -Werror" 
		    ;;
        *)
		    # do nothing at the moment
			;;
    esac
else
	AC_MSG_RESULT(no)
fi], [AC_MSG_RESULT(no)]
)

dnl ----------------------------------------------
dnl enable profiling flags
dnl -----------------------------------------------
AC_MSG_CHECKING([whether gprof flags are to be used when compiling])
AC_ARG_ENABLE(profile, AC_HELP_STRING([--enable-profile], [enable profile flags (no)]),
[
if test x"${enableval}" = x"yes" ; then
	AC_MSG_RESULT(yes)
	case "$CXX" in
	    g++|icc)
	        CXXFLAGS="$CXXFLAGS -pg"
			;;
        *)
		    # do nothing
			;;
    esac
else
	AC_MSG_RESULT(no)
fi], [AC_MSG_RESULT(no)]
)

dnl -----------------------------------------------
dnl Setup for the cppunit testsuite
dnl -----------------------------------------------
AC_LANG_PUSH(C++)
dnl need to determine how to run AC_CHECK_LIB on a c++ library
dnl Until then just use AC_CHECK_HEADER by itself
dnl AC_CHECK_HEADER([cppunit/Asserter.h],
dnl     [AC_CHECK_LIB([cppunit], [CppUnit::Exception], [HAVE_CPPUNIT="true"], [HAVE_CPPUNIT="false"], [ -ldl ])], [HAVE_CPPUNIT="false"])
AC_CHECK_HEADER([cppunit/Asserter.h],[HAVE_CPPUNIT="true"], [HAVE_CPPUNIT="false"])
AM_CONDITIONAL(HAVE_CPPUNIT, $HAVE_CPPUNIT)
AC_LANG_POP(C++)

dnl -----------------------------------------------
dnl Generates Makefile's, configuration files and scripts
dnl -----------------------------------------------

AC_OUTPUT(Makefile dirac.pc \
          util/Makefile \
          util/conversion/Makefile \
          util/conversion/common/Makefile \
	  util/instrumentation/Makefile \
	  util/instrumentation/libdirac_instrument/Makefile \
          doc/Makefile \
          libdirac_common/Makefile \
          libdirac_motionest/Makefile \
          libdirac_encoder/Makefile \
          libdirac_decoder/Makefile \
          encoder/Makefile \
          decoder/Makefile \
          picheader/Makefile \
          unit_tests/Makefile )
