#
# Makefile for Microsoft Visual C++
#

CPP = cl
CPPFLAGS = /Ox /EHsc /I.. /nologo

all : libraries encoder decoder conversion instrumentation picheader

libraries : DiracCommon.lib DiracMotionEst.lib DiracDecoder.lib DiracEncoder.lib \
			DiracInstrument.lib

encoder : DiracEncoder.exe

decoder : DiracDecoder.exe

conversion : BMPtoRGB.exe RGBtoBMP.exe RGBtoUYVY.exe RGBtoYUV411.exe \
             RGBtoYUV420.exe RGBtoYUV422.exe RGBtoYUV444.exe UYVYtoRGB.exe \
             YUV411toRGB.exe YUV420toRGB.exe YUV422toRGB.exe YUV444toRGB.exe
             
instrumentation : DiracInstrumentation.exe

picheader : DiracMakeHeader.exe

clean :
	-del *.obj
	-del *.lib
	-del *.exe

DiracCommon.lib : band_codec.obj bit_manager.obj common.obj frame.obj \
	golomb.obj frame_buffer.obj mot_comp.obj motion.obj mv_codec.obj \
	pic_io.obj upconvert.obj wavelet_utils.obj cmd_line.obj \
	dirac_assertions.obj dirac_cppparser.obj dirac_parser.obj
	lib /out:$@ /nologo $**

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

frame_buffer.obj : ..\libdirac_common\frame_buffer.cpp
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
	
cmd_line.obj : ..\libdirac_common\cmd_line.cpp
	$(CPP) $(CPPFLAGS) /c $**
	
dirac_assertions.obj : ..\libdirac_common\dirac_assertions.cpp
	$(CPP) $(CPPFLAGS) /c $**
	
dirac_cppparser.obj : ..\libdirac_decoder\dirac_cppparser.cpp
	$(CPP) $(CPPFLAGS) /c $**
	
dirac_parser.obj : ..\libdirac_decoder\dirac_parser.cpp
	$(CPP) $(CPPFLAGS) /c $**
	
DiracMotionEst.lib : block_match.obj downconvert.obj me_mode_decn.obj me_subpel.obj \
                        me_utils.obj motion_estimate.obj pixel_match.obj
	lib /out:$@ /nologo $**

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

pixel_match.obj : ..\libdirac_motionest\pixel_match.cpp
	$(CPP) $(CPPFLAGS) /c $**

DiracDecoder.lib : comp_decompress.obj frame_decompress.obj seq_decompress.obj
	lib /out:$@ /nologo $**

comp_decompress.obj : ..\libdirac_decoder\comp_decompress.cpp
	$(CPP) $(CPPFLAGS) /c $**

frame_decompress.obj : ..\libdirac_decoder\frame_decompress.cpp
	$(CPP) $(CPPFLAGS) /c $**

seq_decompress.obj : ..\libdirac_decoder\seq_decompress.cpp
	$(CPP) $(CPPFLAGS) /c $**

DiracEncoder.lib : comp_compress.obj frame_compress.obj seq_compress.obj \
				   quality_monitor.obj
	lib /out:$@ /nologo $**

comp_compress.obj : ..\libdirac_encoder\comp_compress.cpp
	$(CPP) $(CPPFLAGS) /c $**

frame_compress.obj : ..\libdirac_encoder\frame_compress.cpp
	$(CPP) $(CPPFLAGS) /c $**

seq_compress.obj : ..\libdirac_encoder\seq_compress.cpp
	$(CPP) $(CPPFLAGS) /c $**

quality_monitor.obj : ..\libdirac_encoder\quality_monitor.cpp
	$(CPP) $(CPPFLAGS) /c $**
        
encmain.obj : ..\encoder\encmain.cpp
	$(CPP) $(CPPFLAGS) /c $**
    
decmain.obj : ..\decoder\decmain.cpp
	$(CPP) $(CPPFLAGS) /c $**

DiracEncoder.exe : encmain.obj DiracEncoder.lib DiracMotionEst.lib DiracCommon.lib
	link /out:DiracEncoder.exe /nologo $**

DiracDecoder.exe : decmain.obj DiracDecoder.lib DiracCommon.lib
	link /out:DiracDecoder.exe /nologo $**


bitmap.obj : ..\util\conversion\common\bitmap.cpp
	$(CPP) $(CPPFLAGS) /c $**

setstdiomode.obj : ..\util\conversion\common\setstdiomode.cpp
	$(CPP) $(CPPFLAGS) /c $**


BMPtoRGB.obj : ..\util\conversion\BMPtoRGB.cpp
	$(CPP) $(CPPFLAGS) /I ..\util\conversion\common /c /c $**

RGBtoBMP.obj : ..\util\conversion\RGBtoBMP.cpp
	$(CPP) $(CPPFLAGS) /I ..\util\conversion\common /c /c $**

RGBtoUYVY.obj : ..\util\conversion\RGBtoUYVY.cpp
	$(CPP) $(CPPFLAGS) /I ..\util\conversion\common /c /c $**

RGBtoYUV411.obj : ..\util\conversion\RGBtoYUV411.cpp
	$(CPP) $(CPPFLAGS) /I ..\util\conversion\common /c /c $**

RGBtoYUV420.obj : ..\util\conversion\RGBtoYUV420.cpp
	$(CPP) $(CPPFLAGS) /I ..\util\conversion\common /c /c $**

RGBtoYUV422.obj : ..\util\conversion\RGBtoYUV422.cpp
	$(CPP) $(CPPFLAGS) /I ..\util\conversion\common /c /c $**

