INCLUDES = -I$(top_srcdir)

h_sources = comp_decompress.h frame_decompress.h seq_decompress.h

cpp_sources = comp_decompress.cpp frame_decompress.cpp seq_decompress.cpp

lib_LIBRARIES = libdirac_decoder.a

libdirac_decoder_a_SOURCES = $(h_sources) $(cpp_sources)

library_includedir=$(includedir)/$(GENERIC_LIBRARY_NAME)
library_include_HEADERS = $(h_sources)

DEFS = -I. -I$(srcdir)
