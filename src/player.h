#ifndef PLAYER_H
#define PLAYER_H

#include <QObject>
#include <QtCore/QTimer>
#include <QtCore/QList>


#define DELAY_OFFSET 250

class Script;
class Event;

class Player : public QObject
{
    Q_OBJECT
public:
    explicit Player(QObject *parent = 0);
    void setScript(Script*);
    QList<Event*> current();
    qlonglong elapsedTime();
    int delay();
signals:
    void pulse(qint64);
    void clear();
    void on(Event*);
    void off(Event*);
    void changed();
    void autoHide();
public slots:
    void play();
    void pause();
    void stop();
    void jumpTo(int);
    void enableSpeedFactor(bool p_state);
    void setElapsedTime(qint64);
    void addDelay(int d = DELAY_OFFSET);
    void subDelay(int d = DELAY_OFFSET);
    void setSpeedFactor(double);
    void autoHideTimeout();
public:
    void enableAutoHide(bool p_state);
    bool isAutoHideEnabled();
    qint64 duration(const Event *p_event) const;
protected:
    void updateCurrent(qint64);
    qint64 tick();
protected slots:
    void timeout();
private:
    Script *m_script;
    QTimer m_timer;
    double m_speedFactor;
    bool m_speedFactorEnabled;
    qint64 m_msseStartTime;
    qint64 m_pauseStart;
    qint64 m_pauseTotal;
    qint64 m_userDelay;
    qint64 m_autoHideDuration;
    QList<Event *> m_lastEvents;
    QTimer m_timerAutoHide;
    bool m_autoHideEnabled;
};

#endif // PLAYER_H
