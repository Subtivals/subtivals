/**
 *  This file is part of Subtivals.
 *
 *  Subtivals is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Subtivals is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Subtivals.  If not, see <http://www.gnu.org/licenses/>
 **/
#include <QApplication>
#include <QFile>
#include <QIODevice>
#include <QSettings>
#include <QStyleFactory>
#include <QTranslator>
#include <QtCore/QFileInfo>
#include <QByteArray>
#include <QIODevice>
#include <QLocale>
#include <QFontDatabase>

#include "configeditor.h"
#include "mainwindow.h"
#include "player.h"
#include "projectionwindow.h"
#include "remoteservice.h"
#include "remoteoptionsdialog.h"

#ifdef Q_OS_MACOS
#include <IOKit/pwr_mgt/IOPMLib.h>

static IOPMAssertionID assertionID;
void disableScreensaver() {
  IOPMAssertionCreateWithName(
      kIOPMAssertionTypeNoDisplaySleep, kIOPMAssertionLevelOn,
      CFSTR("Prevent display sleep for Subtivals"), &assertionID);
}
#endif
#ifdef Q_OS_WIN
#include <windows.h>

void disableScreensaver() {
  SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED);
}
#endif
#ifdef Q_OS_LINUX
#include <xcb/dpms.h>
#include <xcb/screensaver.h>

void disableScreensaver() {
  if (auto *x11Application =
          qGuiApp->nativeInterface<QNativeInterface::QX11Application>()) {
    xcb_connection_t *connection = x11Application->connection();
    xcb_dpms_set_timeouts(connection, 0, 0, 0);
    xcb_screensaver_suspend(connection, XCB_SCREENSAVER_SUSPEND);
  } else {
    // Wayland?
    qWarning() << "Could not disable screensaver on this platform";
  }
}
#endif

int main(int argc, char *argv[]) {
  // Load settings from profile.
  QSettings::setDefaultFormat(QSettings::IniFormat);
  QCoreApplication::setOrganizationName("Subtivals");
  QCoreApplication::setOrganizationDomain("http://subtivals.org");
  QCoreApplication::setApplicationName("Subtivals");

  QSettings settings;
  qInfo() << "Configuration location:" << settings.fileName();

  QApplication a(argc, argv);
  a.setQuitOnLastWindowClosed(true);
  // Disable screensaver
  disableScreensaver();

  // Load translations (i18n) from system locale, but allow to override
  // from configuration.
  settings.beginGroup("Language");
  QString locale =
      settings.value("locale", QLocale::system().name()).toString();
  settings.setValue("locale", locale);
  settings.endGroup();

  QTranslator translator;
  if (translator.load(locale, QString::fromUtf8(TRANSLATIONS_PATH))) {
    a.installTranslator(&translator);
  } else {
    qWarning() << "No translations found for" << locale << "in"
               << TRANSLATIONS_PATH;
  }

  // Load packages fonts.
  if (QFontDatabase::addApplicationFont(":/fonts/TiresiasSignFont.ttf") < 0) {
    qWarning("Failed to load 'TiresiasSignFont'");
  }

  ProjectionWindow f;
  MainWindow w;
  RemoteService service;

  // Player -> Remote service
  QObject::connect(
      &w, SIGNAL(stateInfo(QString, QString, quint64, quint64, QString)),
      &service, SLOT(stateInfo(QString, QString, quint64, quint64, QString)));
  // TODO: info from movieStarted() lost if user refreshes the page
  QObject::connect(w.player(), SIGNAL(pulse(quint64)), &service,
                   SLOT(playPulse(quint64)));
  QObject::connect(w.player(), SIGNAL(on(Subtitle *)), &service,
                   SLOT(addSubtitle(Subtitle *)));
  QObject::connect(w.player(), SIGNAL(off(Subtitle *)), &service,
                   SLOT(remSubtitle(Subtitle *)));
  QObject::connect(w.player(), SIGNAL(stopped()), &service,
                   SLOT(clearSubtitles()));
  // Remote service -> Play
  QObject::connect(&service, SIGNAL(play()), &w, SLOT(actionPlay()));
  QObject::connect(&service, SIGNAL(pause()), &w, SLOT(actionPause()));
  QObject::connect(&service, SIGNAL(subDelay()), w.player(), SLOT(subDelay()));
  QObject::connect(&service, SIGNAL(addDelay()), w.player(), SLOT(addDelay()));
  // Remote service -> Remote options dialog
  QObject::connect(&service,
                   SIGNAL(settingsLoaded(bool, quint16, quint16, QString)),
                   w.remoteOptionsDialog(),
                   SLOT(onSettingsLoaded(bool, quint16, quint16, QString)));
  QObject::connect(&service, SIGNAL(started(QString, QString)),
                   w.remoteOptionsDialog(),
                   SLOT(onServiceStarted(QString, QString)));
  QObject::connect(&service, SIGNAL(stopped()), w.remoteOptionsDialog(),
                   SLOT(onServiceStopped()));
  QObject::connect(&service, SIGNAL(errorOccurred(QString)),
                   w.remoteOptionsDialog(), SLOT(onServiceError(QString)));
  QObject::connect(&service, SIGNAL(clientsConnected(quint16, quint16)),
                   w.remoteOptionsDialog(),
                   SLOT(onClientsConnected(quint16, quint16)));
  // Remote options dialog -> Remote Service
  QObject::connect(w.remoteOptionsDialog(),
                   SIGNAL(startRequested(quint16, quint16)), &service,
                   SLOT(start(quint16, quint16)));
  QObject::connect(w.remoteOptionsDialog(), SIGNAL(disableRequested()),
                   &service, SLOT(disable()));
  QObject::connect(w.remoteOptionsDialog(), SIGNAL(setPassphrase(QString)),
                   &service, SLOT(setPassphrase(QString)));

  // Showing subtitles
  w.connectProjectionEvents(&f);

  // Projection Window
  QObject::connect(&w, SIGNAL(screenResizable(bool)), &f,
                   SLOT(screenResizable(bool)));
  QObject::connect(&w, SIGNAL(hideDesktop(bool)), &f,
                   SLOT(toggleHideDesktop(bool)));

  f.show();
  w.show();

  // Service now owns persistence & autostart:
  service.loadSettingsAndMaybeStart();

  // If more than one arg and last arg is a file, open it
  if (argc > 1) {
    QFileInfo fileInfo(argv[argc - 1]);
    if (fileInfo.exists() && fileInfo.isReadable())
      w.openFile(fileInfo.absoluteFilePath());
  }
  return a.exec();
}
