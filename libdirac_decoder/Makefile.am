INCLUDES = -I$(top_srcdir) -I$(srcdir)

h_sources = comp_decompress.h frame_decompress.h seq_decompress.h \
            decoder_types.h dirac_cppparser.h dirac_parser.h

cpp_sources = comp_decompress.cpp frame_decompress.cpp seq_decompress.cpp \
             dirac_cppparser.cpp dirac_parser.cpp

lib_LIBRARIES = libdirac_decoder.a

libdirac_decoder_a_SOURCES = $(h_sources) $(cpp_sources)

library_includedir=$(includedir)/$(GENERIC_LIBRARY_NAME)/libdirac_decoder
library_include_HEADERS = $(h_sources)

if USE_MSVC
CLEANFILES = *.pdb *.ilk
endif

if USE_MSVC
libdirac_decoder_a_AR = link.exe -lib /out:libdirac_decoder.a $(libdirac_decoder_a_OBJECTS) ; echo
else
libdirac_decoder_a_AR = ar crv
endif
