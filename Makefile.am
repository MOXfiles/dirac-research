SUBDIRS = libdirac_common libdirac_motionest libdirac_encoder libdirac_decoder encoder decoder picheader
EXTRA_DIST = win doc

dist-hook:
	if test -d doc; then \
	  mkdir $(distdir)/doc; \
      cd doc; \
      cp ../../../doc/dirac_algor.tex .; \
      cp ../../../doc/dirac_api.doxygen .; \
      latex dirac_algor.tex; \
      dvipdfm dirac_algor.dvi; \
      doxygen dirac_api.doxygen; \
      cd ..; \
	fi
