bin_PROGRAMS=dirac_unittest

dirac_unittest_SOURCES = utmain.cpp \
                         core_suite.h \
                         cppunit_testsuite.cpp \
						 arrays_test.h \
						 arrays_test.cpp

LDADD = ../libdirac_encoder/libdirac_encoder.a ../libdirac_common/libdirac_common.a ../libdirac_motionest/libdirac_motionest.a -ldl -lcppunit
DEFS = -I.. -I. -I../.. -I$(srcdir)
