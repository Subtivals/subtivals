#-------------------------------------------------
#
# Project created by QtCreator 2011-06-14T23:35:51
#
#-------------------------------------------------

QT       += core gui

TARGET = subtivals
TEMPLATE = app
CONFIG += qt

SOURCES += main.cpp\
        mainwindow.cpp \
    script.cpp \
    style.cpp \
    subtitle.cpp \
    subtitlesform.cpp \
    styleeditor.cpp \
    configeditor.cpp \
    player.cpp

HEADERS  += mainwindow.h \
    script.h \
    style.h \
    subtitle.h \
    subtitlesform.h \
    styleeditor.h \
    configeditor.h \
    player.h

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
    BINDIR = $${PREFIX}/bin
    DATADIR =$${PREFIX}/share
    SHAREDIR = $${DATADIR}/$${TARGET}
    TRANSLATIONS_PATH = $${SHAREDIR}/translations

    INSTALLS += target desktop icon translations

    target.path = $${BINDIR}
    
    desktop.path = $${DATADIR}/applications
    desktop.files += ../resources/$${TARGET}.desktop

    icon.path = $${DATADIR}/icons/hicolor/scalable/apps
    icon.files += ../resources/$${TARGET}.svg
    
    translations.path = $${TRANSLATIONS_PATH}
    translations.files = ../locale/*.qm
}

win32 {
    TRANSLATIONS_PATH = locale
}

mac {
    ICON = ../resources/subtivals.icns
}
TRANSLATIONS_PATH_STR = '\\"$${TRANSLATIONS_PATH}\\"'
DEFINES += TRANSLATIONS_PATH=\"$${TRANSLATIONS_PATH_STR}\"
