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
#include <QtCore/QSettings>
#include <QtCore/qmath.h>
#include <QtGui/QCursor>
#include <QtGui/QPainter>
#include <QGuiApplication>

#include "subtitlestyle.h"
#include "subtitlesform.h"
#include "ui_subtitlesform.h"

SubtitlesForm::SubtitlesForm(QWidget *parent)
    : QWidget(parent), ui(new Ui::SubtitlesForm), m_visible(true),
      m_rotation(0), m_color(Qt::black) {
  setStyleSheet("background:transparent;");
  ui->setupUi(this);
}

SubtitlesForm::~SubtitlesForm() { delete ui; }

void SubtitlesForm::addSubtitle(Subtitle *p_subtitle) {
  m_currentSubtitles.append(p_subtitle);
  repaint();
}

void SubtitlesForm::remSubtitle(Subtitle *p_subtitle) {
  m_currentSubtitles.removeOne(p_subtitle);
  repaint();
}

void SubtitlesForm::clearSubtitles() {
  m_currentSubtitles.clear();
  repaint();
}

void SubtitlesForm::toggleHide(bool state) {
  m_visible = !state;
  repaint();
}

void SubtitlesForm::paintEvent(QPaintEvent *) {
  QPainter p(this);
  // Black background
  p.fillRect(QRect(0, 0, width(), height()), m_color);
  // Draw text only if visible
  if (!m_visible)
    return;
  // Rotate from top left or top right
  if (m_rotation < 0) {
    QPoint topRight(geometry().width(), 0);
    p.translate(topRight);
    p.rotate(m_rotation);
    p.translate(QPoint() - topRight);
  } else {
    p.rotate(m_rotation);
  }

  // Rectangle where subtitles are drawn
  QRect subtitlesBounds(m_topleft.x(), m_topleft.y(), width(), height());
  qInfo() << subtitlesBounds << m_currentSubtitles.size();
  foreach (Subtitle *e, m_currentSubtitles) {
    if (e && e->style()) {
      e->style()->drawSubtitle(&p, *e, subtitlesBounds, m_outline);
    }
  }
}

void SubtitlesForm::rotate(double p_rotation) {
  m_rotation = p_rotation;
  repaint();
}

void SubtitlesForm::color(QColor c) {
  m_color = c;
  repaint();
}

void SubtitlesForm::outline(QColor c, int width) {
  m_outline = QPen(c);
  m_outline.setWidth(width);
}
