AUTOMAKE_OPTIONS = no-dependencies
bin_PROGRAMS = dirac_encoder

dirac_encoder_SOURCES = encmain.cpp

CFLAGS = -O3
LIBS = -L../libdirac_encoder -L../libdirac_common -L../libdirac_motionest -ldirac_encoder -ldirac_common -ldirac_motionest
DEFS = -I.. -I. -I../.. -I$(srcdir)
