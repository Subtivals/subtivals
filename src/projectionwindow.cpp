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
#include "projectionwindow.h"

ProjectionWindow::ProjectionWindow(QWidget *parent)
    : SubtitlesForm(parent), m_hideDesktop(false), m_monitor(-1),
      m_resizable(false) {
  Qt::WindowFlags flags = Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint |
                          Qt::X11BypassWindowManagerHint;
#ifdef WIN32
  flags |= Qt::SubWindow;
#endif
  setStyleSheet("background:transparent;");
  setAttribute(Qt::WA_TranslucentBackground);
  setWindowFlags(flags);
  setCursor(QCursor(Qt::BlankCursor));
}

ProjectionWindow::~ProjectionWindow() {}

void ProjectionWindow::toggleHideDesktop(bool state) {
  int idxPrimary =
      QGuiApplication::screens().indexOf(QGuiApplication::primaryScreen());
  m_hideDesktop = (state && QGuiApplication::screens().length() > 1 &&
                   m_monitor != idxPrimary);
  if (m_hideDesktop) {
    setGeometry(m_screenGeom);
    m_topleft = (m_subtitlesGeom.topLeft() - m_screenGeom.topLeft());
  } else {
    setGeometry(m_subtitlesGeom);
    m_topleft = QPoint(0, 0);
  }

  repaint();
}

void ProjectionWindow::changeGeometry(int monitor, const QRect &r) {
  m_monitor = monitor;
  m_screenGeom = QGuiApplication::screens().at(monitor)->geometry();
  // This is sent from UI, add screen geometry
  m_subtitlesGeom =
      QRect(m_screenGeom.x() + r.x(),
            m_screenGeom.y() + m_screenGeom.height() - r.height() - r.y(),
            r.width(), r.height());
  toggleHideDesktop(m_hideDesktop);
}

void ProjectionWindow::changeGeometry(const QRect &r) {
  m_subtitlesGeom = r;
  if (!m_hideDesktop)
    setGeometry(r);
  // This is sent back to UI, substract screen geometry
  emit geometryChanged(
      QRect(r.x() - m_screenGeom.x(),
            m_screenGeom.y() + m_screenGeom.height() - r.height() - r.y(),
            r.width(), r.height()));
}

void ProjectionWindow::mousePressEvent(QMouseEvent *e) {
  m_mouseOffset = e->globalPosition() - geometry().topLeft();
}

void ProjectionWindow::mouseMoveEvent(QMouseEvent *e) {
  if (m_mouseOffset.isNull())
    return;

  if (!m_resizable)
    return;

  // Simply move the window on mouse drag
  QRect current = geometry();
  QPointF moveTo = e->globalPosition() - m_mouseOffset;
  current.moveTopLeft(moveTo.toPoint());

  changeGeometry(current);
}

void ProjectionWindow::mouseReleaseEvent(QMouseEvent *) {
  m_mouseOffset = QPoint();
}

void ProjectionWindow::wheelEvent(QWheelEvent *event) {
  if (!m_resizable)
    return;
  QRect current = geometry();
  int step = 24;
  if (event->modifiers().testFlag(Qt::ShiftModifier))
    step = 60;
  int factor = event->angleDelta().y() / step;
  if (event->modifiers().testFlag(Qt::ControlModifier)) {
    current.setHeight(current.height() + factor);
  } else {
    current.moveLeft(current.left() - factor / 2); // Keep centered
    current.setWidth(current.width() + factor);
  }
  changeGeometry(current);
}

void ProjectionWindow::screenResizable(bool state) { m_resizable = state; }
