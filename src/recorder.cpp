#include <QDir>
#include <QTime>
#include <QTextStream>

#include "recorder.h"

Recorder::Recorder(QObject *parent) : QObject(parent) {}

qint64 Recorder::elapsed() const { return m_clock.elapsed(); }

QString Recorder::start() {
  m_file.setFileTemplate(QDir::tempPath() + "/subtivals-record-XXXXXX.srt");
  if (!m_file.open())
    return m_file.errorString();
  m_index = 1;
  m_currentText.clear();
  m_clock.start();
  m_recording = true;
  return {};
}

QString Recorder::finish() {
  if (!m_recording)
    return {};
  if (!m_currentText.isEmpty())
    writeCurrent(m_clock.elapsed());
  m_recording = false;
  m_file.flush();
  const QString fileName = m_file.fileName();
  m_file.setAutoRemove(false);
  m_file.close();
  return fileName;
}

void Recorder::recordSubtitles(QList<Subtitle *> subtitles) {
  if (!m_recording)
    return;
  const qint64 now = m_clock.elapsed();
  // The previous caption's end time is only known when the display changes.
  if (!m_currentText.isEmpty())
    writeCurrent(now);
  if (subtitles.isEmpty())
    return;
  QStringList lines;
  for (const Subtitle *subtitle : std::as_const(subtitles))
    lines << subtitle->prettyText();
  // Keep the active caption until its end time is known, or recording stops.
  m_currentText = lines.join("\n");
  m_currentStart = now;
}

void Recorder::writeCurrent(qint64 end) {
  end = qMax(m_currentStart + 1, end);
  QTextStream out(&m_file);
  out << m_index++ << "\n"
      << QTime(0, 0).addMSecs(int(m_currentStart)).toString("hh:mm:ss,zzz")
      << " --> "
      << QTime(0, 0).addMSecs(int(end)).toString("hh:mm:ss,zzz")
      << "\n" << m_currentText << "\n\n";
  m_currentText.clear();
}
