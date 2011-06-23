#include "event.h"

#include "script.h"

#include <QDebug>
#include <QStringList>

Event::Event(const QString &p_line, const Script *p_script, QObject *p_parent) :
    QObject(p_parent)
{
    QList<QString> subparts = p_line.split(',');
    m_msseStart = QTime().msecsTo(QTime::fromString(subparts[1], "h:mm:ss.z"));
    m_msseEnd = QTime().msecsTo(QTime::fromString(subparts[2], "h:mm:ss.z"));
    m_style = p_script->style(subparts[3].trimmed());
    int p = p_line.indexOf(",");
    for (int i = 0; i < 8; i++)
    {
        p = p_line.indexOf(",", p+1);
    }
    m_text = p_line.mid(p+1).replace("\\N", QString('\n'));
    int idxAccOpen = m_text.indexOf("{\\pos(");
    if (idxAccOpen != -1)
    {
        int idxAccClose = m_text.indexOf(")}", idxAccOpen);
        if (idxAccClose != -1)
        {
            m_text = m_text.left(idxAccOpen) + m_text.mid(idxAccClose+2);
        }
    }
    m_text = m_text.trimmed();
    while(m_text.startsWith("\n"))
    {
        m_text = m_text.mid(1);
    }
}

qint64 Event::msseStart() const
{
    return m_msseStart;
}

qint64 Event::msseEnd() const
{
    return m_msseEnd;
}

const Style *Event::style() const
{
    return m_style;
}

const QString &Event::text() const
{
    return m_text;
}

bool Event::match(qint64 p_msecs) const
{
    return m_msseStart <= p_msecs && m_msseEnd >= p_msecs;
}
