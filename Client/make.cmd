@echo off
cls
del *.o
del *.prg
cl65 -Osir -t c64 -C c64-koala.cfg          KoalaClient.c -o koalascope.prg    -vm -m koalascope.map
cl65 -Osir -t c64 -C c64-koala.cfg -D LOCAL KoalaClient.c -o localkoalasc.prg
echo.
dir *.prg
copy koalascope.prg w:\
copy localkoalasc.prg w:\
