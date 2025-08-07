@echo off
setlocal

if exist *.obj del /q *.obj
if exist *.pdb del /q *.pdb
if exist *.exe del /q *.exe
if exist *.dll del /q *.dll
if exist *.ilk del /q *.ilk
if exist *.exp del /q *.exp

set SRCPATH=source
set BLDPATH=build
:: To enable audio output please add /DAUDIO_PLAYING to the compiler flags :)
set CFLAGS=/nologo /Zi /Od /W4 /Fe:%BLDPATH%/win32.exe /Fo:%BLDPATH%/ /Fd:%BLDPATH%/game.pdb /Iinclude 
set SOURCE=%SRCPATH%/win32.cpp %SRCPATH%/game.cpp %SRCPATH%/gl.c %SRCPATH%/wgl.c 
set LFLAGS=/DEBUG /SUBSYSTEM:WINDOWS user32.lib gdi32.lib ole32.lib xaudio2.lib opengl32.lib

cl %CFLAGS% %SOURCE% /link %LFLAGS%
REM cl /LD /nologo /Zi /Od /W4 /Iinclude  game.cpp gl.c /link /DEBUG opengl32.lib

endlocal