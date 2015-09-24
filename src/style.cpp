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
#include <QtGui/QTextFrame>

#include "style.h"
#include "script.h"
#include "subtitle.h"

Style::Style(const QString &p_name, const QFont &p_font, const QColor &p_color, QObject *p_parent) :
    QObject(p_parent),
    m_name(p_name),
    m_metrics(p_font),
    m_primaryColour(p_color),
    m_lineSpacing(0.0),
    m_alignment(Qt::AlignTop | Qt::AlignHCenter),
    m_marginL(0),
    m_marginR(0),
    m_marginV(0),
    m_offsetH(0),
    m_offsetV(0)
{
    setFont(p_font);
}

Style::Style(const Style &p_oth, const QFont& f, QObject *p_parent) :
    QObject(p_parent),
    m_name(p_oth.m_name),
    m_metrics(QFontMetrics(p_oth.font())),
    m_primaryColour(p_oth.m_primaryColour),
    m_lineSpacing(p_oth.m_lineSpacing),
    m_alignment(p_oth.m_alignment),
    m_marginL(p_oth.m_marginL),
    m_marginR(p_oth.m_marginR),
    m_marginV(p_oth.m_marginV),
    m_offsetH(p_oth.m_offsetH),
    m_offsetV(p_oth.m_offsetV)
{
    setFont(f);
}

void Style::setAlignment(Qt::Alignment p_alignment)
{
    m_alignment = p_alignment;
}

void Style::setOffsets(double p_h, double p_v)
{
    m_offsetH = p_h;
    m_offsetV = p_v;
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

void Style::setName(const QString &p_name)
{
    m_name = p_name;
}

const QFont &Style::font() const {
    return m_font;
}

void Style::setFont(const QFont& f) {
    m_font = f;
    m_font.setStyleStrategy(QFont::PreferAntialias);
    m_metrics = QFontMetrics(f);
}

const QColor &Style::primaryColour() const {
    return m_primaryColour;
}

void Style::setPrimaryColour(const QColor &c)
{
    m_primaryColour = c;
}

qreal Style::lineSpacing() const
{
    return m_lineSpacing;
}

void Style::setLineSpacing(qreal p_lineSpacing)
{
    m_lineSpacing = p_lineSpacing;
}

int Style::subtitleHeight(const Subtitle &subtitle) const
{
    int h = m_metrics.height();
    int nb = subtitle.nbLines();
    return (h * nb) + (m_lineSpacing * h * (nb-1));
}

const QPoint Style::textAnchor(const QPoint &p_point, const QString &p_text) const
{
    QPoint offset(0, 0);
    // Make sure text contains no HTML
    QString strip(p_text);
    strip.remove(QRegExp("<[^>]*>"));
    // alignment becomes text anchor
    if (m_alignment & Qt::AlignVCenter) {
        offset.setY(m_metrics.height() / 2);
    } else if (m_alignment & Qt::AlignBottom) {
        offset.setY(m_metrics.height());
    }
    if (m_alignment & Qt::AlignHCenter) {
        offset.setX(-m_metrics.width(strip) / 2);
    } else if (m_alignment & Qt::AlignRight) {
        offset.setX(-m_metrics.width(strip));
    }
    return p_point + offset;
}

void Style::drawSubtitle(QPainter *painter, const Subtitle &subtitle, const QRect &bounds, const QPen &outline) const
{
    QSize resolution = subtitle.script()->resolution();
    QPointF scale = QPointF(1.0, 1.0);
    if (resolution.width() > 0)  scale.setX(double(bounds.width()) / resolution.width());
    if (resolution.height() > 0) scale.setY(double(bounds.height()) / resolution.height());

    int stack = 0;

    foreach (SubtitleLine line, subtitle.lines()) {
        QRect final(bounds);

        QPoint position = line.position();

        QString html = "<p align=\"HORIZONTAL\">TEXT</p>";
        if (position.x() >= 0 && position.y() >= 0) {
            // absolute positioning : (x, y)
            position.setX(position.x() * scale.x());
            position.setY(position.y() * scale.y());
            final.setTopLeft(textAnchor(position, line.text()));
        }
        else {
            int marginV = (m_marginV + subtitle.marginV()) * scale.y();
            int marginL = 0;
            int marginR = 0;

            if (position.x() < 0) { // If position is not set
                // Horizontal margins
                marginL = (m_marginL + subtitle.marginL()) * scale.x();
                marginR = (m_marginR + subtitle.marginR()) * scale.x();
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
                position.setX(position.x() * scale.x());
                html = html.replace("align=\"HORIZONTAL\"", "");
                final.moveLeft(textAnchor(position, line.text()).x());
            }

            // Apply margins
            final = final.adjusted(marginL, marginV, -marginR, -marginV);

            // Vertical positioning
            if (m_alignment & Qt::AlignBottom) {
                final.moveTop(final.bottom() - subtitleHeight(subtitle) + stack);
            } else if (m_alignment & Qt::AlignVCenter) {
                final.moveTop(final.center().y() - subtitleHeight(subtitle)/2 + stack);
            }
            else {  // AlignTop
                final.moveTop(final.top() + stack);
            }

            // Offsets.
            final.moveBottom(final.bottom() - (m_offsetV * bounds.height()));
            final.moveLeft(final.left() + (m_offsetH * bounds.width()));

            // Stack lines
            stack += m_metrics.height() * (1.0 + m_lineSpacing);
        }

        html = html.replace("TEXT", line.text());
        QTextDocument doc;
        doc.setPageSize(QSize(final.width(), final.height()));
        doc.setHtml(html);
        doc.setDefaultFont(m_font);

        // Reduce document margins to 0
        QTextFrame *tf = doc.rootFrame();
        QTextFrameFormat tff = tf->frameFormat();
        tff.setTopMargin(0);
        tf->setFrameFormat(tff);

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

