EXTRA_DIST = win

if HAVE_DOXYGEN
DOC_DIR = doc
else
DOC_DIR =
endif

if HAVE_LATEX
DOC_DIR = doc
endif

SUBDIRS = util libdirac_common libdirac_motionest libdirac_encoder libdirac_decoder \
encoder decoder picheader tests $(DOC_DIR) 
