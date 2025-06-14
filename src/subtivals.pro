#-------------------------------------------------
#
# Project created by QtCreator 2011-06-14T23:35:51
#
#-------------------------------------------------

QT += core gui widgets websockets xml

TARGET = subtivals
TEMPLATE = app
CONFIG += qt

SOURCES += main.cpp\
    mainwindow.cpp \
    script.cpp \
    subtitlestyle.cpp \
    subtitle.cpp \
    subtitlesform.cpp \
    styleeditor.cpp \
    configeditor.cpp \
    player.cpp \
    styleadvanced.cpp \
    shortcuteditor.cpp \
    wizard.cpp \
    weblive.cpp

HEADERS  += mainwindow.h \
    script.h \
    subtitlestyle.h \
    subtitle.h \
    subtitlesform.h \
    styleeditor.h \
    configeditor.h \
    player.h \
    styleadvanced.h \
    shortcuteditor.h \
    wizard.h \
    weblive.h

FORMS    += mainwindow.ui \
    subtitlesform.ui \
    styleeditor.ui \
    configeditor.ui \
    styleadvanced.ui \
    shortcuteditor.ui \
    wizard.ui

RESOURCES += \
    ../resources/icons.qrc \
    ../resources/samples.qrc \
    ../resources/ssl.qrc \
    ../resources/fonts.qrc

TRANSLATIONS = ../locale/fr_FR.ts \
    ../locale/es_ES.ts \
    ../locale/ca_ES.ts

isEmpty(QMAKE_LRELEASE) {
    win32:QMAKE_LRELEASE = $$[QT_INSTALL_BINS]/lrelease.exe
    else: QMAKE_LRELEASE = $$[QT_INSTALL_BINS]/lrelease
}
QMAKE_POST_LINK += $$QMAKE_LRELEASE $$_PRO_FILE_

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

unix:!macx {
    LIBS += -lxcb -lxcb-screensaver -lxcb-dpms
}

win32 {
    TRANSLATIONS_PATH = locale
}

mac {
    QT += svg
    ICON = ../resources/subtivals.icns
}

TRANSLATIONS_PATH_STR = '\\"$${TRANSLATIONS_PATH}\\"'
DEFINES += TRANSLATIONS_PATH=\"$${TRANSLATIONS_PATH_STR}\"

VERSION = 1.10.0
DEFINES += VERSION=\\\"$$VERSION\\\" \
 DEFAULT_FONT_SIZE=36 \
 DEFAULT_FONT_NAME='\\"Tiresias\ Signfont\ Z\\"' \
 DEFAULT_LINESPACING=0.3

OTHER_FILES += \
    ../debian/control \
    ../debian/rules \
    ../debian/changelog \
    ../win-installer/installer.nsi \
    ../win-installer/files.nsi \
    ../win-installer/unfiles.nsi
