#ifndef PLAYER_H
#define PLAYER_H

#include <QObject>
#include <QtCore/QList>
#include <QtCore/QTimer>

#include "subtitle.h"

class Script;

class Player : public QObject {
  Q_OBJECT
public:
  explicit Player(QObject *parent = nullptr);
  void setScript(Script *);
  quint64 elapsedTime();
  int delay();
signals:
  void pulse(quint64 elapsed);
  void stopped();
  void on(Subtitle *);
  void off(Subtitle *);
  void changed(QList<Subtitle *>);
  void autoHide();
public slots:
  void play();
  void pause();
  void stop();
  void jumpTo(int);
  void enableSpeedFactor(bool p_state);
  void setElapsedTime(quint64);
  void setDelayStep(int);
  int delayStep() const { return m_delayStep; }
  void addDelay(int step = 0);
  void subDelay();
  void setSpeedFactor(double);
  void autoHideTimeout();

public:
  void enableAutoHide(bool p_state);
  bool isAutoHideEnabled();
  quint64 duration(const Subtitle *p_subtitle) const;

protected:
  void updateCurrent(quint64);
  quint64 tick();
  const QList<Subtitle *> current() const;
protected slots:
  void timeout();

private:
  Script *m_script;
  QTimer m_timer;
  double m_speedFactor;
  bool m_speedFactorEnabled;
  quint64 m_msseStartTime;
  quint64 m_pauseStart;
  quint64 m_pauseTotal;
  int m_userDelay;
  int m_delayStep;
  quint64 m_autoHideDuration;
  QList<Subtitle *> m_lastSubtitles;
  QTimer m_timerAutoHide;
  bool m_autoHideEnabled;
};

#endif // PLAYER_H
