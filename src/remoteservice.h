#pragma once
#include <QObject>
#include <QHttpServer>
#include <QWebSocketServer>
#include <QTcpServer>
#include <memory>

#include "subtitle.h"

struct RemoteScreensConfig {
  bool enabled;
  QString uuid;
  quint16 httpPort = 8080;
  quint16 webSocketPort = 8765;
};

class RemoteService : public QObject {
  Q_OBJECT
public:
  explicit RemoteService(QObject *parent = nullptr);
  ~RemoteService() override;

  bool isRunning() const { return m_isRunning; }
  quint16 httpPortInUse() const { return m_httpPortInUse; }
  quint16 webSocketPortInUse() const { return m_webSocketPortInUse; }

public slots:
  void loadSettingsAndMaybeStart();

  // Control
  void start(quint16 httpPort, quint16 webSocketPort);
  void stop();
  void disable();

  // Messages
  void addSubtitle(Subtitle *);
  void remSubtitle(Subtitle *);
  void clearSubtitles();

signals:
  void started(const QString &url);
  void stopped();
  void clientsConnected(quint16 count);
  void errorOccurred(const QString &message);
  void settingsLoaded(const bool enabled, quint16 httpPort,
                      quint16 webSocketPort);

private:
  void sendMessage(const QJsonObject &);
  void saveSettings() const;
  QString effectiveHost() const; // Prefer .local on macOS, else first IPv4
  QString buildViewerUrl(const QString &host) const; // http://host:port/

private:
  RemoteScreensConfig m_config;
  bool m_enabledSetting = false;

  QHttpServer m_httpServer;
  std::unique_ptr<QWebSocketServer> m_webSocketServer;
  std::unique_ptr<QTcpServer> m_tcpServer;
  QSet<QWebSocket *> m_clients; // tracked websocket clients (some may be
                                // unauthorized until handshake)

  bool m_isRunning = false;
  quint16 m_httpPortInUse = 0;
  quint16 m_webSocketPortInUse = 0;

#ifdef Q_OS_MAC
  class QProcess *m_dnsSdProcess = nullptr;
#endif
};
