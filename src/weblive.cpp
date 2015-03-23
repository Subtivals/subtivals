#include "weblive.h"

WebLive::WebLive(QObject *parent) : QObject(parent)
{
}

WebLive::~WebLive()
{

}

void WebLive::start(QUrl p_url)
{
    connect(&m_webSocket, &QWebSocket::connected, this, &WebLive::onConnected);
    m_webSocket.open(p_url);
}

void WebLive::onConnected() {
    m_webSocket.sendTextMessage("{\"type\": \"hello-server\"}");
}

void WebLive::addSubtitle(Subtitle *p_subtitle)
{
    QString message = "{\"type\": \"add-subtitle\", \"content\": \"%1\"}";
    m_webSocket.sendTextMessage(message.arg(p_subtitle->text()));
}

void WebLive::remSubtitle(Subtitle *p_subtitle)
{
    QString message = "{\"type\": \"rem-subtitle\", \"content\": \"%1\"}";
    m_webSocket.sendTextMessage(message.arg(p_subtitle->text()));
}

void WebLive::clearSubtitles()
{
    QString message = "{\"type\": \"clear\"}";
    m_webSocket.sendTextMessage(message);
}

void WebLive::toggleHide(bool state)
{
    QString message = "{\"type\": \"hide\", \"content\": \"%1\"}";
    m_webSocket.sendTextMessage(message.arg(state ? "true" : "false"));
}
