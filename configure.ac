
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
GENERIC_MINOR_VERSION=5
GENERIC_MICRO_VERSION=0

#shared library versioning
GENERIC_LIBRARY_VERSION=0:0:0
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
AM_SANITY_CHECK
AC_LIBTOOL_WIN32_DLL
AM_PROG_LIBTOOL

dnl -------------------------------------------------------------------------
dnl Check if compiling using MS VC++ compiler in msys environment on Windows
dnl -------------------------------------------------------------------------
use_msvc=no
AC_SUBST([LIBFLAGS],[])
AC_SUBST([LIBEXT],[])
case "$CXX" in
		cl*|CL*)
		CXXFLAGS="-nologo -W1 -EHsc -DWIN32"
		if test x"$enable_shared" = "xyes"; then
		    LIBEXT=".dll";
		    LIBFLAGS="-DLL -INCREMENTAL:NO"
			CXXFLAGS="$CXXFLAGS -D_WINDLL"
		else
		    LIBEXT=".lib";
		    LIBFLAGS="-lib"
		fi
		RANLIB="echo"
		use_msvc=yes
		;;
	*)
		;;
esac
AM_CONDITIONAL(USE_MSVC, test x"$use_msvc" = "xyes")

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

AC_CHECK_PROG(HAVE_DOT, dot, true, false)
	if test $HAVE_DOT = "false"; then
		AC_MSG_WARN([*** dot not found, inheritance diagrams will not be generated in code documentation])
	fi
AM_CONDITIONAL(HAVE_DOT, $HAVE_DOT)

AC_CHECK_PROG(HAVE_LATEX, latex, true, false)
	if test $HAVE_LATEX = "false"; then
		AC_MSG_WARN([*** latex not found, algorithm documentation will not be built])
	fi
AM_CONDITIONAL(HAVE_LATEX, $HAVE_LATEX)

AC_CHECK_PROG(HAVE_DVIPDFM, dvipdfm, true, false)
	if test $HAVE_DVIPDFM = "false"; then
	    AC_MSG_WARN([*** dvipdfm not found, programmer documentation will not be built])
    fi
AM_CONDITIONAL(HAVE_DVIPDFM, $HAVE_DVIPDFM)

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
AC_CONFIG_TESTDIR([tests], [tests:picheader:encoder:decoder:unit_tests:util/instrumentation])
AC_CONFIG_FILES([tests/Makefile])
AC_CONFIG_FILES([tests/at_wrap], [chmod +x tests/at_wrap])
AM_MISSING_PROG([AUTOM4TE], [autom4te])

dnl ----------------------------------------------
dnl enable debug flags
dnl -----------------------------------------------
AC_MSG_CHECKING([whether extra debug flags are to be used when compiling])
AC_ARG_ENABLE(debug, AC_HELP_STRING([--enable-debug], [enable extra debug flags (no)]),
[
AC_LANG_PUSH(C++)
if test x"${enableval}" = x"yes" ; then
	AC_MSG_RESULT(yes)
	case "$CXX" in
	    g++)
			CXXFLAGS=`echo $CXXFLAGS | sed 's/-O[[0-9]]*//g'`
			CXXFLAGS="-DDIRAC_DEBUG -g -O0 $CXXFLAGS"
			SAVE_CXXFLAGS=$CXXFLAGS;
			AC_MSG_CHECKING([whether -Wno-long-double is supported])
			CXXFLAGS="$CXXFLAGS -Wno-long-double"
			AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <math.h>]], [[long double lw;]])],[AC_MSG_RESULT(yes)],[CXXFLAGS=$SAVE_CXXFLAGS; AC_MSG_RESULT(no)])
		    
			# report only errors and warnings as errors
		    CXXFLAGS="$CXXFLAGS -pedantic -Wall -Werror -Woverloaded-virtual"
		    ;;
	    icc)
			CXXFLAGS=`echo $CXXFLAGS | sed 's/-O[[0-9]]*//g'`
			CXXFLAGS="-DDIRAC_DEBUG -g -O0 $CXXFLAGS"
		    # report only errors and warnings as errors
		    CXXFLAGS="$CXXFLAGS -w1 -Werror" 
		    ;;
	    cl*|CL*)
		    # -Od turn off optimisation, -Z7 turn on debugging info
			CXXFLAGS=`echo $CXXFLAGS | sed 's/-O[[0-9]]*//g'`
			CXXFLAGS=`echo $CXXFLAGS | sed 's/-O[[a-z]]*//g'`
			CXXFLAGS=`echo $CXXFLAGS | sed 's/-W[[0-9]]*//g'`
		    CXXFLAGS="$CXXFLAGS -Od -Z7 -W2"
			if test x"$enable_shared" = "xyes"; then
			  LIBFLAGS="$LIBFLAGS -DEBUG";
            fi
		    ;;
        *)
		    # do nothing at the moment
			;;
    esac
AC_LANG_POP(C++)
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
dnl AC_CHECK_HEADER([cppunit/TestRunner.h],
dnl     [AC_CHECK_LIB([cppunit], [main], [HAVE_CPPUNIT="true"], [HAVE_CPPUNIT="false"], [ -ldl ])], [HAVE_CPPUNIT="false"])
AC_CHECK_HEADER([cppunit/TestRunner.h],[HAVE_CPPUNIT="true"], [HAVE_CPPUNIT="false"])
	if test $HAVE_CPPUNIT = "true"; then
		AC_MSG_CHECKING([whether unit tests code will compile and link])
		SAVE_LIBS=$LIBS
		LIBS="$LIBS -lcppunit -ldl"
		AC_RUN_IFELSE([AC_LANG_PROGRAM([[#include <cppunit/TestResult.h> 
		#include <cppunit/TestRunner.h>]], [[CPPUNIT_NS::TestResult controller; CPPUNIT_NS::TestRunner runner;]])],[AC_MSG_RESULT(yes)],[AC_MSG_RESULT(no); HAVE_CPPUNIT="false"])
		LIBS=$SAVE_LIBS
	fi
AM_CONDITIONAL(HAVE_CPPUNIT, $HAVE_CPPUNIT)
AC_LANG_POP(C++)

dnl -----------------------------------------------
dnl Check if static libraries to be used on non-Windows platforms
dnl -----------------------------------------------
AM_CONDITIONAL(USE_STATIC, test x"$enable_static" = "xyes")

dnl -----------------------------------------------
dnl Setup valgrind and necessary args for memcheck
dnl -----------------------------------------------
dnl Using -q --leak-check on valgrind pre-2.2 gives stderr output when
dnl no leaks are found so only use --leak-check with recent valgrind.
dnl valgrind is used during the make valgrind-check target.
AC_CHECK_PROGS(VALGRIND, valgrind)
if test x"$VALGRIND" != "x" ; then
	if $VALGRIND -q --tool=memcheck true 2>/dev/null; then
		VALGRIND="valgrind --tool=memcheck --leak-check=yes"
	fi
fi

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
          unit_tests/Makefile \
          win32/Makefile \
          win32/VS2003/Makefile )
