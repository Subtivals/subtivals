#include "player.h"

#include "script.h"
#include "event.h"


Player::Player(QObject *parent) :
    QObject(parent),
    m_script(NULL),
    m_speedFactor(1.0),
    m_speedFactorEnabled(false),
    m_msseStartTime(0),
    m_pauseStart(0),
    m_pauseTotal(0),
    m_userDelay(0)
{
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(timeout()));
}

void Player::setScript(Script* p_script)
{
    m_script = p_script;
    if (!p_script) {
        stop();
    }
}

void Player::play()
{
    m_userDelay = 0;
    // From stop
    if (m_msseStartTime == 0) m_msseStartTime = tick();
    // From pause
    if (m_pauseStart > 0) {
        m_pauseTotal += tick() - m_pauseStart;
        m_pauseStart = 0;
    }
    m_timer.start(100);
}

void Player::pause()
{
    m_timer.stop();
    m_pauseStart = tick();
}

void Player::stop()
{
    emit clear();
    m_timer.stop();
    m_msseStartTime = 0;
    m_pauseTotal = 0;
    m_userDelay = 0;
    m_lastEvents.clear();
}

int Player::delay()
{
    return m_userDelay;
}

void Player::addDelay(int d)
{
    m_userDelay += d;
}

void Player::subDelay(int d)
{
    m_userDelay -= d;
}

void Player::timeout()
{
    qint64 d = elapsedTime();
    emit pulse(d);
    updateCurrent(d);
}

void Player::jumpTo(int i)
{
    // Reset user delay
    m_userDelay = 0;
    bool playing = m_timer.isActive();
    m_timer.stop();
    // Get event in script
    const Event *event = m_script->eventAt(i);
    qint64 start_mss = event->msseStart();
    setElapsedTime(start_mss);
    // Show it !
    updateCurrent(start_mss + 1);
    // Continuous play, even while pause
    if (playing) {
        m_timer.start(100);
    }
    else {
        m_pauseTotal = 0;
        if (m_pauseStart > 0) m_pauseStart = tick();
    }
}

QList<Event*> Player::current()
{
    return m_lastEvents;
}

void Player::updateCurrent(qint64 msecsElapsed)
{
    // Sanity check
    if(m_script == 0)
        return;
    // Find events that match elapsed time
    QList<Event *> currentEvents = m_script->currentEvents(msecsElapsed);

    // Compare events that match elapsed time with events that matched elapsed time last
    // time the timer was fired to find the differences
    bool change = false;
    foreach(Event *e, m_lastEvents) {
        // Events that where presents and that are no more presents : suppress
        if(!currentEvents.contains(e)) {
            emit off(e);
            change = true;
        }
    }
    foreach(Event *e, currentEvents) {
        // Events that are presents and that were not presents : add
        if(!m_lastEvents.contains(e)) {
            emit on(e);
            change = true;
        }
    }
    m_lastEvents = currentEvents;
    if (change)
        emit changed();
}

qint64 Player::tick()
{
    QDateTime dateTime = QDateTime::currentDateTime();
    qint64 dt=QDate(1982, 5, 8).daysTo(dateTime.date());
    QTime tt=dateTime.time();
    return 86400000 * dt + 3600000 * tt.hour() + 60000 * tt.minute() + 1000 * tt.second() + tt.msec();
}

void Player::setElapsedTime(qint64 p_elapsed)
{
    double factor = m_speedFactorEnabled ? m_speedFactor : 1.0;
    m_msseStartTime = tick() - p_elapsed/factor + m_userDelay - m_pauseTotal;
}

qint64 Player::elapsedTime()
{
    if (!m_timer.isActive() && m_lastEvents.count() > 0) {
        return m_lastEvents.last()->msseStart() + 1;
    }
    // Gets the elapsed time in milliseconds
    double factor = m_speedFactorEnabled ? m_speedFactor : 1.0;
    return (tick() - m_msseStartTime + m_userDelay - m_pauseTotal) * factor;
}

void Player::setSpeedFactor(double p_factor)
{
    qint64 elapsed = elapsedTime();
    m_speedFactor = p_factor/100.0;
    setElapsedTime(elapsed);
}

void Player::enableSpeedFactor(bool p_state)
{
    qint64 elapsed = elapsedTime();
    m_speedFactorEnabled = p_state;
    setElapsedTime(elapsed);
}
