
dnl --------------------------------
dnl Initialization macros.
dnl --------------------------------

AC_INIT(encoder/encmain.cpp)

dnl -----------------------------------------------
dnl Package name and version number (user defined)
dnl -----------------------------------------------

GENERIC_LIBRARY_NAME=Dirac

#release versioning
GENERIC_MAJOR_VERSION=0
GENERIC_MINOR_VERSION=2
GENERIC_MICRO_VERSION=0

#shared library versioning
GENERIC_LIBRARY_VERSION=0:2:0
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


dnl -----------------------------------------------
dnl Generates Makefile's, configuration files and scripts
dnl -----------------------------------------------

AC_OUTPUT(Makefile dirac.pc \
          doc/Makefile \
          libdirac_common/Makefile \
          libdirac_motionest/Makefile \
          libdirac_encoder/Makefile \
          libdirac_decoder/Makefile \
          encoder/Makefile \
          decoder/Makefile \
          picheader/Makefile)
