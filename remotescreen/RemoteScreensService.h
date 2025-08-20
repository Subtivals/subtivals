#pragma once
#include "RemoteScreensTypes.h"
#include <QObject>
#include <QHttpServer>
#include <QWebSocketServer>
#include <QTcpServer>
#include <memory>

class RemoteScreensService : public QObject {
    Q_OBJECT
public:
    explicit RemoteScreensService(QObject *parent = nullptr);
    ~RemoteScreensService() override;

    bool isRunning() const { return m_isRunning; }
    quint16 httpPortInUse() const { return m_httpPortInUse; }
    quint16 webSocketPortInUse() const { return m_webSocketPortInUse; }

public slots:
    void loadSettingsAndMaybeStart();

    // Control
    void start(int httpPort, int webSocketPort);
    void stop();
    void disable();

    // Messages
    void sendMessage(const QString &message);

signals:
    void started(const QString &url, quint16 httpPort, quint16 webSocketPort);
    void stopped();
    void clientsConnected(int count);
    void errorOccurred(const QString &message);
    void settingsLoaded(const RemoteScreensConfig &config);

private:
    void saveSettings() const;
    QString effectiveHost() const;         // Prefer .local on macOS, else first IPv4
    QString buildViewerUrl(const QString &host) const; // http://host:port/
    void publishMdns();
    void unpublishMdns();

private:
    RemoteScreensConfig m_config;
    bool m_enabledSetting = false;

    QHttpServer m_httpServer;
    std::unique_ptr<QWebSocketServer> m_webSocketServer;
    std::unique_ptr<QTcpServer> m_tcpServer;
    QSet<QWebSocket*> m_clients; // tracked websocket clients (some may be unauthorized until handshake)

    bool m_isRunning = false;
    quint16 m_httpPortInUse = 0;
    quint16 m_webSocketPortInUse = 0;

#ifdef Q_OS_MAC
    class QProcess *m_dnsSdProcess = nullptr;
#endif
};
