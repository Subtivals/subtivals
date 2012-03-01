#include <QtGui/QApplication>
#include <QtCore/QFileInfo>
#include <QTranslator>

#include "mainwindow.h"
#include "subtitlesform.h"
#include "configeditor.h"


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QCoreApplication::setOrganizationName("Gedial");
    QCoreApplication::setOrganizationDomain("gedial.com");
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

    QObject::connect(&w, SIGNAL(eventStart(Event*)), &f, SLOT(addEvent(Event*)));
    QObject::connect(&w, SIGNAL(eventEnd(Event*)), &f, SLOT(remEvent(Event*)));
    QObject::connect(&w, SIGNAL(eventClear()), &f, SLOT(clearEvents()));
    QObject::connect(&w, SIGNAL(toggleHide(bool)), &f, SLOT(toggleHide(bool)));
    QObject::connect(&w, SIGNAL(screenResizable(bool)), &f, SLOT(screenResizable(bool)));
    QObject::connect(&f, SIGNAL(geometryChanged(QRect)), w.configEditor(), SLOT(screenChanged(QRect)));
    QObject::connect(w.configEditor(), SIGNAL(changeScreen(int,QRect)), &f, SLOT(changeGeometry(int,QRect)));
    QObject::connect(w.configEditor(), SIGNAL(rotate(double)), &f, SLOT(rotate(double)));
    QObject::connect(w.configEditor(), SIGNAL(styleChanged()), &f, SLOT(repaint()));

    f.show();
    w.show();
    // If more than one arg and last arg is a file, open it
    if( argc > 1)
    {
        QFileInfo fileInfo(argv[argc - 1]);
        if (fileInfo.exists() && fileInfo.isReadable())
        {
           w.openFile(fileInfo.absoluteFilePath());
        }

    }
    return a.exec();
}
