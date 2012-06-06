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

#include "subtitle.h"
#include "script.h"


#define  AUTO_CHARS_RATE      12    // Chars/sec
#define  AUTO_EVENT_INTERVAL  1000  // msec
#define  AUTO_MIN_DURATION    1000  // msec


Subtitle::Subtitle(int p_index, const QString &p_text, qint64 p_msseStart, qint64 p_msseEnd, const Script *p_script, QObject *p_parent) :
    QObject(p_parent),
    m_index(p_index),
    m_script(p_script),
    m_msseStart(p_msseStart),
    m_msseEnd(p_msseEnd),
    m_style(0),
    m_marginL(0),
    m_marginR(0),
    m_marginV(0),
    m_corrected(false)
{
    setText(p_text);

    // Auto duration
    m_autoDuration = 1000 * text().size() / AUTO_CHARS_RATE;
    if (m_autoDuration < AUTO_MIN_DURATION) m_autoDuration = AUTO_MIN_DURATION;

    // Check if no timecode is specified
    if (m_msseStart == 0 || m_msseEnd == 0) {
        qint64 endPrevious = 0;
        if (m_index > 0)
            endPrevious = m_script->subtitleAt(m_index-1)->msseEnd() + AUTO_EVENT_INTERVAL;
        m_msseStart = endPrevious;
        m_msseEnd = m_msseStart + m_autoDuration;
    }
}

qint64 Subtitle::msseStart() const
{
    return m_msseStart;
}

qint64 Subtitle::msseEnd() const
{
    if (m_corrected) {
        return m_msseStart + m_autoDuration;
    }
    return m_msseEnd;
}

qint64 Subtitle::duration() const
{
    return msseEnd() - msseStart();
}

qint64 Subtitle::autoDuration() const
{
    return m_autoDuration;
}

bool Subtitle::isCorrected() const
{
    return m_corrected;
}

void Subtitle::correct(bool p_state)
{
    m_corrected = p_state && (m_autoDuration > (m_msseEnd - m_msseStart));
}

void Subtitle::setStyle(Style *p_style)
{
    m_style = p_style;
}

const Style *Subtitle::style() const
{
    return m_style;
}

void Subtitle::setText(const QString& p_text)
{
    m_text = p_text;

    m_prettyText = QString(p_text);
    m_prettyText = m_prettyText.replace(QRegExp("<br/>"), " # ");
    m_prettyText = m_prettyText.replace(QRegExp("</?[^bi]+/?>"), "");
}

const QString &Subtitle::text() const
{
    return m_text;
}

const QString &Subtitle::prettyText() const
{
    return m_prettyText;
}

bool Subtitle::match(qint64 p_msecs) const
{
    return msseStart() <= p_msecs && msseEnd() >= p_msecs;
}

void Subtitle::setMargins(int p_marginL, int p_marginR, int p_marginV)
{
    m_marginL = p_marginL;
    m_marginR = p_marginR;
    m_marginV = p_marginV;
}

int Subtitle::marginL() const
{
    return m_marginL;
}

int Subtitle::marginR() const
{
    return m_marginR;
}

int Subtitle::marginV() const
{
    return m_marginV;
}
