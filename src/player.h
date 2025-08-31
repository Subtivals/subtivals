#ifndef PLAYER_H
#define PLAYER_H

#include <QObject>
#include <QtCore/QList>
#include <QtCore/QTimer>

#include "subtitle.h"

#define DELAY_OFFSET 250

class Script;

class Player : public QObject {
  Q_OBJECT
public:
  explicit Player(QObject *parent = nullptr);
  void setScript(Script *);
  qlonglong elapsedTime();
  int delay();
signals:
  void started(const QString &);
  void pulse(int);
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
  void setElapsedTime(qint64);
  void addDelay(int d = DELAY_OFFSET);
  void subDelay(int d = DELAY_OFFSET);
  void setSpeedFactor(double);
  void autoHideTimeout();

public:
  void enableAutoHide(bool p_state);
  bool isAutoHideEnabled();
  qint64 duration(const Subtitle *p_subtitle) const;

protected:
  void updateCurrent(qint64);
  qint64 tick();
  const QList<Subtitle *> current() const;
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
  int m_userDelay;
  qint64 m_autoHideDuration;
  QList<Subtitle *> m_lastSubtitles;
  QTimer m_timerAutoHide;
  bool m_autoHideEnabled;
};

#endif // PLAYER_H
