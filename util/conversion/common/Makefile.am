INCLUDES = -I$(top_srcdir) -I$(srcdir)

h_sources = setstdiomode.h bitmap.h

cpp_sources = setstdiomode.cpp bitmap.cpp

lib_LIBRARIES = libconv_common.a

libconv_common_a_SOURCES = $(h_sources) $(cpp_sources)

library_includedir=$(includedir)/$(GENERIC_LIBRARY_NAME)
library_include_HEADERS = $(h_sources)

