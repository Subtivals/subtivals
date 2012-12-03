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
#ifndef STYLE_H
#define STYLE_H

#include <QObject>
#include <QString>
#include <QFont>
#include <QColor>
#include <QRect>
#include <QTextDocument>

class Subtitle;

/*
 * Style to use to show an subtitle.
 */
class Style : public QObject
{
    Q_OBJECT
public:
    explicit Style(const QString &p_name, const QFont &p_font, const QColor &p_color, QObject *p_parent);
    explicit Style(const Style &p_oth, const QFont& f, QObject *p_parent = 0);
    const QString &name() const;
    const QFont &font() const;
    void setFont(const QFont &f);
    const QColor &primaryColour() const;
    void setPrimaryColour(const QColor &c);
    void setMargins(int p_marginL, int p_marginR, int p_marginV);
    void setAlignment(Qt::Alignment p_alignment);
    int marginL() const;
    int marginR() const;
    int marginV() const;
    Qt::Alignment alignment() const;

    void drawSubtitle(QPainter*, const Subtitle&, const QRect&, const double, const QPen&) const;
    int textHeight(const Subtitle &subtitle) const;
private:
    QString m_name;
    QFont m_font;
    QColor m_primaryColour;
    /*
     * Alignment : after the layout of the numpad (1-3 sub, 4-6 mid, 7-9 top)
     */
    Qt::Alignment m_alignment;
    /*
     * MarginL, MarginR : left right margins
     */
    int m_marginL;
    int m_marginR;
    /*
     * MarginV : subtitle : margin from bottom; toptitle : margin from top, midtitle : ignored
     */
    int m_marginV;
};

#endif // STYLE_H
