bin_PROGRAMS = dirac_instrumentation

h_sources =

cpp_sources = instrmain.cpp

dirac_instrumentation_SOURCES = $(h_sources) $(cpp_sources)

LDADD = ../../libdirac_encoder/libdirac_encoder.a ../../libdirac_common/libdirac_common.a ../../libdirac_motionest/libdirac_motionest.a libdirac_instrument/libdirac_instrument.a

DEFS = -I.. -I. -I../.. -Ilibinstr/ -I$(srcdir)

if OVERLAY
DEFS += -DOVERLAY=1
endif

SUBDIRS=libdirac_instrument
