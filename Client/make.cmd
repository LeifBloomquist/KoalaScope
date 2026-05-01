@echo off
cls
del *.o
del *.prg
cl65 -Osir -t c64 KoalaClient.c -o koalascope.prg
cl65 -Osir -t c64 -D LOCAL KoalaClient.c -o localkoalasc.prg
echo.
dir *.prg
copy koalascope.prg w:\
copy localkoalasc.prg w:\
