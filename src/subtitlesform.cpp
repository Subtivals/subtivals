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
#include <QDesktopWidget>
#include <QtCore/QSettings>
#include <QtCore/qmath.h>
#include <QtGui/QCursor>
#include <QtGui/QPainter>
#include "QtGui/QScreen.h"

#include "style.h"
#include "subtitlesform.h"
#include "ui_subtitlesform.h"

SubtitlesForm::SubtitlesForm(QWidget *parent)
    : QWidget(parent), ui(new Ui::SubtitlesForm), m_visible(true),
      m_hideDesktop(false), m_monitor(-1), m_resizable(false), m_rotation(0),
      m_color(Qt::black) {
  Qt::WindowFlags flags = Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint |
                          Qt::X11BypassWindowManagerHint;
#ifdef WIN32
  flags |= Qt::SubWindow;
#endif
  setStyleSheet("background:transparent;");
  setAttribute(Qt::WA_TranslucentBackground);
  setWindowFlags(flags);
  ui->setupUi(this);
  setCursor(QCursor(Qt::BlankCursor));
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

void SubtitlesForm::toggleHideDesktop(bool state) {
  int primaryScreen = QGuiApplication::screens().indexOf(QGuiApplication::primaryScreen());
  m_hideDesktop = (state && QGuiApplication::screens().length() > 1 &&
                   m_monitor != primaryScreen);
  if (m_hideDesktop)
    setGeometry(m_screenGeom);
  else
    setGeometry(m_subtitlesGeom);
  repaint();
}

void SubtitlesForm::changeGeometry(int monitor, const QRect &r) {
  m_monitor = monitor;
  m_screenGeom = QGuiApplication::screens().at(monitor)->geometry();
  // This is sent from UI, add screen geometry
  m_subtitlesGeom =
      QRect(m_screenGeom.x() + r.x(),
            m_screenGeom.y() + m_screenGeom.height() - r.height() - r.y(),
            r.width(), r.height());
  toggleHideDesktop(m_hideDesktop);
}

void SubtitlesForm::changeGeometry(const QRect &r) {
  m_subtitlesGeom = r;
  if (!m_hideDesktop)
    setGeometry(r);
  // This is sent back to UI, substract screen geometry
  emit geometryChanged(
      QRect(r.x() - m_screenGeom.x(),
            m_screenGeom.y() + m_screenGeom.height() - r.height() - r.y(),
            r.width(), r.height()));
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

  // Rectangle where subtitles are drawn : should be relative to current window
  QRect subtitlesBounds(0, 0, m_subtitlesGeom.width(),
                        m_subtitlesGeom.height());
  if (m_hideDesktop) {
    subtitlesBounds.moveTopLeft(m_subtitlesGeom.topLeft() -
                                m_screenGeom.topLeft());
  }
  foreach (Subtitle *e, m_currentSubtitles) {
    if (e && e->style())
      e->style()->drawSubtitle(&p, *e, subtitlesBounds, m_outline);
  }
}

void SubtitlesForm::mousePressEvent(QMouseEvent *e) {
  m_mouseOffset = e->globalPos() - geometry().topLeft();
}

void SubtitlesForm::mouseMoveEvent(QMouseEvent *e) {
  if (m_mouseOffset.isNull())
    return;

  if (!m_resizable)
    return;

  // Simply move the window on mouse drag
  QRect current = geometry();
  QPoint moveTo = e->globalPos() - m_mouseOffset;
  current.moveTopLeft(moveTo);

  changeGeometry(current);
}

void SubtitlesForm::mouseReleaseEvent(QMouseEvent *) {
  m_mouseOffset = QPoint();
}

void SubtitlesForm::wheelEvent(QWheelEvent *event) {
  if (!m_resizable)
    return;
  QRect current = geometry();
  int step = 24;
  if (event->modifiers().testFlag(Qt::ShiftModifier))
    step = 60;
  int factor = event->angleDelta().y() / step;
  if (event->angleDelta().x() > 0 ||
      event->modifiers().testFlag(Qt::ControlModifier)) {
    current.setHeight(current.height() + factor);
  } else {
    current.moveLeft(current.left() - factor / 2); // Keep centered
    current.setWidth(current.width() + factor);
  }
  changeGeometry(current);
}

void SubtitlesForm::screenResizable(bool state) { m_resizable = state; }

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
