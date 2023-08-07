@echo off
if not defined DevEnvDir (
	call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
)
if not defined DevEnvDir (
	call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
)

set FILES=main.cpp

set LIBS=opengl32.lib kernel32.lib user32.lib gdi32.lib

cl /nologo /W3 /Z7 /GS- /Gs999999 /O2  /arch:AVX2 /DMAIN_DEBUG=0 %FILES% -FeFPSCameraBasic.exe %LIBS% /link /incremental:no /opt:icf /opt:ref /subsystem:windows

cl /nologo /W3 /Z7 /GS- /Gs999999 /arch:AVX2 /DMAIN_DEBUG=1 %FILES% -FeFPSCameraBasicDebug.exe %LIBS% /link /incremental:no /opt:icf /opt:ref /subsystem:console