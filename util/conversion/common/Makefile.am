INCLUDES = -I$(top_srcdir) -I$(srcdir)

h_sources = setstdiomode.h bitmap.h

cpp_sources = setstdiomode.cpp bitmap.cpp

lib_LIBRARIES = libconv_common.a

libconv_common_a_SOURCES = $(h_sources) $(cpp_sources)

library_includedir=$(includedir)/$(GENERIC_LIBRARY_NAME)
library_include_HEADERS = $(h_sources)

if USE_MSVC
CLEANFILES = *.pdb *.ilk
endif

if USE_MSVC
libconv_common_a_AR = link.exe -lib /out:libconv_common.a $(libconv_common_a_OBJECTS) ; echo
else
libconv_common_a_AR = ar crv
endif
