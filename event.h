#ifndef EVENT_H
#define EVENT_H

#include <QObject>
#include <QTime>
#include <QString>

class Style;
class Script;

class Event : public QObject
{
    Q_OBJECT
public:
    explicit Event(const QString &p_line, const Script *p_script, QObject *p_parent = 0);
    qint64 msseStart() const;
    qint64 msseEnd() const;
    const Style *style() const;
    const QString &text() const;
    bool match(qint64 msecs) const;
signals:

public slots:
private:
    qint64 m_msseStart;
    qint64 m_msseEnd;
    const Style *m_style;
    QString m_text;
};

#endif // EVENT_H
