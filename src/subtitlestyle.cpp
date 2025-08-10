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
#include <QtGui/QAbstractTextDocumentLayout>
#include <QtGui/QPainter>
#include <QtGui/QTextDocument>
#include <QtGui/QTextFrame>
#include <QRegularExpression>
#include <QHash>

#include "script.h"
#include "subtitlestyle.h"
#include "subtitle.h"

SubtitleStyle::SubtitleStyle(const QString &p_name, const QFont &p_font,
                             const QColor &p_color, QObject *p_parent)
    : QObject(p_parent), m_name(p_name), m_primaryColour(p_color),
      m_lineSpacing(DEFAULT_LINESPACING),
      m_alignment(Qt::AlignTop | Qt::AlignHCenter), m_marginL(0), m_marginR(0),
      m_marginV(0), m_offsetH(0), m_offsetV(0) {
  setFont(p_font);
}

SubtitleStyle::SubtitleStyle(const SubtitleStyle &p_oth, const QFont &f,
                             QObject *p_parent)
    : QObject(p_parent), m_name(p_oth.m_name),
      m_primaryColour(p_oth.m_primaryColour),
      m_lineSpacing(p_oth.m_lineSpacing), m_alignment(p_oth.m_alignment),
      m_marginL(p_oth.m_marginL), m_marginR(p_oth.m_marginR),
      m_marginV(p_oth.m_marginV), m_offsetH(p_oth.m_offsetH),
      m_offsetV(p_oth.m_offsetV) {
  setFont(f);
}

void SubtitleStyle::setAlignment(Qt::Alignment p_alignment) {
  m_alignment = p_alignment;
  invalidateCache();
}

void SubtitleStyle::setOffsets(double p_h, double p_v) {
  m_offsetH = p_h;
  m_offsetV = p_v;
  invalidateCache();
}

int SubtitleStyle::marginL() const { return m_marginL; }

int SubtitleStyle::marginR() const { return m_marginR; }

int SubtitleStyle::marginV() const { return m_marginV; }

Qt::Alignment SubtitleStyle::alignment() const { return m_alignment; }

void SubtitleStyle::setMargins(int p_marginL, int p_marginR, int p_marginV) {
  m_marginL = p_marginL;
  m_marginR = p_marginR;
  m_marginV = p_marginV;
  invalidateCache();
}

const QString &SubtitleStyle::name() const { return m_name; }

void SubtitleStyle::setName(const QString &p_name) { m_name = p_name; }

const QFont SubtitleStyle::scaledFont(const qreal p_dpi,
                                      const qreal p_scale) const {
  QFont temp = m_font;
  if (m_font.pixelSize() > 0) {
    qreal pts = (temp.pixelSize() * 72.0) / p_dpi;
    temp.setPointSizeF(pts * p_scale);
  } else {
    qreal ptSize = temp.pointSizeF();
    if (ptSize <= 0)
      ptSize = 12.0; // fallback default font size
    temp.setPointSizeF(ptSize * p_scale);
  }
  return temp;
}

const QFont &SubtitleStyle::font() const { return m_font; }

void SubtitleStyle::setFont(const QFont &f) {
  m_font = f;
  m_font.setStyleStrategy(QFont::PreferAntialias);
  invalidateCache();
}

const QColor &SubtitleStyle::primaryColour() const { return m_primaryColour; }

void SubtitleStyle::setPrimaryColour(const QColor &c) {
  m_primaryColour = c;
  invalidateCache();
}

qreal SubtitleStyle::lineSpacing() const { return m_lineSpacing; }

void SubtitleStyle::setLineSpacing(qreal p_lineSpacing) {
  m_lineSpacing = p_lineSpacing;
  invalidateCache();
}

int SubtitleStyle::subtitleHeight(const Subtitle &subtitle,
                                  const QFontMetrics &metrics) const {
  int h = metrics.height();
  int nb = subtitle.nbLines();
  return (h * nb) + (m_lineSpacing * h * (nb - 1));
}

const QPoint SubtitleStyle::textAnchor(const QPoint &p_point,
                                       const QString &p_text,
                                       const QFontMetrics &metrics) const {
  QPoint offset(0, 0);
  // strip HTML
  QString strip(p_text);
  strip.remove(QRegularExpression("<[^>]*>"));

  // vertical position
  if (m_alignment & Qt::AlignVCenter) {
    offset.setY(metrics.height() / 2);
  } else if (m_alignment & Qt::AlignBottom) {
    offset.setY(metrics.height());
  }

  // horizontal: use horizontalAdvance for accurate width
  int textW = metrics.horizontalAdvance(strip);
  if (m_alignment & Qt::AlignHCenter) {
    offset.setX(-(textW / 2));
  } else if (m_alignment & Qt::AlignRight) {
    offset.setX(-textW);
  }
  return p_point + offset;
}

void SubtitleStyle::invalidateCache() const {
  qDeleteAll(m_docCache);
  m_docCache.clear();
}

