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
  QString passphrase;
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
  void setPassphrase(const QString &);

  // Messages
  void stateInfo(const QString state, const QString &title,
                 quint64 totalDuration, quint64 delay,
                 const QString presetName);
  void playPulse(quint64 elapsed);
  void addSubtitle(Subtitle *);
  void remSubtitle(Subtitle *);
  void clearSubtitles();

signals:
  void started(const QString &viewersUrl, const QString &controlUrl);
  void stopped();
  void clientsConnected(quint16 viewersCount, quint16 controlCount);
  void errorOccurred(const QString &message);
  void settingsLoaded(const bool enabled, quint16 httpPort,
                      quint16 webSocketPort, QString passphrase);
  // Remote control
  void play();
  void pause();
  void addDelay();
  void subDelay();

private:
  void sendMessage(const QJsonObject &);
  void saveSettings() const;
  QString effectiveHost() const; // Prefer .local on macOS, else first IPv4
  QString buildViewersUrl(const QString &host) const;
  QString buildControlUrl(const QString &host) const;

private:
  RemoteScreensConfig m_config;
  bool m_enabledSetting = false;

  QHttpServer m_httpServer;
  std::unique_ptr<QWebSocketServer> m_webSocketServer;
  std::unique_ptr<QTcpServer> m_tcpServer;
  QSet<QWebSocket *> m_remoteViewers;  // tracked websocket clients (some may be
  QSet<QWebSocket *> m_remoteControls; // unauthorized until handshake)

  bool m_isRunning = false;
  quint16 m_httpPortInUse = 0;
  quint16 m_webSocketPortInUse = 0;

  bool m_playPulseDebounce = false;

#ifdef Q_OS_MAC
  class QProcess *m_dnsSdProcess = nullptr;
#endif
};
