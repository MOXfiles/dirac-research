# $Id$
# $Name$
#

INCLUDES = -I$(top_srcdir) -I$(srcdir)

h_sources = block_match.h downconvert.h me_mode_decn.h me_subpel.h me_utils.h motion_estimate.h pixel_match.h global_motion.h model_global_motion.h model_affine.h test_global_motion_model.h test_mv_sad.h reject_motion_vector_outliers.h reject_sad.h reject_local.h reject_intensity.h reject_value.h filter_weighted_local.h refine_motion_vector_field.h reject_outlier.h reject_edge.h model_projective.h

cpp_sources = block_match.cpp downconvert.cpp me_mode_decn.cpp me_subpel.cpp me_utils.cpp motion_estimate.cpp pixel_match.cpp global_motion.cpp model_global_motion.cpp model_affine.cpp test_global_motion_model.cpp test_mv_sad.cpp reject_motion_vector_outliers.cpp reject_sad.cpp reject_local.cpp reject_intensity.cpp reject_value.cpp filter_weighted_local.cpp refine_motion_vector_field.cpp reject_outlier.cpp reject_edge.cpp model_projective.cpp

if USE_MSVC
noinst_LIBRARIES = libdirac_motionest.a
libdirac_motionest_a_AR = link.exe -lib /out:libdirac_motionest.a $(libdirac_motionest_a_OBJECTS) ; echo
else
libdirac_motionest_a_AR = ar crv
noinst_LTLIBRARIES = libdirac_motionest.la
endif

libdirac_motionest_a_SOURCES = $(h_sources) $(cpp_sources)

libdirac_motionest_la_SOURCES = $(h_sources) $(cpp_sources)
libdirac_motionest_la_CXXFLAGS = $(AM_CXXFLAGS)


if USE_MSVC
CLEANFILES = *.pdb *.ilk 
endif

