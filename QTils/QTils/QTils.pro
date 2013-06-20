TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
    ../QTxml.c \
    ../QTMovieWindowTest.c \
    ../QTMovieWin.c \
    ../QTils.c \
    ../Logger.cpp \
    ../Lists.cpp \
    ../dllmain.cpp \
    ../CritSectEx/CritSectEx.cpp \
    ../mswin/QTMovieWinWM.c \
    ../mswin/AskFileName.c \
    ../mswin/vsscanf.cpp \
    ../CritSectEx/timing.c \
    ../../QTpfuSaveImage.c \
    ../../QTMovieSink_mod2.c \
    ../../QTMovieSink.c

OTHER_FILES += \
    ../Mod2/def/QTVODcomm.def \
    ../Mod2/def/QTilsM2_.def \
    ../Mod2/def/QTilsM2.def \
    ../Mod2/def/POSIXm2.def \
    ../Mod2/def/Chaussette2.def \
    ../Mod2/def/QTVODlib.def \
    ../Mod2/mod/QTVODm2.mod \
    ../Mod2/mod/QTVODlib.mod \
    ../Mod2/mod/QTVODcomm.mod \
    ../Mod2/mod/QTilsM2.mod \
    ../Mod2/mod/POSIXm2.mod \
    ../Mod2/mod/Chaussette2.mod \
    ../Mod2/mod/TstQTVDSrv.mod \
    ../QTilsversion.rc \
    ../mswin/QTils.dll.manifest \
    ../QTils.ico

HEADERS += \
    ../QTMovieWin.h \
    ../POSIXm2.h \
    ../MacErrorTable.h \
    ../Logging.h \
    ../Logger.h \
    ../Lists.h \
    ../copyright.h \
    ../winixdefs.h \
    ../mswin/QTilsIconXOR128x128.h \
    ../mswin/QTilsIconXOR48x48.h \
    ../mswin/vsscanf.h \
    ../mswin/resource.h \
    ../../QTMovieSinkQTStuff.h \
    ../../QTMovieSink.h \
    ../../QTpfuSaveImage.h

