INCLUDES = -I$(top_srcdir) -I$(srcdir) -Ilibinstr

bin_PROGRAMS = dirac_instrumentation

h_sources = process_sequence.h

cpp_sources = process_sequence.cpp instrmain.cpp

dirac_instrumentation_SOURCES = $(h_sources) $(cpp_sources)

LDADD = ../../libdirac_encoder/libdirac_encoder.a ../../libdirac_common/libdirac_common.a ../../libdirac_motionest/libdirac_motionest.a libdirac_instrument/libdirac_instrument.a

if USE_MSVC
CLEANFILES = *.pdb *.ilk
endif

SUBDIRS=libdirac_instrument