RGBtoYUV444.obj : ..\util\conversion\RGBtoYUV444.cpp
	$(CPP) $(CPPFLAGS) /I ..\util\conversion\common /c /c $**

UYVYtoRGB.obj : ..\util\conversion\UYVYtoRGB.cpp
	$(CPP) $(CPPFLAGS) /I ..\util\conversion\common /c /c $**

YUV411toRGB.obj : ..\util\conversion\YUV411toRGB.cpp
	$(CPP) $(CPPFLAGS) /I ..\util\conversion\common /c /c $**

YUV420toRGB.obj : ..\util\conversion\YUV420toRGB.cpp
	$(CPP) $(CPPFLAGS) /I ..\util\conversion\common /c /c $**

YUV422toRGB.obj : ..\util\conversion\YUV422toRGB.cpp
	$(CPP) $(CPPFLAGS) /I ..\util\conversion\common /c /c $**

YUV444toRGB.obj : ..\util\conversion\YUV444toRGB.cpp
	$(CPP) $(CPPFLAGS) /I ..\util\conversion\common /c /c $**


BMPtoRGB.exe : BMPtoRGB.obj setstdiomode.obj bitmap.obj
		link /out:BMPtoRGB.exe /nologo $**

RGBtoBMP.exe : RGBtoBMP.obj setstdiomode.obj bitmap.obj
		link /out:RGBtoBMP.exe /nologo $**

RGBtoUYVY.exe : RGBtoUYVY.obj setstdiomode.obj bitmap.obj
		link /out:RGBtoUYVY.exe /nologo $**

RGBtoYUV411.exe : RGBtoYUV411.obj setstdiomode.obj bitmap.obj
		link /out:RGBtoYUV411.exe /nologo $**

RGBtoYUV420.exe : RGBtoYUV420.obj setstdiomode.obj bitmap.obj
		link /out:RGBtoYUV420.exe /nologo $**

RGBtoYUV422.exe : RGBtoYUV422.obj setstdiomode.obj bitmap.obj
		link /out:RGBtoYUV422.exe /nologo $**

RGBtoYUV444.exe : RGBtoYUV444.obj setstdiomode.obj bitmap.obj
		link /out:RGBtoYUV444.exe /nologo $**

UYVYtoRGB.exe : UYVYtoRGB.obj setstdiomode.obj bitmap.obj
		link /out:UYVYtoRGB.exe /nologo $**

YUV411toRGB.exe : YUV411toRGB.obj setstdiomode.obj bitmap.obj
		link /out:YUV411toRGB.exe /nologo $**

YUV420toRGB.exe : YUV420toRGB.obj setstdiomode.obj bitmap.obj
		link /out:YUV420toRGB.exe /nologo $**

YUV422toRGB.exe : YUV422toRGB.obj setstdiomode.obj bitmap.obj
		link /out:YUV422toRGB.exe /nologo $**

YUV444toRGB.exe : YUV444toRGB.obj setstdiomode.obj bitmap.obj
		link /out:YUV444toRGB.exe /nologo $**

DiracInstrument.lib : overlay.obj overlay_symbols.obj draw_overlay.obj \
                 motion_arrows.obj motion_colour.obj motion_colour_arrows.obj \
                 pred_mode.obj sad.obj split_mode.obj
	lib /out:$@ /nologo $**
	
overlay.obj : ..\util\instrumentation\libdirac_instrument\overlay.cpp
	$(CPP) $(CPPFLAGS) /c $**
	
overlay_symbols.obj : ..\util\instrumentation\libdirac_instrument\overlay_symbols.cpp
	$(CPP) $(CPPFLAGS) /c $**
	
draw_overlay.obj : ..\util\instrumentation\libdirac_instrument\draw_overlay.cpp
	$(CPP) $(CPPFLAGS) /c $**

motion_arrows.obj : ..\util\instrumentation\libdirac_instrument\motion_arrows.cpp
	$(CPP) $(CPPFLAGS) /c $**

motion_colour.obj : ..\util\instrumentation\libdirac_instrument\motion_colour.cpp
	$(CPP) $(CPPFLAGS) /c $**

motion_colour_arrows.obj : ..\util\instrumentation\libdirac_instrument\motion_colour_arrows.cpp
	$(CPP) $(CPPFLAGS) /c $**

pred_mode.obj : ..\util\instrumentation\libdirac_instrument\pred_mode.cpp
	$(CPP) $(CPPFLAGS) /c $**

sad.obj : ..\util\instrumentation\libdirac_instrument\sad.cpp
	$(CPP) $(CPPFLAGS) /c $**

split_mode.obj : ..\util\instrumentation\libdirac_instrument\split_mode.cpp
	$(CPP) $(CPPFLAGS) /c $**
	
process_sequence.obj : ..\util\instrumentation\process_sequence.cpp
	$(CPP) $(CPPFLAGS) /c $**

instrmain.obj : ..\util\instrumentation\instrmain.cpp
	$(CPP) $(CPPFLAGS) /c $**

DiracInstrumentation.exe : instrmain.obj process_sequence.obj DiracInstrument.lib DiracMotionEst.lib DiracCommon.lib
	link /out:DiracInstrumentation.exe /nologo $**

headmain.obj : ..\picheader\headmain.cpp
	$(CPP) $(CPPFLAGS) /c $**

DiracMakeHeader.exe : headmain.obj DiracCommon.lib
	link /out:DiracMakeHeader.exe /nologo $**
