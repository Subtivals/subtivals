#include <QtGui/QApplication>
#include <QtCore/QFileInfo>

#include "mainwindow.h"
#include "subtitlesform.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QCoreApplication::setOrganizationName("Gedial");
    QCoreApplication::setOrganizationDomain("gedial.com");
    QCoreApplication::setApplicationName("Subtivals");
    a.setQuitOnLastWindowClosed(true);
    SubtitlesForm f;
    MainWindow w;
    QObject::connect(&w, SIGNAL(eventStart(Event*)), &f, SLOT(addEvent(Event*)));
    QObject::connect(&w, SIGNAL(eventEnd(Event*)), &f, SLOT(remEvent(Event*)));
    QObject::connect(&w, SIGNAL(eventClear()), &f, SLOT(clearEvents()));
    QObject::connect(&w, SIGNAL(configChanged()), &f, SLOT(applyConfig()));
    QObject::connect(&f, SIGNAL(configChanged()), &w, SLOT(onConfigChanged()));
    QObject::connect(&w, SIGNAL(toggleHide(bool)), &f, SLOT(toggleHide(bool)));
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
