#!/bin/sh -x

echo EXPORTS > $1.def
/cygdrive/d/applis/Dev-Cpp/bin/reimp -s $2.lib | sed 's/_//' >> $1.def
/cygdrive/d/applis/Dev-Cpp/bin/dlltool --def $1.def -A -p "_" -C --output-lib $1.a --dllname $2.dll
/cygdrive/d/sbm2-4/StripDebug.exe $1.a
