/**
  *  This file is part of Subtivals.
  *
  *  Subtivals is free software: you can redistribute it and/or modify
  *  it under the terms of the GNU General Public License as published by
  *  the Free Software Foundation, either version 3 of the License, or
  *  (at your option) any later version.
  *
  *  Subtivals is distributed in the hope that it will be useful,
  *  but WITHOUT ANY WARRANTY; without even the implied warranty of
  *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  *  GNU General Public License for more details.
  *
  *  You should have received a copy of the GNU General Public License
  *  along with Subtivals.  If not, see <http://www.gnu.org/licenses/>
  **/
#include <QtCore/QStringList>

#include "event.h"
#include "script.h"


#define  AUTO_CHARS_RATE      12    // Chars/sec
#define  AUTO_EVENT_INTERVAL  1000  // msec
#define  AUTO_MIN_DURATION    1000  // msec


Event::Event(const QString &p_line, const Script *p_script, int p_index, QObject *p_parent) :
    QObject(p_parent)
{
    QList<QString> subparts = p_line.split(',');
    m_msseStart = QTime().msecsTo(QTime::fromString(subparts[1], "h:mm:ss.z"));
    m_msseEnd = QTime().msecsTo(QTime::fromString(subparts[2], "h:mm:ss.z"));
    m_style = p_script->style(subparts[3].trimmed());

    // Check if style is overridden in event
    m_marginL = subparts[5].toInt();
    m_marginR = subparts[6].toInt();
    m_marginV = subparts[7].toInt();

    int p = p_line.indexOf(",");
    for (int i = 0; i < 8; i++) {
        p = p_line.indexOf(",", p+1);
    }
    m_prettyText = p_line.mid(p+1);
    m_text = m_prettyText;

    m_prettyText = m_prettyText.replace("\\N", QString::fromUtf8(" ↲ "));
    m_prettyText = m_prettyText.replace("\\n", QString::fromUtf8(" ↲ "));
    m_prettyText = m_prettyText.replace("{\\i1}", "**");
    m_prettyText = m_prettyText.replace("{\\i0}", "**");
    // Drop others hints that cannot be translated in HTML
    {
        int idxAccOpenDrop = m_prettyText.indexOf("{\\");
        while (idxAccOpenDrop != -1)
        {
            int idxAccCloseDrop = m_prettyText.indexOf("}", idxAccOpenDrop);
            if (idxAccCloseDrop != -1)
            {
                m_prettyText = m_prettyText.left(idxAccOpenDrop) + m_prettyText.mid(idxAccCloseDrop+1);
            }
            idxAccOpenDrop = m_prettyText.indexOf("{\\");
        }
    }

    // Transform the hints in the text into HTML:
    // New ligne HTML-ification
    m_text = m_text.replace("\\N", "<br/>");
    m_text = m_text.replace("\\n", "<br/>");
    // Italic HTML-ification
    m_text = m_text.replace("{\\i1}", "<i>");
    m_text = m_text.replace("{\\i0}", "</i>");
    // Color HTML-ification
    {
        int idxAccOpenColor = m_text.indexOf("{\\1c&H");
        while (idxAccOpenColor != -1)
        {

            int idxAccCloseColor = m_text.indexOf("}", idxAccOpenColor);
            if (idxAccCloseColor != -1)
            {
                QString htmlColor = "<font color=\"#" + m_text.mid(idxAccOpenColor + 6 + 4, 2) + m_text.mid(idxAccOpenColor + 6 + 2, 2) + m_text.mid(idxAccOpenColor + 6, 2) + "\">";
                m_text = m_text.left(idxAccOpenColor) + htmlColor + m_text.mid(idxAccCloseColor+1) + "</font>";
            }
            idxAccOpenColor = m_text.indexOf("{\\1c&H");
        }
    }
    // Drop others hints that cannot be translated in HTML
    {
        int idxAccOpenDrop = m_text.indexOf("{\\");
        while (idxAccOpenDrop != -1)
        {
            int idxAccCloseDrop = m_text.indexOf("}", idxAccOpenDrop);
            if (idxAccCloseDrop != -1)
            {
                m_text = m_text.left(idxAccOpenDrop) + m_text.mid(idxAccCloseDrop+1);
            }
            idxAccOpenDrop = m_text.indexOf("{\\");
        }
    }
    m_text = m_text.trimmed();
    while(m_text.startsWith("\n"))
    {
        m_text = m_text.mid(1);
    }

    // Check if no timecode is specified
    if (m_msseStart == 0 && m_msseStart == m_msseEnd) {
        qint64 endPrevious = 0;
        if (p_index > 0)
            endPrevious = p_script->eventAt(p_index-1)->msseEnd() + AUTO_EVENT_INTERVAL;
        m_msseStart = endPrevious;
        m_msseEnd = m_msseStart + duration(true);
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

qint64 Event::duration(bool p_auto) const
{
    if (p_auto) {
        qint64 d = 1000 * text().size() / AUTO_CHARS_RATE;
        if (d < AUTO_MIN_DURATION) d = AUTO_MIN_DURATION;
        return d;
    }
    return m_msseEnd - m_msseStart;
}

const Style *Event::style() const
{
    return m_style;
}

const QString &Event::text() const
{
    return m_text;
}

const QString &Event::prettyText() const
{
    return m_prettyText;
}

bool Event::match(qint64 p_msecs) const
{
    return m_msseStart <= p_msecs && m_msseEnd >= p_msecs;
}

int Event::marginL() const
{
    return m_marginL;
}

int Event::marginR() const
{
    return m_marginR;
}

int Event::marginV() const
{
    return m_marginV;
}
