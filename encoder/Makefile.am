AUTOMAKE_OPTIONS = no-dependencies
bin_PROGRAMS = dirac_encoder

dirac_encoder_SOURCES = encmain.cpp

LDADD = ../libdirac_encoder/libdirac_encoder.a ../libdirac_common/libdirac_common.a ../libdirac_motionest/libdirac_motionest.a
DEFS = -I.. -I. -I../.. -I$(srcdir)
