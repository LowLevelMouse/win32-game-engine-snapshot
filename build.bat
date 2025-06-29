@echo off
setlocal

del /q *.obj *.pdb *.exe *.ilk *.lib *.exp

set CFLAGS=/nologo /Zi /Od /W4 /Fe:win32.exe /DUSE_STRETCHDIBITS
set SOURCE=win32.cpp
set LFLAGS=/DEBUG /SUBSYSTEM:WINDOWS user32.lib gdi32.lib ole32.lib xaudio2.lib

cl %CFLAGS% %SOURCE% /link %LFLAGS%

endlocal