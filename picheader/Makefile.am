INCLUDES = -I$(top_srcdir) -I$(srcdir)

bin_PROGRAMS = make_header

make_header_SOURCES = headmain.cpp

LDADD = ../libdirac_common/libdirac_common.a

if USE_MSVC
CLEANFILES = *.pdb *.ilk
endif
