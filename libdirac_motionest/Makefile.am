INCLUDES = -I$(top_srcdir) -I$(srcdir)

h_sources = block_match.h downconvert.h me_mode_decn.h me_subpel.h me_utils.h motion_estimate.h pixel_match.h

cpp_sources = block_match.cpp downconvert.cpp me_mode_decn.cpp me_subpel.cpp me_utils.cpp motion_estimate.cpp pixel_match.cpp

lib_LIBRARIES = libdirac_motionest.a

libdirac_motionest_a_SOURCES = $(h_sources) $(cpp_sources)

library_includedir=$(includedir)/$(GENERIC_LIBRARY_NAME)/libdirac_motionest
library_include_HEADERS = $(h_sources)

if USE_MSVC
CLEANFILES = *.pdb *.ilk
endif

if USE_MSVC
libdirac_motionest_a_AR = link.exe -lib /out:libdirac_motionest.a $(libdirac_motionest_a_OBJECTS) ; echo
else
libdirac_motionest_a_AR = ar crv
endif
