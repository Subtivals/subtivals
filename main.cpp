#include <QtGui/QApplication>

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
    QObject::connect(&w, SIGNAL(configChanged()), &f, SLOT(applyConfig()));
    QObject::connect(&w, SIGNAL(toggleHide(bool)), &f, SLOT(toggleHide(bool)));
    f.show();
    w.show();
    return a.exec();
}
