# Microsoft Developer Studio Project File - Name="hosC����" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=hosC���� - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "hosC����.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "hosC����.mak" CFG="hosC���� - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "hosC���� - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "hosC���� - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "hosC���� - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386

!ELSEIF  "$(CFG)" == "hosC���� - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /Fp"Debug/handler.pch" /YX /FD /Gs /TC /c
# ADD BASE RSC /l 0x804 /d "_DEBUG"
# ADD RSC /l 0x804 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "hosC���� - Win32 Release"
# Name "hosC���� - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "fs"

# PROP Default_Filter "c"
# Begin Source File

SOURCE=..\file_c\fs\bitmap.c
# End Source File
# Begin Source File

SOURCE=..\file_c\fs\buffer.c
# End Source File
# Begin Source File

SOURCE=..\file_c\fs\minode.c
# End Source File
# Begin Source File

SOURCE=..\file_c\fs\nameinode.c
# End Source File
# Begin Source File

SOURCE=..\file_c\fs\super.c
# End Source File
# Begin Source File

SOURCE=..\file_c\fs\truncate.c
# End Source File
# End Group
# Begin Source File

SOURCE=..\file_c\clock.c
# End Source File
# Begin Source File

SOURCE=..\file_c\floppy.c
# End Source File
# Begin Source File

SOURCE=..\file_c\fnDev_RW_blk.c
# End Source File
# Begin Source File

SOURCE=..\file_c\hd.c
# End Source File
# Begin Source File

SOURCE=..\file_c\interrup.c
# End Source File
# Begin Source File

SOURCE=..\file_c\keyboard.c
# End Source File
# Begin Source File

SOURCE=..\file_c\page.c
# End Source File
# Begin Source File

SOURCE=..\file_c\panic.c
# End Source File
# Begin Source File

SOURCE=..\file_c\pMemory.c
# End Source File
# Begin Source File

SOURCE=..\file_c\process.c
# End Source File
# Begin Source File

SOURCE=..\file_c\startmain.c
# End Source File
# Begin Source File

SOURCE=..\file_c\timer.c
# End Source File
# Begin Source File

SOURCE=..\file_c\tty.c
# End Source File
# Begin Source File

SOURCE=..\file_c\vprintf.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\include\asm.h
# End Source File
# Begin Source File

SOURCE=..\include\blk.h
# End Source File
# Begin Source File

SOURCE=..\file_c\fs\buffer.h
# End Source File
# Begin Source File

SOURCE=..\include\color.h
# End Source File
# Begin Source File

SOURCE=..\include\floppy.h
# End Source File
# Begin Source File

SOURCE=..\file_c\fs\fs.h
# End Source File
# Begin Source File

SOURCE=..\include\hd.h
# End Source File
# Begin Source File

SOURCE=..\include\interrup.h
# End Source File
# Begin Source File

SOURCE=..\include\keyboard.h
# End Source File
# Begin Source File

SOURCE=..\include\logkey.h
# End Source File
# Begin Source File

SOURCE=..\include\pMemory.h
# End Source File
# Begin Source File

SOURCE=..\include\process.h
# End Source File
# Begin Source File

SOURCE=..\include\timer.h
# End Source File
# Begin Source File

SOURCE=..\include\tty.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
