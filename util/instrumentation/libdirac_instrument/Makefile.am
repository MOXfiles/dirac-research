INCLUDES = -I$(top_srcdir)

h_sources = overlay.h draw_overlay.h motion_colour.h motion_arrows.h motion_colour_arrows.h sad.h pred_mode.h split_mode.h overlay_symbols.h

cpp_sources = overlay.cpp draw_overlay.cpp motion_colour.cpp motion_arrows.cpp motion_colour_arrows.cpp sad.cpp pred_mode.cpp split_mode.cpp overlay_symbols.cpp

lib_LIBRARIES = libdirac_instrument.a

libdirac_instrument_a_SOURCES = $(h_sources) $(cpp_sources)

library_includedir=$(includedir)/$(GENERIC_LIBRARY_NAME)
library_include_HEADERS = $(h_sources)

DEFS = -I. -I$(srcdir)
