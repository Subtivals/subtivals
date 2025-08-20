#-------------------------------------------------
#
# Project created by QtCreator 2011-06-14T23:35:51
#
#-------------------------------------------------

isEmpty(PACKAGE_VERSION) {
    PACKAGE_VERSION = 0.0.0
}
DEFINES += VERSION=\\\"$$PACKAGE_VERSION\\\" \
 DEFAULT_LINESPACING=0.3 \
 NB_PRESETS=6 \
 DEFAULT_PROJECTION_WINDOW_HEIGHT=200

QT += core gui widgets websockets xml svg httpserver

TARGET = subtivals
TEMPLATE = app
CONFIG += qt

SOURCES += main.cpp\
    mainwindow.cpp \
    script.cpp \
    subtitlestyle.cpp \
    subtitle.cpp \
    subtitlesform.cpp \
    projectionwindow.cpp \
    styleeditor.cpp \
    configeditor.cpp \
    player.cpp \
    styleadvanced.cpp \
    shortcuteditor.cpp \
    wizard.cpp \
    weblive.cpp \
    remoteservice.cpp

HEADERS += mainwindow.h \
    script.h \
    subtitlestyle.h \
    subtitle.h \
    subtitlesform.h \
    projectionwindow.h \
    styleeditor.h \
    configeditor.h \
    player.h \
    styleadvanced.h \
    shortcuteditor.h \
    wizard.h \
    weblive.h \
    remoteservice.h \
    macwindowhelper.h

OBJECTIVE_SOURCES += macwindowhelper.mm

macx {
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.15
}

FORMS += mainwindow.ui \
    subtitlesform.ui \
    styleeditor.ui \
    configeditor.ui \
    styleadvanced.ui \
    shortcuteditor.ui \
    wizard.ui

RESOURCES += \
    ../resources/icons.qrc \
    ../resources/samples.qrc \
    ../resources/fonts.qrc \
    ../resources/www.qrc

# Linux librairies to disable screensaver
unix:!macx {
    LIBS += -lxcb -lxcb-screensaver -lxcb-dpms
}

RC_FILE = ../resources/subtivals.rc

TRANSLATIONS = ../locale/fr_FR.ts \
    ../locale/es_ES.ts \
    ../locale/ca_ES.ts

# Default translation path (used for Windows and fallback)
TRANSLATIONS_PATH = $$PWD/../locale

# Platform-specific install paths
unix:!macx {
    isEmpty(PREFIX) {
        PREFIX = ./usr
    }

    BINDIR = $$PREFIX/bin
    DATADIR = $$PREFIX/share
    SHAREDIR = $$DATADIR/$$TARGET
    TRANSLATIONS_PATH = $$SHAREDIR/translations

    INSTALLS += target desktop icon translations

    target.path = $$BINDIR

    desktop.path = $$DATADIR/applications
    desktop.files += ../resources/$$TARGET.desktop

    icon.path = $$DATADIR/icons/hicolor/scalable/apps
    icon.files += ../resources/$$TARGET.svg

    translations.path = $$TRANSLATIONS_PATH
    translations.files = ../locale/*.qm

    message("Using Unix install path: $$TRANSLATIONS_PATH")
}

macx {
    ICON = ../resources/subtivals.icns
    TRANSLATIONS_PATH = $$OUT_PWD/$${TARGET}.app/Contents/Resources/locale
    translations.files = ../locale/*.qm
    translations.path = $$TRANSLATIONS_PATH
    INSTALLS += translations

    message("Using macOS bundle path: $$TRANSLATIONS_PATH")
}

win32 {
    TRANSLATIONS_PATH = $$OUT_PWD/locale
    translations.files = ../locale/*.qm
    translations.path = $$TRANSLATIONS_PATH
    INSTALLS += translations

    message("Using Windows path: $$TRANSLATIONS_PATH")
}

isEmpty(QMAKE_LRELEASE) {
    win32:QMAKE_LRELEASE = $$[QT_INSTALL_BINS]/lrelease.exe
    else: QMAKE_LRELEASE = $$[QT_INSTALL_BINS]/lrelease
}
QMAKE_POST_LINK += $$QMAKE_LRELEASE $$_PRO_FILE_

# Make sure the destination directory exists and copy .qm files
unix {
    QMAKE_POST_LINK += && $$quote(mkdir -p $$TRANSLATIONS_PATH && cp ../locale/*.qm $$TRANSLATIONS_PATH)
}

# Embed the translation path as a preprocessor define
TRANSLATIONS_PATH_STR = '\"$$TRANSLATIONS_PATH\"'
DEFINES += TRANSLATIONS_PATH=\\\"$$TRANSLATIONS_PATH_STR\\\"

OTHER_FILES += \
    ../debian/control \
    ../debian/rules \
    ../debian/changelog
