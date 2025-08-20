#include "remoteservice.h"
#include <QFile>
#include <QDir>
#include <QNetworkInterface>
#include <QHostInfo>
#include <QWebSocket>
#include <QProcess>
#include <QUrl>
#include <QResource>
#include <QMimeDatabase>
#include <QSettings>
#include <QUuid>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QHttpHeaders>

#include "subtitlestyle.h"

RemoteService::RemoteService(QObject *parent) : QObject(parent) {
  // "/" -> redirect.
  m_httpServer.route("/", [](const QHttpServerRequest &) {
    QHttpServerResponse response(QHttpServerResponder::StatusCode::Found);
    QHttpHeaders h;
    h.replaceOrAppend(QHttpHeaders::WellKnownHeader::Location,
                      "https://subtivals.org");
    response.setHeaders(std::move(h));
    return response;
  });

  // "/*" -> serve any resource from :/www/
  m_httpServer.route("/.+", [](const QHttpServerRequest &req) {
    QString path = req.url().path();
    if (path.startsWith('/'))
      path.remove(0, 1);

    QString resPath = QString(":/www/%1").arg(path);
    QFile f(resPath);
    if (!f.exists() || !f.open(QIODevice::ReadOnly)) {
      qInfo() << "Subresource not found:" << resPath;
      return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);
    }

    QMimeDatabase mimeDb;
    QMimeType mime = mimeDb.mimeTypeForFile(path);
    QByteArray mimeType = mime.isValid()
                              ? mime.name().toUtf8()
                              : QByteArray("application/octet-stream");

    return QHttpServerResponse(mimeType, f.readAll());
  });
}

RemoteService::~RemoteService() { stop(); }

void RemoteService::loadSettingsAndMaybeStart() {
  // Keep your explicit org/app for consistency with existing data
  QSettings s("Remote", "RemoteApp");

  s.beginGroup("RemoteScreens");
  m_config.enabled = s.value("enabled", false).toBool();
  m_config.httpPort = static_cast<quint16>(s.value("httpPort", 8080).toInt());
  m_config.webSocketPort =
      static_cast<quint16>(s.value("webSocketPort", 8765).toInt());

  // Persist UUID immediately if missing/empty so it survives even when not
  // enabled
  m_config.uuid = s.value("uuid").toString();
  if (m_config.uuid.isEmpty()) {
    m_config.uuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    s.setValue("uuid", m_config.uuid);
    s.sync();
  }
  s.endGroup();

  // Let the UI reflect what we loaded, and auto-start if requested
  emit settingsLoaded(m_config.enabled, m_config.uuid, m_config.httpPort,
                      m_config.webSocketPort);

  qDebug() << "RemoteService loaded settings:" << m_config.enabled
           << m_config.httpPort << m_config.webSocketPort << m_config.uuid;
  if (m_config.enabled)
    start(m_config.httpPort, m_config.webSocketPort);
}

void RemoteService::saveSettings() const {
  QSettings s("Remote", "RemoteApp");
  s.beginGroup("RemoteScreens");
  s.setValue("enabled", m_config.enabled);
  s.setValue("httpPort", static_cast<int>(m_config.httpPort));
  s.setValue("webSocketPort", static_cast<int>(m_config.webSocketPort));
  s.setValue("uuid", m_config.uuid);
  s.endGroup();
  qDebug() << "RemoteService saved settings:" << m_config.enabled
           << m_config.httpPort << m_config.webSocketPort << m_config.uuid;
}

