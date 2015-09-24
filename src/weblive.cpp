#include <QSettings>
#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>

#include "style.h"
#include "weblive.h"

#define DEFAULT_PLAYER_URL "http://live.subtivals.org"
#define DEFAULT_PORT 3141
#define DEFAULT_SCHEME "ws://"


WebLive::WebLive(QObject *parent) :
    QObject(parent),
    m_configured(false),
    m_enabled(false)
{
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
            m_server.setPort(DEFAULT_PORT);
        }
        qDebug() << "Live Server: " << m_server.toString();
        qDebug() << "Player URL: " << m_liveUrl.toString();
    }
    settings.endGroup();

    connect(&m_webSocket, SIGNAL(connected()), this, SLOT(onConnected()));
    connect(&m_webSocket, SIGNAL(disconnected()), this, SLOT(onDisconnected()));
    connect(&m_webSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onError()));
}

WebLive::~WebLive()
{

}

bool WebLive::configured() const {
    return m_configured;
}

void WebLive::enable(bool p_state)
{
    if (m_enabled && !p_state) {
        stop();
    }
    else if (!m_enabled && p_state) {
        start();
    }
}

void WebLive::stop()
{
    qDebug() << "Disconnect...";
    m_enabled = false;
    m_webSocket.close();
}

void WebLive::start()
{
    qDebug() << "Connect...";
    m_enabled = true;
    emit connected(false, liveUrl());
    m_webSocket.open(m_server);
}

void WebLive::onConnected() {
    qDebug() << "Connected!";
    QString message = QString("{\"type\": \"hello-server\", \"channel\": \"%2\"}");
    m_webSocket.sendTextMessage(message.arg(m_secret));
    emit connected(true, liveUrl());
}

void WebLive::onDisconnected() {
    qDebug() << "Disconnected!";
    emit connected(false, m_webSocket.errorString());
}

void WebLive::onError() {
    qDebug() << "Error: " << m_webSocket.errorString();
    emit connected(false, m_webSocket.errorString());
    m_enabled = false;
}

QString WebLive::liveUrl() const
{
    QByteArray ba;
    ba.append(QString("%1|%2").arg(m_server.toString()).arg(m_secret));
    QString key = ba.toBase64();
    return QString("%1/#%2").arg(m_liveUrl.toString()).arg(key);
}

void WebLive::sendJson(const QJsonObject& p_json) {
    if (!m_enabled)
        return;

    QString message(QJsonDocument(p_json).toJson(QJsonDocument::Compact));
    m_webSocket.sendTextMessage(message);
}

void WebLive::addSubtitle(Subtitle *p_subtitle)
{
    QJsonObject json;
    json["type"] = "add-subtitle";
    json["id"] = p_subtitle->index();
    json["content"] = p_subtitle->text();
    json["comments"] = p_subtitle->comments();
    json["channel"] = m_secret;

    const Style* style = p_subtitle->style();
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
        // XXX: second line is ignored.
        const SubtitleLine &firstLine = p_subtitle->lines().at(0);
        if (!firstLine.position().x() > 0)
            jsonPosition["x"] = firstLine.position().x();
        if (!firstLine.position().y() > 0)
            jsonPosition["y"] = firstLine.position().y();
    }

    json["position"] = jsonPosition;

    sendJson(json);
}

void WebLive::remSubtitle(Subtitle *p_subtitle)
{
    QJsonObject json;
    json["type"] = "rem-subtitle";
    json["channel"] = m_secret;
    json["id"] = p_subtitle->index();
    sendJson(json);
}

void WebLive::clearSubtitles()
{
    QJsonObject json;
    json["type"] = "clear";
    json["channel"] = m_secret;
    sendJson(json);
}

void WebLive::toggleHide(bool state)
{
    QJsonObject json;
    json["type"] = "hide";
    json["channel"] = m_secret;
    json["state"] = state;
    sendJson(json);
}
