#-------------------------------------------------
#
# Project created by QtCreator 2011-06-14T23:35:51
#
#-------------------------------------------------

QT       += core gui

TARGET = subtivals
TEMPLATE = app


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
    resources/icons.qrc \
    resources/samples.qrc

TRANSLATIONS = locale/fr_FR.ts \
    locale/es_ES.ts \
    locale/ca_ES.ts
