#include <QApplication>
#include <QTimer>
#include <QUuid>
#include "RemoteScreensDialog.h"
#include "RemoteScreensService.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QCoreApplication::setOrganizationName("Remote");
    QCoreApplication::setApplicationName("RemoteApp");

    RemoteScreensDialog dialog;
    RemoteScreensService service;

    QObject::connect(&dialog, &RemoteScreensDialog::startRequested,
                     &service, &RemoteScreensService::start);
    QObject::connect(&dialog, &RemoteScreensDialog::disableRequested,
                     &service, &RemoteScreensService::disable);

    QObject::connect(&service, &RemoteScreensService::settingsLoaded,
                     &dialog, &RemoteScreensDialog::onSettingsLoaded);
    QObject::connect(&service, &RemoteScreensService::started,
                     &dialog, &RemoteScreensDialog::onServiceStarted);
    QObject::connect(&service, &RemoteScreensService::stopped,
                     &dialog, &RemoteScreensDialog::onServiceStopped);
    QObject::connect(&service, &RemoteScreensService::errorOccurred,
                     &dialog, &RemoteScreensDialog::onServiceError);
    QObject::connect(&service, &RemoteScreensService::clientsConnected,
                     &dialog, &RemoteScreensDialog::clientsConnected);

    dialog.show();

    // Service now owns persistence & autostart:
    service.loadSettingsAndMaybeStart();

    // Every 3s, send a random message to all authorized websocket clients
    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, &service, [&service]{
        const QString msg = QStringLiteral("tick: %1").arg(
            QUuid::createUuid().toString(QUuid::WithoutBraces));
        service.sendMessage(msg);
    });
    timer.start(3000);


    return app.exec();
}
