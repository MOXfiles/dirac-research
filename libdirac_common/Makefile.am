AUTOMAKE_OPTIONS = no-dependencies
INCLUDES = -I$(top_srcdir)

h_sources = arith_codec.h arrays.h band_codec.h bit_manager.h common.h \
            context.h frame.h golomb.h gop.h mot_comp.h motion.h \
            mv_codec.h pic_io.h upconvert.h wavelet_utils.h

cpp_sources = band_codec.cpp bit_manager.cpp common.cpp frame.cpp \
              golomb.cpp gop.cpp mot_comp.cpp motion.cpp \
              mv_codec.cpp pic_io.cpp upconvert.cpp wavelet_utils.cpp

lib_LTLIBRARIES = libdirac_common.la

libdirac_common_la_SOURCES = $(h_sources) $(cpp_sources)
libdirac_common_la_LDFLAGS= -version-info $(GENERIC_LIBRARY_VERSION) -release $(GENERIC_RELEASE)

library_includedir=$(includedir)/$(GENERIC_LIBRARY_NAME)
library_include_HEADERS = $(h_sources)

CFLAGS = -O3
DEFS = -I. -I$(srcdir)
