INCLUDES = -I$(top_srcdir) -I$(srcdir)

h_sources = comp_compress.h frame_compress.h quality_monitor.h seq_compress.h

cpp_sources = comp_compress.cpp frame_compress.cpp quality_monitor.cpp seq_compress.cpp

lib_LIBRARIES = libdirac_encoder.a

libdirac_encoder_a_SOURCES = $(h_sources) $(cpp_sources)

library_includedir=$(includedir)/$(GENERIC_LIBRARY_NAME)/libdirac_encoder
library_include_HEADERS = $(h_sources)

if USE_MSVC
CLEANFILES = *.pdb *.ilk
endif

if USE_MSVC
libdirac_encoder_a_AR = link.exe -lib /out:libdirac_encoder.a $(libdirac_encoder_a_OBJECTS) ; echo
else
libdirac_encoder_a_AR = ar crv
endif
