EXTRA_DIST = win

if HAVE_DOXYGEN
DOC_DIR = doc
else
DOC_DIR =
endif

if HAVE_LATEX
DOC_DIR = doc
endif

if HAVE_CPPUNIT
CPPUNITTESTS_DIR = unit_tests
endif

SUBDIRS = libdirac_common libdirac_motionest libdirac_encoder libdirac_decoder \
encoder decoder picheader util $(CPPUNITTESTS_DIR) tests $(DOC_DIR) 
