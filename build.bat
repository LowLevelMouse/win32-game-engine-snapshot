@echo off
setlocal

if exist *.obj del /q *.obj
if exist *.pdb del /q *.pdb
if exist *.exe del /q *.exe
if exist *.ilk del /q *.ilk
if exist *.exp del /q *.exp

:: To enable audio output please add /DAUDIO_PLAYING to the compiler flags :)
set CFLAGS=/nologo /Zi /Od /W4 /Fe:win32.exe /Iinclude 
set SOURCE=win32.cpp gl.c wgl.c
set LFLAGS=/DEBUG /SUBSYSTEM:WINDOWS user32.lib gdi32.lib ole32.lib xaudio2.lib opengl32.lib

cl %CFLAGS% %SOURCE% /link %LFLAGS%

endlocal