load(common_pre)

QT       += core dbus gui svg network xml multimedia

# QWebEngine is not supported for MinGW
!win32-g++: QT += webenginewidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = swiftlauncher
TEMPLATE = app

SOURCES += *.cpp
HEADERS += *.h
FORMS   += *.ui
CONFIG  += blackmisc blacksound blackinput blackcore blackgui
CONFIG  -= app_bundle

macx {
    deployment.files = ../blackgui/qss
    deployment.path = Contents/MacOS
    QMAKE_BUNDLE_DATA += deployment
}

DEPENDPATH += . $$SourceRoot/src/blackmisc \
                $$SourceRoot/src/blacksound \
                $$SourceRoot/src/blackcore \
                $$SourceRoot/src/blackgui \
                $$SourceRoot/src/blackinput

INCLUDEPATH += . $$SourceRoot/src

OTHER_FILES += *.qss *.ico *.rc
RESOURCES += swiftlauncher.qrc
RC_FILE = swift.rc
DISTFILES += swift.rc

DESTDIR = $$DestRoot/bin

target.path = $$PREFIX/bin
INSTALLS += target

load(common_post)
