load(common_pre)

# GUI is required for the matrix classes
# Network for host info etc.
QT       += network dbus xml multimedia qml

TARGET = blackcore
TEMPLATE = lib
CONFIG += blackconfig blackmisc blackinput blacksound precompile_header

swiftConfig(static) {
    CONFIG += staticlib
}

INCLUDEPATH += ..
DEPENDPATH += . ..

PRECOMPILED_HEADER = pch/pch.h
INCLUDEPATH += pch

DEFINES += LOG_IN_FILE BUILD_BLACKCORE_LIB

HEADERS += *.h
HEADERS += $$PWD/application/*.h
HEADERS += $$PWD/audio/*.h
HEADERS += $$PWD/context/*.h
HEADERS += $$PWD/data/*.h
HEADERS += $$PWD/db/*.h
HEADERS += $$PWD/vatsim/*.h
HEADERS += $$PWD/fsd/*.h
HEADERS += $$PWD/afv/*.h
HEADERS += $$PWD/afv/audio/*.h
HEADERS += $$PWD/afv/clients/*.h
HEADERS += $$PWD/afv/crypto/*.h
HEADERS += $$PWD/afv/connection/*.h
HEADERS += $$PWD/afv/model/*.h

SOURCES += *.cpp
SOURCES += $$PWD/context/*.cpp
SOURCES += $$PWD/data/*.cpp
SOURCES += $$PWD/db/*.cpp
SOURCES += $$PWD/vatsim/*.cpp
SOURCES += $$PWD/fsd/*.cpp
SOURCES += $$PWD/afv/audio/*.cpp
SOURCES += $$PWD/afv/clients/*.cpp
SOURCES += $$PWD/afv/crypto/*.cpp
SOURCES += $$PWD/afv/connection/*.cpp
SOURCES += $$PWD/afv/model/*.cpp

LIBS *= \
    -lvatlib \
    -lvatsimauth \
    -lsodium \

DESTDIR = $$DestRoot/lib
DLLDESTDIR = $$DestRoot/bin

OTHER_FILES += readme.txt *.xml

win32 {
    dlltarget.path = $$PREFIX/bin
    INSTALLS += dlltarget
} else {
    target.path = $$PREFIX/lib
    INSTALLS += target
}

load(common_post)
