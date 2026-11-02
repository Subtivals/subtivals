#ifndef RECORDER_H
#define RECORDER_H

#include <QElapsedTimer>
#include <QObject>
#include <QTemporaryFile>

#include "subtitle.h"

class Recorder : public QObject {
  Q_OBJECT
public:
  explicit Recorder(QObject *parent = nullptr);
  QString start();
  QString finish();
  qint64 elapsed() const;

public slots:
  void recordSubtitles(QList<Subtitle *> subtitles);

private:
  void writeCurrent(qint64 end);

  QTemporaryFile m_file;
  QElapsedTimer m_clock;
  QString m_currentText;
  qint64 m_currentStart = 0;
  int m_index = 1;
  bool m_recording = false;
};

#endif // RECORDER_H
