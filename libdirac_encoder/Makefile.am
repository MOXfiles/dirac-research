INCLUDES = -I$(top_srcdir)

h_sources = comp_compress.h frame_compress.h seq_compress.h

cpp_sources = comp_compress.cpp frame_compress.cpp seq_compress.cpp

lib_LTLIBRARIES = libdirac_encoder.la

libdirac_encoder_la_SOURCES = $(h_sources) $(cpp_sources)
libdirac_encoder_la_LDFLAGS= -version-info $(GENERIC_LIBRARY_VERSION) -release $(GENERIC_RELEASE)

library_includedir=$(includedir)/$(GENERIC_LIBRARY_NAME)
library_include_HEADERS = $(h_sources)

CFLAGS = -O3
DEFS = -I. -I$(srcdir)
