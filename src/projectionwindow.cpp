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
#include <QScreen>
#include <QTimer>

#include "subtitlestyle.h"
#include "projectionwindow.h"
#include "macwindowhelper.h"

ProjectionWindow::ProjectionWindow(QWidget *parent)
    : SubtitlesForm(parent), m_hideDesktop(false), m_monitor(-1),
      m_resizable(true),
      // Wayland does not let apps position their windows: cover the whole
      // screen and draw the subtitles at their position within it.
      m_alwaysFullScreen(
          QGuiApplication::platformName().startsWith("wayland")) {
  Qt::WindowFlags flags = Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint |
                          Qt::NoDropShadowWindowHint;
#ifdef WIN32
  flags |= Qt::SubWindow;
#endif
#ifdef Q_OS_LINUX
  flags |= Qt::X11BypassWindowManagerHint;
#endif
  setStyleSheet("background:transparent;");
  setAttribute(Qt::WA_TranslucentBackground);
  setWindowFlags(flags);
  setCursor(QCursor(Qt::BlankCursor));
  makeWindowCoverMenuBar(this, true);
  m_drawBounds = false;
  m_fixedScale = true;

  m_geometryChangedDebounceTimer = new QTimer(this);
  m_geometryChangedDebounceTimer->setSingleShot(true);
  connect(m_geometryChangedDebounceTimer, &QTimer::timeout, this, [this]() {
    emit geometryChanged(m_monitor, m_subtitlesGeomBottomScreenRelative);
  });
}

ProjectionWindow::~ProjectionWindow() {}

void ProjectionWindow::screenResizable(bool state) { m_resizable = state; }

void ProjectionWindow::toggleHideDesktop(bool state) {
  // Do not hide desktop if projection window is on primary screen.
  int idxPrimary =
      QGuiApplication::screens().indexOf(QGuiApplication::primaryScreen());
  m_hideDesktop = (state && QGuiApplication::screens().length() > 1 &&
                   m_monitor != idxPrimary);

  if (m_alwaysFullScreen) {
    applyFullScreen();
  } else if (m_hideDesktop) {
    setGeometry(m_screenGeom);
  } else {
    QRect widgetGeom = QRect(m_subtitlesGeomBottomScreenRelative);
    // Convert relative to screen (bottom-aligned) to absolute on desktop
    // (top-aligned)
    widgetGeom.moveTo(m_screenGeom.x() + widgetGeom.x(),
                      m_screenGeom.y() + m_screenGeom.height() -
                          (widgetGeom.y() + widgetGeom.height()));

    setGeometry(widgetGeom);
  }

  repaint();
}

void ProjectionWindow::setVisible(bool visible) {
  // A full screen window on the only screen would cover the main window.
  if (visible && m_alwaysFullScreen && QGuiApplication::screens().size() < 2) {
    static bool warned = false;
    if (!warned)
      qWarning() << "Projection window needs a second screen on Wayland";
    warned = true;
    visible = false;
  }
  SubtitlesForm::setVisible(visible);
}

void ProjectionWindow::applyFullScreen() {
  const auto screens = QGuiApplication::screens();
  if (m_monitor < 0 || m_monitor >= screens.size())
    return;
  QScreen *target = screens.at(m_monitor);
  if (screen() != target) {
    // The compositor picks the output when the window goes full screen.
    hide();
    setScreen(target);
  }
  setWindowState(Qt::WindowFullScreen);
  show();
  updateInputMask();
}

void ProjectionWindow::updateInputMask() {
  if (!m_alwaysFullScreen)
    return;
  // Let clicks go through the transparent part of the screen.
  if (m_hideDesktop)
    clearMask();
  else
    setMask(subtitlesBounds());
}

QRect ProjectionWindow::backgroundRect() {
  if (m_alwaysFullScreen && !m_hideDesktop)
    return subtitlesBounds();
  return SubtitlesForm::backgroundRect();
}

