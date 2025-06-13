#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QSslCertificate>
#include <QSslSocket>
#include <QFile>

#include "style.h"
#include "weblive.h"

#define DEFAULT_PLAYER_URL "https://live.subtivals.org"
#define DEFAULT_PORT_WS 3141
#define DEFAULT_PORT_WSS 8443
#define DEFAULT_SCHEME "ws://"

WebLive::WebLive(QObject *parent)
    : QObject(parent), m_configured(false), m_enabled(false) {

  // Load Let's Encrypts issuer certificate (WebLive feature).
  QFile file(":/ssl/lets-encrypt-r3.pem");
  file.open(QIODevice::ReadOnly);
  const QByteArray bytes = file.readAll();
  const QSslCertificate certificate(bytes);

  QSslConfiguration sslConfiguration(QSslConfiguration::defaultConfiguration());
  sslConfiguration.addCaCertificate(certificate);
  m_webSocket.setSslConfiguration(sslConfiguration);

  // Reload from settings
  QSettings settings;
  settings.beginGroup(QString("Weblive"));
  m_liveUrl = QUrl(settings.value("url", DEFAULT_PLAYER_URL).toString());
  m_secret = settings.value("secret").toString();

  QString server = settings.value("server").toString();
  if (!server.isEmpty() && !m_secret.isEmpty()) {
    m_configured = true;
    m_server.setUrl(server);
    if (m_server.scheme().isEmpty()) {
      m_server.setScheme(DEFAULT_SCHEME);
    }
    if (m_server.port() < 0) {
      m_server.setPort(m_server.scheme() == "wss" ? DEFAULT_PORT_WSS
                                                  : DEFAULT_PORT_WS);
    }
    qDebug() << "Live Server: " << m_server.toString();
    qDebug() << "Player URL: " << m_liveUrl.toString();
  }
  settings.endGroup();

  qDebug() << "SSL stack: " << QSslSocket::sslLibraryVersionString();

  connect(&m_webSocket, SIGNAL(connected()), this, SLOT(onConnected()));
  connect(&m_webSocket, SIGNAL(disconnected()), this, SLOT(onDisconnected()));
  connect(&m_webSocket, SIGNAL(error(QAbstractSocket::SocketError)), this,
          SLOT(onError()));
}

WebLive::~WebLive() {}

bool WebLive::configured() const { return m_configured; }

void WebLive::enable(bool p_state) {
  if (m_enabled && !p_state) {
    stop();
  } else if (!m_enabled && p_state) {
    start();
  }
}

void WebLive::stop() {
  qDebug() << "Disconnect...";
  m_enabled = false;
  m_webSocket.close();
}

void WebLive::start() {
  qDebug() << "Connect...";
  m_enabled = true;
  emit connected(false, liveUrl());
  m_webSocket.open(m_server);
}

void WebLive::onConnected() {
  qDebug() << "Connected!";
  QString message =
      QString("{\"type\": \"hello-server\", \"channel\": \"%2\"}");
  m_webSocket.sendTextMessage(message.arg(m_secret));
  emit connected(true, liveUrl());
}

void WebLive::onDisconnected() {
  qDebug() << "Disconnected!";
  emit connected(false, m_webSocket.errorString());
}

void WebLive::onError() {
  qDebug() << "Error: " << m_webSocket.errorString();
  qDebug()
      << "SSL issuer: "
      << m_webSocket.sslConfiguration().peerCertificate().issuerDisplayName();
  emit connected(false, m_webSocket.errorString());
  m_enabled = false;
}

QString WebLive::liveUrl() const {
  QString urlSecret = QString("%1|%2").arg(m_server.toString(), m_secret);
  QByteArray ba = urlSecret.toUtf8();
  QString key = ba.toBase64();
  return QString("%1/#%2").arg(m_liveUrl.toString(), key);
}

void WebLive::sendJson(const QJsonObject &p_json) {
  if (!m_enabled)
    return;

  QString message(QJsonDocument(p_json).toJson(QJsonDocument::Compact));
  m_webSocket.sendTextMessage(message);
}

void WebLive::addSubtitle(Subtitle *p_subtitle) {
  QJsonObject json;
  json["type"] = "add-subtitle";
  json["id"] = p_subtitle->index();
  json["content"] = p_subtitle->text();
  json["comments"] = p_subtitle->comments();
  json["channel"] = m_secret;

  const Style *style = p_subtitle->style();
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
    const QList<SubtitleLine> lines = p_subtitle->lines();  // Capture the list
    const SubtitleLine &firstLine = lines.at(0);            // Now safe to take a reference
    // XXX: second line is ignored.
    if (firstLine.position().x() <= 0)
      jsonPosition["x"] = firstLine.position().x();
    if (firstLine.position().y() <= 0)
      jsonPosition["y"] = firstLine.position().y();
  }

  json["position"] = jsonPosition;

  sendJson(json);
}

void WebLive::remSubtitle(Subtitle *p_subtitle) {
  QJsonObject json;
  json["type"] = "rem-subtitle";
  json["channel"] = m_secret;
  json["id"] = p_subtitle->index();
  sendJson(json);
}

void WebLive::clearSubtitles() {
  QJsonObject json;
  json["type"] = "clear";
  json["channel"] = m_secret;
  sendJson(json);
}

void WebLive::toggleHide(bool state) {
  QJsonObject json;
  json["type"] = "hide";
  json["channel"] = m_secret;
  json["state"] = state;
  sendJson(json);
}
