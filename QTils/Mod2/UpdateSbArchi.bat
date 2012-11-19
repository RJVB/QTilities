@echo off&setlocal enableextensions

REM met a jour les fichiers dans la distribution Archisim

copy /V /Y mod\QTVODcomm.mod* m:\Archisim\libdrmod\
copy /V /Y mod\QTVODlib.mod* m:\Archisim\libdrmod\
copy /V /Y mod\QTilsM2.mod* m:\Archisim\libUtMod\
copy /V /Y mod\Chaussette2.mod* m:\Archisim\libUtMod\
copy /V /Y mod\QTVODm2.mod* m:\Archisim\mod\
copy /V /Y mod\TstQTVDSrv.mod* m:\Archisim\mod\

copy /V /Y def\QTVOD*.def m:\Archisim\libdrdef\
copy /V /Y def\QTilsM2.def* m:\Archisim\libUtDef\
copy /V /Y def\Chaussette2.def* m:\Archisim\libUtDef\

copy /B /V /Y QTils*.dll m:\SbArchi\

pause