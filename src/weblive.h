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

    void start(QUrl);
signals:


public slots:
    void onConnected();
    void addSubtitle(Subtitle *p_subtitle);
    void remSubtitle(Subtitle *p_subtitle);
    void clearSubtitles();
    void toggleHide(bool state);

private:
    QWebSocket m_webSocket;
};

#endif // WEBLIVE_H
