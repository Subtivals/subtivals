#pragma once
#include <QString>
#include <QtGlobal>

struct RemoteScreensConfig {
    bool enabled;
    QString uuid;
    quint16 httpPort = 8080;
    quint16 webSocketPort = 8765;
};
