AUTOMAKE_OPTIONS = no-dependencies
INCLUDES = -I$(top_srcdir)

h_sources = comp_decompress.h frame_decompress.h seq_decompress.h

cpp_sources = comp_decompress.cpp frame_decompress.cpp seq_decompress.cpp

lib_LTLIBRARIES = libdirac_decoder.la

libdirac_decoder_la_SOURCES = $(h_sources) $(cpp_sources)
libdirac_decoder_la_LDFLAGS= -version-info $(GENERIC_LIBRARY_VERSION) -release $(GENERIC_RELEASE)

library_includedir=$(includedir)/$(GENERIC_LIBRARY_NAME)
library_include_HEADERS = $(h_sources)

CFLAGS = -O3
DEFS = -I. -I$(srcdir)
