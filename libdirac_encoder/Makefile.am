INCLUDES = -I$(top_srcdir)

h_sources = comp_compress.h frame_compress.h seq_compress.h

cpp_sources = comp_compress.cpp frame_compress.cpp seq_compress.cpp

lib_LIBRARIES = libdirac_encoder.a

libdirac_encoder_a_SOURCES = $(h_sources) $(cpp_sources)

library_includedir=$(includedir)/$(GENERIC_LIBRARY_NAME)
library_include_HEADERS = $(h_sources)

DEFS = -I. -I$(srcdir)
