INCLUDES = -I$(top_srcdir) -I$(srcdir)

h_sources = arith_codec.h arrays.h band_codec.h bit_manager.h \
            common_types.h common.h frame.h golomb.h frame_buffer.h \
            mot_comp.h motion.h mv_codec.h pic_io.h upconvert.h \
            wavelet_utils.h cmd_line.h dirac_assertions.h

cpp_sources = band_codec.cpp bit_manager.cpp common.cpp \
              frame.cpp golomb.cpp frame_buffer.cpp mot_comp.cpp motion.cpp \
              mv_codec.cpp pic_io.cpp upconvert.cpp wavelet_utils.cpp \
              cmd_line.cpp dirac_assertions.cpp

lib_LIBRARIES = libdirac_common.a

libdirac_common_a_SOURCES = $(h_sources) $(cpp_sources)

library_includedir=$(includedir)/$(GENERIC_LIBRARY_NAME)/libdirac_common
library_include_HEADERS = $(h_sources)

if USE_MSVC
CLEANFILES = *.pdb *.ilk
endif

if USE_MSVC
libdirac_common_a_AR = link.exe -lib /out:libdirac_common.a $(libdirac_common_a_OBJECTS) ; echo
else
libdirac_common_a_AR = ar crv
endif