QRect ProjectionWindow::subtitlesBounds() {
  if (!m_hideDesktop && !m_alwaysFullScreen) {
    // If we don't hide desktop, we just fill the window.
    return QRect(0, 0, width(), height());
  }
  // The rectangle in which the subtitles are drawn is relative to current
  // widget, and top-aligned.
  QRect subtitlesBounds = QRect(m_subtitlesGeomBottomScreenRelative);
  subtitlesBounds.moveTop(m_screenGeom.height() -
                          (m_subtitlesGeomBottomScreenRelative.y() +
                           m_subtitlesGeomBottomScreenRelative.height()));
  return subtitlesBounds;
}

// Slot called when UI tells us to change geometry relative to a given screen.
void ProjectionWindow::changeGeometry(int monitor, const QRect &r) {
  const auto screens = QGuiApplication::screens();
  // Validate monitor index
  if (monitor < 0 || monitor >= screens.size()) {
    qWarning() << "Invalid monitor index:" << monitor;
    return;
  }
  // Validate rectangle
  if (!r.isValid()) {
    qWarning() << "Invalid geometry rectangle:" << r;
    return;
  }

  m_monitor = monitor;
  m_screenGeom = screens.at(monitor)->geometry();
  m_dpi = screens.at(monitor)->logicalDotsPerInchY();
  m_subtitlesGeomBottomScreenRelative = r;
  // Refresh widget (will call `setGeometry()`):
  toggleHideDesktop(m_hideDesktop);
}

void ProjectionWindow::mousePressEvent(QMouseEvent *e) {
  if (m_alwaysFullScreen)
    m_mouseOffset = e->position() - subtitlesBounds().topLeft();
  else
    m_mouseOffset = e->globalPosition() - geometry().topLeft();
}

void ProjectionWindow::mouseMoveEvent(QMouseEvent *e) {
  if (m_mouseOffset.isNull())
    return;

  if (!m_resizable || m_hideDesktop)
    return;

  if (m_alwaysFullScreen) {
    // Move the subtitles within the full screen window.
    QRect current = subtitlesBounds();
    current.moveTopLeft((e->position() - m_mouseOffset).toPoint());
    applyGeometry(current.translated(m_screenGeom.topLeft()));
    return;
  }

  // Simply move the window on mouse drag
  QRect current = geometry();
  QPointF moveTo = e->globalPosition() - m_mouseOffset;
  current.moveTopLeft(moveTo.toPoint());

  const auto screens = QGuiApplication::screens();
  int halfAreaWindow = this->width() * this->height() / 2;
  for (int i = 0; i < screens.size(); ++i) {
    QRect intersected = this->geometry().intersected(screens[i]->geometry());
    int intersectionArea = intersected.width() * intersected.height();
    if (intersectionArea > halfAreaWindow) {
      m_monitor = i;
      m_screenGeom = screens[i]->geometry();
      break;
    }
  }

  applyGeometry(current);
}

void ProjectionWindow::mouseReleaseEvent(QMouseEvent *) {
  m_mouseOffset = QPoint();
}

void ProjectionWindow::wheelEvent(QWheelEvent *event) {
  if (!m_resizable || m_hideDesktop)
    return;

  QRect current = m_alwaysFullScreen
                      ? subtitlesBounds().translated(m_screenGeom.topLeft())
                      : geometry();
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
  applyGeometry(current);
}

void ProjectionWindow::applyGeometry(const QRect &r) {
  if (!m_alwaysFullScreen) {
    setGeometry(r);

    // Hide menu bar if not single and main screen.
    int idxPrimary =
        QGuiApplication::screens().indexOf(QGuiApplication::primaryScreen());
    bool notMainScreen =
        QGuiApplication::screens().length() > 1 && m_monitor != idxPrimary;
    makeWindowCoverMenuBar(this, notMainScreen);
  }

  // Convert back from absolute (top-aligned) to relative to screen
  // (bottom-aligned)
  m_subtitlesGeomBottomScreenRelative = r;
  m_subtitlesGeomBottomScreenRelative.moveTo(
      r.x() - m_screenGeom.x(),
      m_screenGeom.y() + m_screenGeom.height() - r.height() - r.y());

  if (m_alwaysFullScreen) {
    updateInputMask();
    repaint();
  }

  // Start/restart timer to emit geometryChanged later
  // Store params for debounce emit
  m_geometryChangedDebounceTimer->start(100); // debounce delay (e.g., 100ms)
}
