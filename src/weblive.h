#ifndef WEBLIVE_H
#define WEBLIVE_H

#include <QObject>
#include <QtWebSockets/QWebSocket>

#include "subtitle.h"

class WebLive : public QObject
{
    Q_OBJECT
public:
    explicit WebLive(QObject *parent = 0);
    ~WebLive();

    void start();
    void stop();
    bool configured() const;

signals:
    void connected(bool, QString);

public slots:
    void enable(bool p_state);
    void addSubtitle(Subtitle *p_subtitle);
    void remSubtitle(Subtitle *p_subtitle);
    void clearSubtitles();
    void toggleHide(bool state);

private slots:
    void onConnected();
    void onDisconnected();
    void onError();
    void sendJson(const QJsonObject &p_json);

private:
    QString liveUrl() const;

    QWebSocket m_webSocket;
    bool m_configured;
    bool m_enabled;
    QUrl m_server;
    QString m_secret;
    QUrl m_liveUrl;
};

#endif // WEBLIVE_H
