@echo off
cls
del *.o
del *.prg
cl65 -Osir -t c64 KoalaClient.c -o koalascope.prg
echo.
dir *.prg
copy koalascope.prg w:\
