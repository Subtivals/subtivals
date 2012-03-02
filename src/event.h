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
    qint64 duration(bool p_auto = false) const;
    const Style *style() const;
    const QString &text() const;
    const QString &prettyText() const;
    bool match(qint64 msecs) const;
    int marginL() const;
    int marginR() const;
    int marginV() const;
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