void RemoteService::start(int httpPort, int webSocketPort) {
  if (m_isRunning)
    stop();

  qDebug() << "Starting RemoteService with config:" << httpPort << webSocketPort
           << m_config.uuid;
  m_config.httpPort = static_cast<quint16>(httpPort);
  m_config.webSocketPort = static_cast<quint16>(webSocketPort);
  m_config.enabled = true;
  saveSettings();

  // HTTP: Qt 6.8 requires QTcpServer + bind()
  m_tcpServer = std::make_unique<QTcpServer>(this);
  if (!m_tcpServer->listen(QHostAddress::Any, m_config.httpPort)) {
    if (!m_tcpServer->listen(QHostAddress::Any, 0)) {
      emit errorOccurred(QStringLiteral("Cannot listen HTTP on port %1")
                             .arg(m_config.httpPort));
      m_tcpServer.reset();
      return;
    }
  }
  if (!m_httpServer.bind(m_tcpServer.get())) {
    const auto p = m_tcpServer->serverPort();
    m_tcpServer->close();
    m_tcpServer.reset();
    emit errorOccurred(
        QStringLiteral("QHttpServer bind() failed on port %1").arg(p));
    return;
  }
  m_httpPortInUse = m_tcpServer->serverPort();

  // WebSocket
  m_webSocketServer = std::make_unique<QWebSocketServer>(
      "RemoteScreensWS", QWebSocketServer::NonSecureMode, this);
  if (!m_webSocketServer->listen(QHostAddress::Any, m_config.webSocketPort)) {
    if (!m_webSocketServer->listen(QHostAddress::Any, 0)) {
      m_webSocketServer.reset();
      if (m_tcpServer) {
        m_tcpServer->close();
        m_tcpServer.reset();
      }
      emit errorOccurred(
          QStringLiteral("Cannot bind WebSocket server to port %1")
              .arg(m_config.webSocketPort));
      return;
    }
  }
  m_webSocketPortInUse = m_webSocketServer->serverPort();

  connect(m_webSocketServer.get(), &QWebSocketServer::newConnection, this,
          [this]() {
            QWebSocket *socket = m_webSocketServer->nextPendingConnection();
            m_clients.insert(socket);
            emit clientsConnected(m_clients.size());

            // Not authorized until first valid uuid arrives
            socket->setProperty("authorized", false);

            connect(
                socket, &QWebSocket::textMessageReceived, this,
                [this, socket](const QString &msg) {
                  const bool authorized =
                      socket->property("authorized").toBool();

                  if (!authorized) {
                    // Handshake phase: expect {"uuid": "..."}
                    QJsonParseError parseErr{};
                    const QJsonDocument doc =
                        QJsonDocument::fromJson(msg.toUtf8(), &parseErr);

                    auto sendErrorAndClose = [socket](const QString &reason) {
                      const QJsonObject err{{"event-type", "error"},
                                            {"data", reason}};
                      socket->sendTextMessage(QString::fromUtf8(
                          QJsonDocument(err).toJson(QJsonDocument::Compact)));
                      socket->close();
                    };

                    if (parseErr.error != QJsonParseError::NoError ||
                        !doc.isObject()) {
                      sendErrorAndClose(QStringLiteral("Invalid JSON"));
                      return;
                    }

                    const QJsonObject obj = doc.object();
                    const QString uuid =
                        obj.value(QStringLiteral("uuid")).toString();

                    if (uuid.isEmpty()) {
                      sendErrorAndClose(QStringLiteral("Missing uuid"));
                      return;
                    }
                    if (uuid != m_config.uuid) {
                      sendErrorAndClose(QStringLiteral("UUID mismatch"));
                      return;
                    }

                    // Authorized 🎉
                    socket->setProperty("authorized", true);
                    // (Optional) send an OK event; comment out if you prefer
                    // silence
                    const QJsonObject ok{{"event-type", "ok"},
                                         {"data", "connected"}};
                    socket->sendTextMessage(QString::fromUtf8(
                        QJsonDocument(ok).toJson(QJsonDocument::Compact)));
                    return;
                  }

                  // --- Post-auth messages (your app protocol goes here) ---
                  // Keep echo for now:
                  socket->sendTextMessage(QStringLiteral("echo: ") + msg);
                });

            connect(socket, &QWebSocket::disconnected, this, [this, socket] {
              m_clients.remove(socket);
              emit clientsConnected(m_clients.size());
              socket->deleteLater();
            });
          });

  publishMdns();

  const QString host = effectiveHost();
  const QString url = buildViewerUrl(host);
  m_isRunning = true;
  m_config.enabled = true;
  saveSettings();
  emit started(url, m_httpPortInUse, m_webSocketPortInUse);
  qDebug() << "Started" << url << m_httpPortInUse << m_webSocketPortInUse;
}

void RemoteService::stop() {
  qDebug() << "Stopping RemoteService";
  unpublishMdns();

  if (m_webSocketServer) {
    m_webSocketServer->close();
    m_webSocketServer.reset();
  }
  if (m_tcpServer) {
    m_tcpServer->close();
    m_tcpServer.reset();
  }

  const bool wasRunning = m_isRunning;
  m_isRunning = false;
  m_httpPortInUse = 0;
  m_webSocketPortInUse = 0;
  if (wasRunning) {
    emit stopped();
  }
}

void RemoteService::disable() {
  m_config.enabled = false;
  saveSettings();
  stop();
}

