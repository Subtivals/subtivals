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
#include <QtCore/QFileInfo>
#include <QTranslator>

#include "mainwindow.h"
#include "subtitlesform.h"
#include "configeditor.h"
#include "player.h"
#include "weblive.h"


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QCoreApplication::setOrganizationName("Subtivals");
    QCoreApplication::setOrganizationDomain("https://github.com/traxtech/subtivals/");
    QCoreApplication::setApplicationName("Subtivals");
    a.setQuitOnLastWindowClosed(true);

    // Load translations (i18n) from system locale
    QString locale = QLocale::system().name();
    QTranslator translator;
    if (translator.load(locale, TRANSLATIONS_PATH)) {
        a.installTranslator(&translator);
    }

    SubtitlesForm f;
    MainWindow w;
    WebLive live;

    QObject::connect(w.player(), SIGNAL(on(Subtitle*)), &f, SLOT(addSubtitle(Subtitle*)));
    QObject::connect(w.player(), SIGNAL(off(Subtitle*)), &f, SLOT(remSubtitle(Subtitle*)));
    QObject::connect(w.player(), SIGNAL(clear()), &f, SLOT(clearSubtitles()), Qt::DirectConnection);

    QObject::connect(w.player(), SIGNAL(on(Subtitle*)), &live, SLOT(addSubtitle(Subtitle*)));
    QObject::connect(w.player(), SIGNAL(off(Subtitle*)), &live, SLOT(remSubtitle(Subtitle*)));
    QObject::connect(w.player(), SIGNAL(clear()), &live, SLOT(clearSubtitles()), Qt::DirectConnection);

    QObject::connect(&w, SIGNAL(toggleHide(bool)), &f, SLOT(toggleHide(bool)));
    QObject::connect(&w, SIGNAL(screenResizable(bool)), &f, SLOT(screenResizable(bool)));
    QObject::connect(&f, SIGNAL(geometryChanged(QRect)), w.configEditor(), SLOT(screenChanged(QRect)));
    QObject::connect(w.configEditor(), SIGNAL(changeScreen(int,QRect)), &f, SLOT(changeGeometry(int,QRect)));
    QObject::connect(w.configEditor(), SIGNAL(hideDesktop(bool)), &f, SLOT(toggleHideDesktop(bool)));
    QObject::connect(w.configEditor(), SIGNAL(rotate(double)), &f, SLOT(rotate(double)));
    QObject::connect(w.configEditor(), SIGNAL(color(QColor)), &f, SLOT(color(QColor)));
    QObject::connect(w.configEditor(), SIGNAL(outline(QColor,int)), &f, SLOT(outline(QColor,int)));
    QObject::connect(w.configEditor(), SIGNAL(styleChanged()), &f, SLOT(repaint()));

    f.show();
    w.show();
    live.start(QUrl("ws://live.subtivals.org:3141"));

    // If more than one arg and last arg is a file, open it
    if( argc > 1) {
        QFileInfo fileInfo(argv[argc - 1]);
        if (fileInfo.exists() && fileInfo.isReadable())
           w.openFile(fileInfo.absoluteFilePath());
    }
    return a.exec();
}
