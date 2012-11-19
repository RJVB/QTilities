@echo off&setlocal enableextensions

echo On est dans %CD% (%~dp0)

IF "%1" == "" GOTO noFile
start /D "%~dp0" QTVODm2.exe -scale 1 -freq 12.5 -chForward 1 -chPilot 2 -chLeft 3 -chRight 4 "%1"
GOTO finished
:noFile
start /D "%~dp0" QTVODm2.exe -scale 1 -freq 12.5 -chForward 1 -chPilot 2 -chLeft 3 -chRight 4

:finished
echo "All Done"
