#-------------------------------------------------
#
# Project created by QtCreator 2011-06-14T23:35:51
#
#-------------------------------------------------

QT       += core gui

TARGET = subtivals
TEMPLATE = app
CONFIG += qt debug

SOURCES += main.cpp\
        mainwindow.cpp \
    script.cpp \
    style.cpp \
    event.cpp \
    subtitlesform.cpp \
    styleeditor.cpp \
    configeditor.cpp

HEADERS  += mainwindow.h \
    script.h \
    style.h \
    event.h \
    subtitlesform.h \
    styleeditor.h \
    configeditor.h

FORMS    += mainwindow.ui \
    subtitlesform.ui \
    styleeditor.ui \
    configeditor.ui

RESOURCES += \
    ../resources/icons.qrc \
    ../resources/samples.qrc

TRANSLATIONS = ../locale/fr_FR.ts \
    ../locale/es_ES.ts \
    ../locale/ca_ES.ts

RC_FILE = ../resources/subtivals.rc

unix {
    isEmpty(PREFIX) {
        PREFIX = /usr
    }
    BINDIR = $$PREFIX/bin
    DATADIR =$$PREFIX/share

    INSTALLS += target desktop icon

    target.path = $$BINDIR
    
    desktop.path = $$DATADIR/applications
    desktop.files += ../resources/$${TARGET}.desktop

    icon.path = $$DATADIR/icons/hicolor/scalable/apps
    icon.files += ../resources/$${TARGET}.svg
}
