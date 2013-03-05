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
#include <QtGui/QPainter>
#include <QtGui/QTextDocument>
#include <QtGui/QAbstractTextDocumentLayout>

#include "style.h"
#include "subtitle.h"

Style::Style(const QString &p_name, const QFont &p_font, const QColor &p_color, QObject *p_parent) :
    QObject(p_parent),
    m_name(p_name),
    m_primaryColour(p_color),
    m_alignment(Qt::AlignTop | Qt::AlignHCenter),
    m_marginL(0),
    m_marginR(0),
    m_marginV(0)
{
    setFont(p_font);
}

Style::Style(const Style &p_oth, const QFont& f, QObject *p_parent):
    QObject(p_parent),
    m_name(p_oth.m_name),
    m_primaryColour(p_oth.m_primaryColour),
    m_alignment(p_oth.m_alignment),
    m_marginL(p_oth.m_marginL),
    m_marginR(p_oth.m_marginR),
    m_marginV(p_oth.m_marginV)
{
    setFont(f);
}

void Style::setAlignment(Qt::Alignment p_alignment)
{
    m_alignment = p_alignment;
}

int Style::marginL() const
{
    return m_marginL;
}

int Style::marginR() const
{
    return m_marginR;
}

int Style::marginV() const
{
    return m_marginV;
}

Qt::Alignment Style::alignment() const
{
    return m_alignment;
}

void Style::setMargins(int p_marginL, int p_marginR, int p_marginV)
{
    m_marginL = p_marginL;
    m_marginR = p_marginR;
    m_marginV = p_marginV;
}

const QString &Style::name() const {
    return m_name;
}

const QFont &Style::font() const {
    return m_font;
}

void Style::setFont(const QFont& f) {
    m_font = f;
    m_font.setStyleStrategy(QFont::PreferAntialias);
}

const QColor &Style::primaryColour() const {
    return m_primaryColour;
}

void Style::setPrimaryColour(const QColor &c)
{
    m_primaryColour = c;
}

int Style::subtitleHeight(const Subtitle &subtitle) const
{
    QFontMetrics metrics(font());
    int h = font().pixelSize();
    int nb = subtitle.nbLines();
    int lineSpace = 0.75 * h * (nb-1) + metrics.descent() + 5;
    return h * nb + lineSpace;
}

void Style::drawSubtitle(QPainter *painter, const Subtitle &subtitle, const QRect &bounds, double zoom, const QPen &outline) const
{
    int stack = 0;

    foreach (SubtitleLine line, subtitle.lines()) {
        QRect final(bounds);
        QPoint position = line.position();

        QString html = "<p align=\"HORIZONTAL\">TEXT</p>";
        if (position.x() >= 0 && position.y() >= 0) {
            // absolute positioning : (x, y)
            final.setTopLeft(position);
            html = html.replace("HORIZONTAL", "left");
        }
        else {
            int marginV = (m_marginV + subtitle.marginV()) * zoom;
            int marginL = 0;
            int marginR = 0;
            int height = 0;

            if (position.x() < 0) {
                // If position is not set
                height = font().pixelSize();
                // Horizontal margins
                marginL = (m_marginL + subtitle.marginL()) * zoom;
                marginR = (m_marginR + subtitle.marginR()) * zoom;
                // Same for alignment
                if (m_alignment & Qt::AlignLeft) {
                    html = html.replace("HORIZONTAL", "left");
                } else if (m_alignment & Qt::AlignRight) {
                    html = html.replace("HORIZONTAL", "right");
                } else {
                    html = html.replace("HORIZONTAL", "center");
                }
            }
            else {
                // Horizontal positioning : (x, ?)
                final.moveLeft(position.x());
            }

            // Apply margins
            final = final.adjusted(marginL, marginV, -marginR, -marginV);

            // Vertical positioning
            if (m_alignment & Qt::AlignBottom) {
                final.moveTop(final.bottom() - subtitleHeight(subtitle) + stack);
            } else if (m_alignment & Qt::AlignVCenter) {
                final.moveTop(final.center().y() - font().pixelSize()/2 + stack);
            }
            else {  // AlignTop
                final.moveTop(final.top() + stack);
            }
            // Stack lines
            stack += (1.75 * height);
        }

        html = html.replace("TEXT", line.text());
        QTextDocument doc;
        doc.setPageSize(QSize(final.width(), final.height()));
        doc.setHtml(html);
        doc.setDefaultFont(m_font);
        QAbstractTextDocumentLayout* layout = doc.documentLayout();

        painter->setFont(m_font);
        painter->setPen(m_primaryColour);
        QAbstractTextDocumentLayout::PaintContext context;
        context.palette.setColor(QPalette::Text, painter->pen().color());
        painter->save();
        painter->translate(final.x(), final.y());

        if (outline.width() > 0) {
            // Paint outline
            QTextCursor cursor(&doc);
            cursor.select(QTextCursor::Document);
            QTextCharFormat format;
            format.setTextOutline(outline);
            cursor.mergeCharFormat(format);
            layout->draw(painter, context);

            format.setTextOutline(Qt::NoPen);
            cursor.mergeCharFormat(format);
        }
        // Repaint above, without outline
        layout->draw(painter, context);
        painter->restore();
    }
}

