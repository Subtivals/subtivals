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
#include "subtitlesform.h"
#include "weblive.h"

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
    Display *display = x11Application->display();
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

  SubtitlesForm f;
  MainWindow w;
  WebLive live;

  // Live
  QObject::connect(w.player(), SIGNAL(on(Subtitle *)), &live,
                   SLOT(addSubtitle(Subtitle *)));
  QObject::connect(w.player(), SIGNAL(off(Subtitle *)), &live,
                   SLOT(remSubtitle(Subtitle *)));
  QObject::connect(w.player(), SIGNAL(clear()), &live, SLOT(clearSubtitles()),
                   Qt::DirectConnection);
  QObject::connect(w.configEditor(), SIGNAL(webliveEnabled(bool)), &live,
                   SLOT(enable(bool)));
  QObject::connect(&live, SIGNAL(connected(bool, QString)), w.configEditor(),
                   SLOT(webliveConnected(bool, QString)));
  w.configEditor()->enableWeblive(live.configured());

  // Projection screen
  QObject::connect(w.player(), SIGNAL(on(Subtitle *)), &f,
                   SLOT(addSubtitle(Subtitle *)));
  QObject::connect(w.player(), SIGNAL(off(Subtitle *)), &f,
                   SLOT(remSubtitle(Subtitle *)));
  QObject::connect(w.player(), SIGNAL(clear()), &f, SLOT(clearSubtitles()),
                   Qt::DirectConnection);
  QObject::connect(&w, SIGNAL(toggleHide(bool)), &f, SLOT(toggleHide(bool)));
  QObject::connect(&w, SIGNAL(screenResizable(bool)), &f,
                   SLOT(screenResizable(bool)));
  QObject::connect(&w, SIGNAL(hideDesktop(bool)), &f,
                   SLOT(toggleHideDesktop(bool)));
  QObject::connect(&f, SIGNAL(geometryChanged(QRect)), w.configEditor(),
                   SLOT(screenChanged(QRect)));
  QObject::connect(w.configEditor(), SIGNAL(changeScreen(int, QRect)), &f,
                   SLOT(changeGeometry(int, QRect)));
  QObject::connect(w.configEditor(), SIGNAL(rotate(double)), &f,
                   SLOT(rotate(double)));
  QObject::connect(w.configEditor(), SIGNAL(color(QColor)), &f,
                   SLOT(color(QColor)));
  QObject::connect(w.configEditor(), SIGNAL(outline(QColor, int)), &f,
                   SLOT(outline(QColor, int)));
  QObject::connect(w.configEditor(), SIGNAL(styleChanged()), &f,
                   SLOT(repaint()));

  f.show();
  w.show();

  // If more than one arg and last arg is a file, open it
  if (argc > 1) {
    QFileInfo fileInfo(argv[argc - 1]);
    if (fileInfo.exists() && fileInfo.isReadable())
      w.openFile(fileInfo.absoluteFilePath());
  }
  return a.exec();
}