void SubtitleStyle::drawSubtitle(QPainter *painter, const Subtitle &subtitle,
                                 const QRect &bounds, const QPen &outline,
                                 const qreal p_scale, const qreal p_dpi) const {
  QSize resolution = subtitle.script()->resolution();
  QPointF scale(p_scale, p_scale);
  if (resolution.width() > 0)
    scale.setX(double(bounds.width()) / resolution.width());
  if (resolution.height() > 0)
    scale.setY(double(bounds.height()) / resolution.height());

  // Build scaled font deterministically:
  qreal uniformScale = qMin(scale.x(), scale.y());
  QFont scaledFont = this->scaledFont(p_dpi, uniformScale);
  QFontMetrics fontMetrics(scaledFont);

  int stack = 0;

  for (const SubtitleLine &line : subtitle.lines()) {
    QRect final(bounds);

    QPoint position = line.position();

    QString horizontalAlign = "center";
    if (position.x() >= 0 && position.y() >= 0) {
      // absolute positioning : (x, y)
      position.setX(bounds.x() + qRound(position.x() * scale.x()));
      position.setY(bounds.y() + qRound(position.y() * scale.y()));
      final.setTopLeft(textAnchor(position, line.text(), fontMetrics));
    } else {
      // scale margins (round to ints)
      int marginV = qRound((m_marginV + subtitle.marginV()) * scale.y());
      int marginL = 0;
      int marginR = 0;

      if (position.x() < 0) { // If position is not set
        // Horizontal margins
        marginL = qRound((m_marginL + subtitle.marginL()) * scale.x());
        marginR = qRound((m_marginR + subtitle.marginR()) * scale.x());
        // Same for alignment
        if (m_alignment & Qt::AlignLeft) {
          horizontalAlign = "left";
        } else if (m_alignment & Qt::AlignRight) {
          horizontalAlign = "right";
        } else {
          horizontalAlign = "center";
        }
      } else {
        // Horizontal positioning : (x, ?)
        position.setX(bounds.x() + qRound(position.x() * scale.x()));
        final.moveLeft(textAnchor(position, line.text(), fontMetrics).x());
      }

      // Apply margins
      final = final.adjusted(marginL, marginV, -marginR, -marginV);

      // Vertical positioning based on final rect and subtitleHeight (using
      // metrics)
      if (m_alignment & Qt::AlignBottom) {
        final.moveTop(final.bottom() - subtitleHeight(subtitle, fontMetrics) +
                      stack);
      } else if (m_alignment & Qt::AlignVCenter) {
        final.moveTop(final.center().y() -
                      subtitleHeight(subtitle, fontMetrics) / 2 + stack);
      } else { // AlignTop
        final.moveTop(final.top() + stack);
      }

      // Offsets (fractions of bounds) - round to nearest pixel
      final.moveBottom(final.bottom() - qRound(m_offsetV * bounds.height()));
      final.moveLeft(final.left() + qRound(m_offsetH * bounds.width()));

      // Stack lines: use fontMetrics.height() directly (already scaled)
      stack += qRound(fontMetrics.height() * (1.0 + m_lineSpacing));
    }

    // Prepare cache key (text + font family + font point size + scale +
    // alignment)
    QString cacheKey = QString("%1|%2|%3|%4|%5")
                           .arg(line.text())
                           .arg(scaledFont.family())
                           .arg(scaledFont.pointSizeF())
                           .arg(uniformScale)
                           .arg(horizontalAlign);

    QTextDocument *doc = nullptr;

    if (m_docCache.contains(cacheKey)) {
      doc = m_docCache.value(cacheKey);
    } else {
      doc = new QTextDocument();
      m_docCache.insert(cacheKey, doc);

      // Set font and HTML once
      doc->setDefaultFont(scaledFont);
      QString html = QString("<style>p { font-size: 100%%; margin:0; "
                             "padding:0; }</style><p align=\"%1\">%2</p>")
                         .arg(horizontalAlign)
                         .arg(line.text());
      doc->setHtml(html);

      QTextFrame *tf = doc->rootFrame();
      QTextFrameFormat tff = tf->frameFormat();
      tff.setTopMargin(0);
      tff.setBottomMargin(0);
      tff.setLeftMargin(0);
      tff.setRightMargin(0);
      tf->setFrameFormat(tff);
    }

    // Set page size to fit final rect
    doc->setPageSize(QSize(qMax(1, final.width()), qMax(1, final.height())));

    QAbstractTextDocumentLayout *layout = doc->documentLayout();
    painter->setFont(scaledFont);
    painter->setPen(m_primaryColour);
    QAbstractTextDocumentLayout::PaintContext context;
    context.palette.setColor(QPalette::Text, painter->pen().color());

    painter->save();
    painter->translate(final.x(), final.y());

    if (outline.width() > 0) {
      QTextCursor cursor(doc);
      cursor.select(QTextCursor::Document);
      QTextCharFormat format;
      format.setTextOutline(outline);
      cursor.mergeCharFormat(format);
      layout->draw(painter, context);

      // remove outline
      format.setTextOutline(Qt::NoPen);
      cursor.mergeCharFormat(format);
    }
    // Draw final text (without outline)
    layout->draw(painter, context);
    painter->restore();
  }
}
