# Microsoft Developer Studio Project File - Name="libdirac_common" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libdirac_common - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libdirac_common.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libdirac_common.mak" CFG="libdirac_common - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libdirac_common - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libdirac_common - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=xicl6.exe
RSC=rc.exe

!IF  "$(CFG)" == "libdirac_common - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=xilink6.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "libdirac_common - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "../../.." /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=xilink6.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "libdirac_common - Win32 Release"
# Name "libdirac_common - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\..\libdirac_common\band_codec.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\libdirac_common\bit_manager.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\libdirac_common\common.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\libdirac_common\frame.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\libdirac_common\golomb.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\libdirac_common\gop.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\libdirac_common\mot_comp.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\libdirac_common\motion.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\libdirac_common\mv_codec.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\libdirac_common\pic_io.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\libdirac_common\upconvert.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\libdirac_common\wavelet_utils.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\..\libdirac_common\arith_codec.h
# End Source File
# Begin Source File

SOURCE=..\..\..\libdirac_common\arrays.h
# End Source File
# Begin Source File

SOURCE=..\..\..\libdirac_common\band_codec.h
# End Source File
# Begin Source File

SOURCE=..\..\..\libdirac_common\bit_manager.h
# End Source File
# Begin Source File

SOURCE=..\..\..\libdirac_common\common.h
# End Source File
# Begin Source File

SOURCE=..\..\..\libdirac_common\context.h
# End Source File
# Begin Source File

SOURCE=..\..\..\libdirac_common\frame.h
# End Source File
# Begin Source File

SOURCE=..\..\..\libdirac_common\golomb.h
# End Source File
# Begin Source File

SOURCE=..\..\..\libdirac_common\gop.h
# End Source File
# Begin Source File

SOURCE=..\..\..\libdirac_common\mot_comp.h
# End Source File
# Begin Source File

SOURCE=..\..\..\libdirac_common\motion.h
# End Source File
# Begin Source File

SOURCE=..\..\..\libdirac_common\mv_codec.h
# End Source File
# Begin Source File

SOURCE=..\..\..\libdirac_common\pic_io.h
# End Source File
# Begin Source File

SOURCE=..\..\..\libdirac_common\upconvert.h
# End Source File
# Begin Source File

SOURCE=..\..\..\libdirac_common\wavelet_utils.h
# End Source File
# End Group
# End Target
# End Project
