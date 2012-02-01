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
    explicit Event(const QString &p_line, const Script *p_script, int p_index, QObject *p_parent = 0);
    qint64 msseStart() const;
    qint64 msseEnd() const;
    const Style *style() const;
    const QString &text() const;
    const QString &prettyText() const;
    bool match(qint64 msecs) const;
    int marginL() const;
    int marginR() const;
    int marginV() const;
signals:

public slots:
private:
    qint64 m_msseStart;
    qint64 m_msseEnd;
    const Style *m_style;
    QString m_text;
    QString m_prettyText;
    int m_marginL;
    int m_marginR;
    int m_marginV;
};

#endif // EVENT_H
