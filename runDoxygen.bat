@echo off&setlocal enableextensions

REM runDoxygen.sh
REM QTMovieSink-106
REM
REM Created by René J.V. Bertin on 20110302.
REM Copyright 2011 IFSTTAR. All rights reserved.

REM  Build the doxygen documentation for the project

REM  A config file 'doxygen.config' must exist
REM  (run 'doxygen -g %SOURCE_ROUTE%\doxygen.config' to a get default file.)

REM  Append the proper input/output directories and docset info to the config file.
REM  This works even though values are assigned higher up in the file. Easier than sed.

SET DEST_NAME=QTils.docset

echo Nous sommes dans %~dp0 (%CD%)

copy /Y %~dp0\doxygen.config %~dp0\doxygentmp.config

echo INPUT = %~dp0 >> %~dp0\doxygentmp.config

echo OUTPUT_DIRECTORY = %~dp0\%DEST_NAME% >> %~dp0\doxygentmp.config
REM set in doxygen.config !
REM echo "DOCSET_BUNDLE_ID       = fr.INRETS.QTils" >> %~dp0\doxygentmp.config

echo GENERATE_DOCSET        = YES >> %~dp0\doxygentmp.config
echo EXCLUDE_PATTERNS += QTVODosx >> %~dp0\doxygentmp.config

REM add the standard compiler tags to the defined tokens:
echo PREDEFINED += _MSC_VER >> %~dp0\doxygentmp.config
echo OPTIMIZE_OUTPUT_FOR_C = YES >> %~dp0\doxygentmp.config

REM  Run doxygen on the updated config file.
REM  Note: doxygen creates a Makefile that does most of the heavy lifting.

start /WAIT /D %~dp0 doxygen %~dp0\doxygentmp.config
REM start /WAIT /D %~dp0 index.hhp
type %~dp0\doxygen-warnings.log