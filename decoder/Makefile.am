AUTOMAKE_OPTIONS = no-dependencies
bin_PROGRAMS = dirac_decoder

dirac_decoder_SOURCES = decmain.cpp

LDADD = ../libdirac_decoder/libdirac_decoder.a ../libdirac_common/libdirac_common.a
DEFS = -I. -I.. -I../.. -I$(srcdir)
