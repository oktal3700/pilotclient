QT       += core
QT       -= gui

TARGET = sample_logging
TEMPLATE = app

CONFIG   += console
CONFIG   -= app_bundle

DEPENDPATH += . ../../src

INCLUDEPATH += . ../../src

SOURCES += main.cpp

LIBS    += -L../../lib -lblackmisc

win32:  PRE_TARGETDEPS += ../../lib/blackmisc.lib
else:   PRE_TARGETDEPS += ../../lib/libblackmisc.a

DESTDIR = ../../bin



