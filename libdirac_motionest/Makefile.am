INCLUDES = -I$(top_srcdir)

h_sources = block_match.h downconvert.h me_mode_decn.h me_subpel.h me_utils.h motion_estimate.h

cpp_sources = block_match.cpp downconvert.cpp me_mode_decn.cpp me_subpel.cpp me_utils.cpp motion_estimate.cpp

lib_LTLIBRARIES = libdirac_motionest.la

libdirac_motionest_la_SOURCES = $(h_sources) $(cpp_sources)
libdirac_motionest_la_LDFLAGS= -version-info $(GENERIC_LIBRARY_VERSION) -release $(GENERIC_RELEASE)

library_includedir=$(includedir)/$(GENERIC_LIBRARY_NAME)
library_include_HEADERS = $(h_sources)

CFLAGS = -O3
DEFS = -I. -I$(srcdir)
