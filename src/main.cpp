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
      CFSTR("Prevent display sleep for " APP_NAME), &assertionID);
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
  QCoreApplication::setOrganizationName(ORG_NAME);
  QCoreApplication::setOrganizationDomain(MAIN_WEBSITE);
  QCoreApplication::setApplicationName(APP_NAME);

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
  QObject::connect(&w, &MainWindow::stateInfo, &service,
                   &RemoteService::stateInfo);
  // TODO: info from movieStarted() lost if user refreshes the page
  QObject::connect(w.player(), &Player::pulse, &service,
                   &RemoteService::playPulse);
  QObject::connect(w.player(), &Player::on, &service,
                   &RemoteService::addSubtitle);
  QObject::connect(w.player(), &Player::off, &service,
                   &RemoteService::remSubtitle);
  QObject::connect(w.player(), &Player::stopped, &service,
                   &RemoteService::clearSubtitles);
  // Remote service -> Play
  QObject::connect(&service, &RemoteService::play, &w, &MainWindow::actionPlay);
  QObject::connect(&service, &RemoteService::pause, &w,
                   &MainWindow::actionPause);
  QObject::connect(&service, &RemoteService::subDelay, w.player(),
                   &Player::subDelay);
  QObject::connect(&service, &RemoteService::addDelay, w.player(),
                   [player = w.player()]() { player->addDelay(); });
  // Remote service -> Remote options dialog
  QObject::connect(&service, &RemoteService::settingsLoaded,
                   w.remoteOptionsDialog(),
                   &RemoteOptionsDialog::onSettingsLoaded);
  QObject::connect(&service, &RemoteService::started, w.remoteOptionsDialog(),
                   &RemoteOptionsDialog::onServiceStarted);
  QObject::connect(&service, &RemoteService::stopped, w.remoteOptionsDialog(),
                   &RemoteOptionsDialog::onServiceStopped);
  QObject::connect(&service, &RemoteService::errorOccurred,
                   w.remoteOptionsDialog(), &RemoteOptionsDialog::onServiceError);
  QObject::connect(&service, &RemoteService::clientsConnected,
                   w.remoteOptionsDialog(),
                   &RemoteOptionsDialog::onClientsConnected);
  // Remote options dialog -> Remote Service
  QObject::connect(w.remoteOptionsDialog(),
                   &RemoteOptionsDialog::startRequested, &service,
                   &RemoteService::start);
  QObject::connect(w.remoteOptionsDialog(),
                   &RemoteOptionsDialog::disableRequested, &service,
                   &RemoteService::disable);
  QObject::connect(w.remoteOptionsDialog(),
                   &RemoteOptionsDialog::setPassphrase, &service,
                   &RemoteService::setPassphrase);

  // Showing subtitles
  w.connectProjectionEvents(&f);

  // Projection Window
  QObject::connect(&w, &MainWindow::screenResizable, &f,
                   &ProjectionWindow::screenResizable);
  QObject::connect(&w, &MainWindow::hideDesktop, &f,
                   &ProjectionWindow::toggleHideDesktop);

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
