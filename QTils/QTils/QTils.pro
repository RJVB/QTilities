TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
    ../QTxml.c \
    ../QTMovieWindowTest.c \
    ../QTMovieWin.c \
    ../QTils.c \
    ../POSIXm2.c \
    ../Logger.cpp \
    ../Lists.cpp \
    ../dllPOSIXm2.cpp \
    ../dllmain.cpp

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
    ../POSIXm2version.rc

HEADERS += \
    ../QTMovieWin.h \
    ../POSIXm2.h \
    ../MacErrorTable.h \
    ../Logging.h \
    ../Logger.h \
    ../Lists.h \
    ../copyright.h \
    ../winixdefs.h

