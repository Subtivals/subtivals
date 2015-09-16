#include <QSettings>
#include <QDebug>

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
    connect(&m_webSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onError(QAbstractSocket::SocketError)));
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

void WebLive::addSubtitle(Subtitle *p_subtitle)
{
    if (!m_enabled)
        return;
    QString message = "{\"type\": \"add-subtitle\", \"content\": \"%1\", \"channel\": \"%2\"}";
    m_webSocket.sendTextMessage(message.arg(p_subtitle->text()).arg(m_secret));
}

void WebLive::remSubtitle(Subtitle *p_subtitle)
{
    if (!m_enabled)
        return;
    QString message = "{\"type\": \"rem-subtitle\", \"content\": \"%1\", \"channel\": \"%2\"}";
    m_webSocket.sendTextMessage(message.arg(p_subtitle->text()).arg(m_secret));
}

void WebLive::clearSubtitles()
{
    if (!m_enabled)
        return;
    QString message = "{\"type\": \"clear\", \"channel\": \"%2\"}";
    m_webSocket.sendTextMessage(message.arg(m_secret));
}

void WebLive::toggleHide(bool state)
{
    if (!m_enabled)
        return;
    QString message = "{\"type\": \"hide\", \"content\": \"%1\", \"channel\": \"%2\"}";
    m_webSocket.sendTextMessage(message.arg(state ? "true" : "false").arg(m_secret));
}
