INCLUDES = -I$(top_srcdir) -I$(srcdir)

bin_PROGRAMS = dirac_decoder

dirac_decoder_SOURCES = decmain.cpp

LDADD = ../libdirac_decoder/libdirac_decoder.a ../libdirac_common/libdirac_common.a

if USE_MSVC
CLEANFILES = *.pdb *.ilk
endif
