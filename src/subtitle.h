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
#include <QString>
#include <QPoint>
#include <QPair>

class Style;
class Script;

class SubtitleLine : public QPair<QString, QPoint>
{
public:
    explicit SubtitleLine(const QString& t, const QPoint& p) { first = t; second = p; }
    const QString text() const    { return first; }
    const QPoint position() const { return second; }
};


class Subtitle : public QObject
{
    Q_OBJECT
public:
    explicit Subtitle(int p_index, const QStringList &p_text, qint64 p_msseStart, qint64 p_msseEnd, const Script *p_script, QObject *p_parent = 0);
    qint64 msseStart() const;
    qint64 msseEnd() const;
    qint64 duration() const;
    qint64 autoDuration() const;
    int charsRate() const;
    void setStyle(Style *p_style);
    const Style *style() const;
    const Script *script() const;
    void setText(const QStringList &p_text);
    void setText(const QList<SubtitleLine> p_lines);
    void setComments(const QString &p_comments);
    const int &index() const;
    const QString &text() const;
    const QString &prettyText() const;
    const QString &comments() const;
    bool match(qint64 msecs) const;
    void setMargins(int p_marginL, int p_marginR, int p_marginV);
    int marginL() const;
    int marginR() const;
    int marginV() const;
    bool isCorrected() const;
    void correct(bool);
    int nbLines() const;
    void setPosition(int p_x, int p_y);
    const QList<SubtitleLine> lines() const;
private:
    int m_index;
    const Script *m_script;
    qint64 m_msseStart;
    qint64 m_msseEnd;
    qint64 m_autoDuration;
    const Style *m_style;
    QString m_text;
    QString m_prettyText;
    QString m_pureText;
    QString m_comments;
    int m_marginL;
    int m_marginR;
    int m_marginV;
    QList<SubtitleLine> m_lines;
    bool m_corrected;
};

#endif // EVENT_H
