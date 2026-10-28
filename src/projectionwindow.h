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
#ifndef PROJECTION_WINDOW_H
#define PROJECTION_WINDOW_H

#include <QMouseEvent>
#include <QWidget>

#include "subtitlesform.h"

namespace Ui {
  class ProjectionWindow;
}

class ProjectionWindow : public SubtitlesForm {
  Q_OBJECT

public:
  explicit ProjectionWindow(QWidget *parent = nullptr);
  ~ProjectionWindow();
signals:
  void geometryChanged(int, QRect);
public slots:
  void toggleHideDesktop(bool state);
  void screenResizable(bool state);
  void changeGeometry(int, const QRect &);
  void setVisible(bool visible) override;

protected:
  void mouseMoveEvent(QMouseEvent *e) override;
  void mousePressEvent(QMouseEvent *e) override;
  void mouseReleaseEvent(QMouseEvent *e) override;
  void wheelEvent(QWheelEvent *e) override;
  QRect subtitlesBounds() override;
  QRect backgroundRect() override;

private:
  void applyGeometry(const QRect &);
  void applyFullScreen();
  void updateInputMask();

  Ui::ProjectionWindow *ui;
  QPointF m_mouseOffset;
  QRect m_screenGeom;
  QRect m_subtitlesGeomBottomScreenRelative;
  bool m_hideDesktop;
  int m_monitor;
  bool m_resizable;
  bool m_alwaysFullScreen;

  QTimer *m_geometryChangedDebounceTimer;
};

#endif // PROJECTION_WINDOW_H
