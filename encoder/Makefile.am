# $Id$
#

INCLUDES = -I$(top_srcdir) -I$(srcdir)

bin_PROGRAMS = dirac_encoder

dirac_encoder_SOURCES = encmain.cpp

if USE_MSVC
LDADD = ../libdirac_encoder/libdirac_encoder.a ../libdirac_common/libdirac_common.a ../libdirac_motionest/libdirac_motionest.a
else
LDADD = ../libdirac_encoder/libdirac_encoder.la
if USE_STATIC
dirac_encoder_LDFLAGS = $(LDFLAGS) -static
else
endif
endif
if USE_MSVC
CLEANFILES = *.pdb *.ilk
endif
