# $Id$
#

INCLUDES = -I$(top_srcdir) -I$(srcdir)

bin_PROGRAMS = dirac_decoder

dirac_decoder_SOURCES = decmain.cpp

if USE_MSVC
LDADD = ../libdirac_decoder/libdirac_decoder.a  ../libdirac_common/libdirac_common.a
else
LDADD = ../libdirac_decoder/libdirac_decoder.la
if USE_STATIC
dirac_decoder_LDFLAGS = $(LDFLAGS) -static
endif
endif

if USE_MSVC
CLEANFILES = *.pdb *.ilk
endif
