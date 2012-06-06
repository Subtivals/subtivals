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

class Subtitle : public QObject
{
    Q_OBJECT
public:
    explicit Subtitle(int p_index, const QString &p_text, qint64 p_msseStart, qint64 p_msseEnd, const Script *p_script, QObject *p_parent = 0);
    qint64 msseStart() const;
    qint64 msseEnd() const;
    qint64 duration() const;
    qint64 autoDuration() const;
    void setStyle(Style *p_style);
    const Style *style() const;
    void setText(const QString &p_text);
    const QString &text() const;
    const QString &prettyText() const;
    bool match(qint64 msecs) const;
    void setMargins(int p_marginL, int p_marginR, int p_marginV);
    int marginL() const;
    int marginR() const;
    int marginV() const;
    bool isCorrected() const;
    void correct(bool);
private:
    int m_index;
    const Script *m_script;
    qint64 m_msseStart;
    qint64 m_msseEnd;
    qint64 m_autoDuration;
    const Style *m_style;
    QString m_text;
    QString m_prettyText;
    int m_marginL;
    int m_marginR;
    int m_marginV;
    bool m_corrected;
};

#endif // EVENT_H
