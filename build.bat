@echo off
setlocal

del /q *.obj *.pdb *.exe *.ilk *.lib *.exp

set CFLAGS=/nologo /Zi /Od /W4 /Fe:win32.exe /Iinclude
set SOURCE=win32.cpp gl.c wgl.c
set LFLAGS=/DEBUG /SUBSYSTEM:WINDOWS user32.lib gdi32.lib ole32.lib xaudio2.lib opengl32.lib

cl %CFLAGS% %SOURCE% /link %LFLAGS%

endlocal