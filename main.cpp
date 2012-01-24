#include <QtGui/QApplication>
#include <QtCore/QFileInfo>

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
    SubtitlesForm f;
    MainWindow w;

    QObject::connect(&w, SIGNAL(eventStart(Event*)), &f, SLOT(addEvent(Event*)));
    QObject::connect(&w, SIGNAL(eventEnd(Event*)), &f, SLOT(remEvent(Event*)));
    QObject::connect(&w, SIGNAL(eventClear()), &f, SLOT(clearEvents()));
    QObject::connect(&w, SIGNAL(toggleHide(bool)), &f, SLOT(toggleHide(bool)));
    QObject::connect(&w, SIGNAL(screenResizable(bool)), &f, SLOT(screenResizable(bool)));
    QObject::connect(&f, SIGNAL(geometryChanged(QRect)), w.configEditor(), SLOT(screenChanged(QRect)));
    QObject::connect(w.configEditor(), SIGNAL(changeScreen(int,QRect)), &f, SLOT(changeGeometry(int,QRect)));
    QObject::connect(w.configEditor(), SIGNAL(styleChanged()), &f, SLOT(repaint()));

    f.show();
    w.show();
    // By default, show the calibration file
    w.actionShowCalibration();
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
