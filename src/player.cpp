#include "player.h"

#include "script.h"
#include "subtitle.h"


Player::Player(QObject *parent) :
    QObject(parent),
    m_script(NULL),
    m_speedFactor(1.0),
    m_speedFactorEnabled(false),
    m_msseStartTime(0),
    m_pauseStart(0),
    m_pauseTotal(0),
    m_userDelay(0),
    m_autoHideEnabled(false)
{
    m_timer.setInterval(100);
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(timeout()));
    // Timer for auto-hiding ended subtitles
    m_timerAutoHide.setInterval(100);
    connect(&m_timerAutoHide, SIGNAL(timeout()), this, SLOT(autoHideTimeout()));
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
    m_timerAutoHide.stop();
    m_timer.start();
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
    m_timerAutoHide.stop();
    m_msseStartTime = 0;
    m_pauseTotal = 0;
    m_userDelay = 0;
    m_lastSubtitles.clear();
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

void Player::autoHideTimeout()
{
    if (m_autoHideDuration > 0) {
        m_autoHideDuration -= m_timerAutoHide.interval();
        emit pulse(tick() - m_pauseStart + m_lastSubtitles.at(0)->msseStart());
    }
    else {
        emit autoHide();
        m_timerAutoHide.stop();
    }
}

qint64 Player::duration(const Subtitle* p_subtitle) const
{
    if (m_timer.isActive())
        return p_subtitle->duration();
    return qMax(p_subtitle->duration(), p_subtitle->autoDuration());
}

void Player::jumpTo(int i)
{
    // Reset user delay
    m_userDelay = 0;
    bool playing = m_timer.isActive();
    m_timer.stop();
    // Get subtitle in script
    const Subtitle *subtitle = m_script->subtitleAt(i);
    qint64 start_mss = subtitle->msseStart();
    setElapsedTime(start_mss);
    // Show it !
    updateCurrent(start_mss + 1);
    // Continuous play, even while pause
    if (playing) {
        m_timer.start();
    }
    else {
        m_pauseTotal = 0;
        if (m_pauseStart > 0) m_pauseStart = tick();

        if (m_autoHideEnabled) {
            m_pauseStart = tick();
            m_autoHideDuration = duration(subtitle);
            m_timerAutoHide.start();
        }
    }
}

QList<Subtitle*> Player::current()
{
    return m_lastSubtitles;
}

void Player::updateCurrent(qint64 msecsElapsed)
{
    // Sanity check
    if(m_script == 0)
        return;
    // Find subtitles that match elapsed time
    QList<Subtitle *> currentSubtitles = m_script->currentSubtitles(msecsElapsed);

    // Compare subtitles that match elapsed time with subtitles that matched elapsed time last
    // time the timer was fired to find the differences
    bool change = false;
    foreach(Subtitle *e, m_lastSubtitles) {
        // Subtitles that where presents and that are no more presents : suppress
        if(!currentSubtitles.contains(e)) {
            emit off(e);
            change = true;
        }
    }
    foreach(Subtitle *e, currentSubtitles) {
        // Subtitles that are presents and that were not presents : add
        if(!m_lastSubtitles.contains(e)) {
            emit on(e);
            change = true;
        }
    }
    m_lastSubtitles = currentSubtitles;
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
    if (!m_timer.isActive() && m_lastSubtitles.count() > 0) {
        return m_lastSubtitles.last()->msseStart() + 1;
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

void Player::enableAutoHide(bool p_state)
{
    m_autoHideEnabled = p_state;
    m_timerAutoHide.stop();
}

bool Player::isAutoHideEnabled()
{
    return m_autoHideEnabled;
}

void Player::enableSpeedFactor(bool p_state)
{
    qint64 elapsed = elapsedTime();
    m_speedFactorEnabled = p_state;
    setElapsedTime(elapsed);
}
