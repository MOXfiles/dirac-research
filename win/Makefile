#
# Makefile for Microsoft Visual C++
#

CPP = cl
CPPFLAGS = /Ox /EHsc /I.. /nologo

all : libraries encoder decoder

libraries : DiracCommon.lib DiracMotionEst.lib DiracDecoder.lib DiracEncoder.lib 

encoder : DiracEncoder.exe

decoder : DiracDecoder.exe

clean :
	-del *.obj
	-del *.lib
	-del *.exe

DiracCommon.lib : band_codec.obj bit_manager.obj common.obj frame.obj golomb.obj gop.obj \
		mot_comp.obj motion.obj mv_codec.obj pic_io.obj upconvert.obj wavelet_utils.obj
	lib /out:$@ $**

band_codec.obj : ..\libdirac_common\band_codec.cpp
	$(CPP) $(CPPFLAGS) /c $**

bit_manager.obj : ..\libdirac_common\bit_manager.cpp
	$(CPP) $(CPPFLAGS) /c $**

common.obj : ..\libdirac_common\common.cpp
	$(CPP) $(CPPFLAGS) /c $**

frame.obj : ..\libdirac_common\frame.cpp
	$(CPP) $(CPPFLAGS) /c $**

golomb.obj : ..\libdirac_common\golomb.cpp
	$(CPP) $(CPPFLAGS) /c $**

gop.obj : ..\libdirac_common\gop.cpp
	$(CPP) $(CPPFLAGS) /c $**

mot_comp.obj : ..\libdirac_common\mot_comp.cpp
	$(CPP) $(CPPFLAGS) /c $**

motion.obj : ..\libdirac_common\motion.cpp
	$(CPP) $(CPPFLAGS) /c $**

mv_codec.obj : ..\libdirac_common\mv_codec.cpp
	$(CPP) $(CPPFLAGS) /c $**

pic_io.obj  : ..\libdirac_common\pic_io.cpp
	$(CPP) $(CPPFLAGS) /c $**

upconvert.obj  : ..\libdirac_common\upconvert.cpp
	$(CPP) $(CPPFLAGS) /c $**

wavelet_utils.obj : ..\libdirac_common\wavelet_utils.cpp
	$(CPP) $(CPPFLAGS) /c $**
	
DiracMotionEst.lib : block_match.obj downconvert.obj me_mode_decn.obj me_subpel.obj \
                        me_utils.obj motion_estimate.obj
	lib /out:$@ $**

block_match.obj : ..\libdirac_motionest\block_match.cpp
	$(CPP) $(CPPFLAGS) /c $**

downconvert.obj : ..\libdirac_motionest\downconvert.cpp
	$(CPP) $(CPPFLAGS) /c $**

me_mode_decn.obj : ..\libdirac_motionest\me_mode_decn.cpp
	$(CPP) $(CPPFLAGS) /c $**

me_subpel.obj : ..\libdirac_motionest\me_subpel.cpp
	$(CPP) $(CPPFLAGS) /c $**

me_utils.obj : ..\libdirac_motionest\me_utils.cpp
	$(CPP) $(CPPFLAGS) /c $**

motion_estimate.obj : ..\libdirac_motionest\motion_estimate.cpp
	$(CPP) $(CPPFLAGS) /c $**

DiracDecoder.lib : comp_decompress.obj frame_decompress.obj seq_decompress.obj
	lib /out:$@ $**

comp_decompress.obj : ..\libdirac_decoder\comp_decompress.cpp
	$(CPP) $(CPPFLAGS) /c $**

frame_decompress.obj : ..\libdirac_decoder\frame_decompress.cpp
	$(CPP) $(CPPFLAGS) /c $**

seq_decompress.obj : ..\libdirac_decoder\seq_decompress.cpp
	$(CPP) $(CPPFLAGS) /c $**

DiracEncoder.lib : comp_compress.obj frame_compress.obj seq_compress.obj
	lib /out:$@ $**

comp_compress.obj : ..\libdirac_encoder\comp_compress.cpp
	$(CPP) $(CPPFLAGS) /c $**

frame_compress.obj : ..\libdirac_encoder\frame_compress.cpp
	$(CPP) $(CPPFLAGS) /c $**

seq_compress.obj : ..\libdirac_encoder\seq_compress.cpp
	$(CPP) $(CPPFLAGS) /c $**
        
encmain.obj : ..\encoder\encmain.cpp
	$(CPP) $(CPPFLAGS) /GR /Ixparam /c $**
    
decmain.obj : ..\decoder\decmain.cpp
	$(CPP) $(CPPFLAGS) /GR /Ixparam /c $**

DiracEncoder.exe : encmain.obj xparam\xparam.lib DiracEncoder.lib DiracMotionEst.lib DiracCommon.lib
	link /out:DiracEncoder.exe $**

DiracDecoder.exe : decmain.obj xparam\xparam.lib DiracDecoder.lib DiracCommon.lib
	link /out:DiracDecoder.exe $**