QString RemoteService::effectiveHost() const {

  // Pick the first non-loopback IPv4 on an active interface
  for (const QNetworkInterface &iface : QNetworkInterface::allInterfaces()) {
    const auto flags = iface.flags();
    if (!(flags & QNetworkInterface::IsUp) ||
        !(flags & QNetworkInterface::IsRunning) ||
        (flags & QNetworkInterface::IsLoopBack)) {
      continue;
    }
    for (const QNetworkAddressEntry &ae : iface.addressEntries()) {
      const auto ip = ae.ip();
      if (ip.protocol() == QAbstractSocket::IPv4Protocol) {
        const QString s = ip.toString();
        // Skip link-local 169.254.x.x
        if (!s.startsWith("169.254."))
          return s;
      }
    }
  }
  // Fallbacks: try .local on macOS, else loopback
#ifdef Q_OS_MAC
  return QString("%1.local").arg(QHostInfo::localHostName());
#else
  return QStringLiteral("127.0.0.1");
#endif
}

QString RemoteService::buildViewerUrl(const QString &host) const {
  return QStringLiteral("http://%1:%2/viewer.html#wsPort=%3&uuid=%4")
      .arg(host)
      .arg(m_httpPortInUse)
      .arg(m_webSocketPortInUse)
      .arg(m_config.uuid);
}

void RemoteService::sendMessage(const QJsonObject &p_json) {
  const QString encoded =
      QString::fromUtf8(QJsonDocument(p_json).toJson(QJsonDocument::Compact));

  const auto clients = m_clients;
  for (QWebSocket *socket : clients) {
    if (!socket)
      continue;
    if (!socket->isValid())
      continue;
    if (!socket->property("authorized").toBool())
      continue;

    socket->sendTextMessage(encoded);
  }
}

void RemoteService::addSubtitle(Subtitle *p_subtitle) {
  QJsonObject json;
  json["type"] = "add-subtitle";
  json["id"] = p_subtitle->index();
  json["content"] = p_subtitle->text();
  json["comments"] = p_subtitle->comments();

  const SubtitleStyle *style = p_subtitle->style();
  QJsonObject jsonStyle;
  jsonStyle["name"] = style->name();
  jsonStyle["color"] = style->primaryColour().name();
  jsonStyle["linespacing"] = style->lineSpacing();
  QJsonObject jsonFont;
  jsonFont["size"] = style->font().pixelSize();
  jsonFont["name"] = style->font().family();
  if (style->font().bold())
    jsonFont["color"] = style->font().bold();
  if (style->font().italic())
    jsonFont["italic"] = style->font().italic();
  jsonStyle["font"] = jsonFont;
  json["style"] = jsonStyle;

  QJsonObject jsonPosition;

  QJsonObject jsonMargins;
  if (style->marginL() > 0)
    jsonMargins["left"] = style->marginL();
  if (style->marginR() > 0)
    jsonMargins["right"] = style->marginR();
  if (style->marginV() > 0)
    jsonMargins["vertical"] = style->marginV();
  if (p_subtitle->marginL() > 0)
    jsonMargins["left"] = p_subtitle->marginL();
  if (p_subtitle->marginR() > 0)
    jsonMargins["right"] = p_subtitle->marginR();
  if (p_subtitle->marginV() > 0)
    jsonMargins["vertical"] = p_subtitle->marginV();
  jsonPosition["margin"] = jsonMargins;

  if (p_subtitle->nbLines() > 0) {
    const QList<SubtitleLine> lines = p_subtitle->lines(); // Capture the list
    const SubtitleLine &firstLine = lines.at(0); // Now safe to take a reference
    // XXX: second line is ignored.
    if (firstLine.position().x() <= 0)
      jsonPosition["x"] = firstLine.position().x();
    if (firstLine.position().y() <= 0)
      jsonPosition["y"] = firstLine.position().y();
  }

  json["position"] = jsonPosition;

  sendMessage(json);
}

void RemoteService::remSubtitle(Subtitle *p_subtitle) {
  QJsonObject json;
  json["type"] = "rem-subtitle";
  json["id"] = p_subtitle->index();
  sendMessage(json);
}

void RemoteService::clearSubtitles() {
  QJsonObject json;
  json["type"] = "clear";
  sendMessage(json);
}

void RemoteService::publishMdns() {
#ifdef Q_OS_MAC
  unpublishMdns();
  qDebug() << "Publishing mDNS service";
  m_dnsSdProcess = new QProcess(this);
  const QString name =
      QStringLiteral("RemoteScreens-%1").arg(m_config.uuid.left(8));
  m_dnsSdProcess->start("dns-sd", {"-R", name, "_http._tcp", ".",
                                   QString::number(m_httpPortInUse)});
#endif
}

void RemoteService::unpublishMdns() {
#ifdef Q_OS_MAC
  qDebug() << "Unpublishing mDNS service";
  if (m_dnsSdProcess) {
    m_dnsSdProcess->kill();
    m_dnsSdProcess->waitForFinished(1000);
    m_dnsSdProcess->deleteLater();
    m_dnsSdProcess = nullptr;
  }
#endif
}
