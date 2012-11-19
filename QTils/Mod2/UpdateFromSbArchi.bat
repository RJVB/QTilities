@echo off&setlocal enableextensions

REM met a jour les fichiers ici a partir de la distribution Archisim

copy /V /Y m:\Archisim\libdrmod\QTVODcomm.mod* mod
copy /V /Y m:\Archisim\libdrmod\QTVODlib.mod* mod
copy /V /Y m:\Archisim\libUtMod\QTilsM2.mod* mod
copy /V /Y m:\Archisim\libUtMod\Chaussette2.mod* mod
copy /V /Y m:\Archisim\mod\QTVODm2.mod* mod
copy /V /Y m:\Archisim\mod\TstQTVDSrv.mod* mod

copy /V /Y m:\Archisim\libdrdef\QTVOD*.def def
copy /V /Y m:\Archisim\libUtDef\QTilsM2.def* def
copy /V /Y m:\Archisim\libUtDef\Chaussette2.def* def

pause
