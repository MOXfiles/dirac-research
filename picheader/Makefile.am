# $Id$
#

INCLUDES = -I$(top_srcdir) -I$(srcdir)

bin_PROGRAMS = make_header

make_header_SOURCES = headmain.cpp

if USE_MSVC
LDADD = ../libdirac_common/libdirac_common.a
else
LDADD = ../libdirac_common/libdirac_common.la
if USE_STATIC
make_header_LDFLAGS = $(LDFLAGS) -static
endif
endif
if USE_MSVC
CLEANFILES = *.pdb *.ilk
endif
