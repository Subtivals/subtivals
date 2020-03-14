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
#ifndef SUBTITLESFORM_H
#define SUBTITLESFORM_H

#include <QColor>
#include <QList>
#include <QMouseEvent>
#include <QPen>
#include <QWidget>

#include "subtitle.h"

namespace Ui {
class SubtitlesForm;
}

class SubtitlesForm : public QWidget {
  Q_OBJECT

public:
  explicit SubtitlesForm(QWidget *parent = nullptr);
  ~SubtitlesForm();
signals:
  void geometryChanged(QRect);
public slots:
  void addSubtitle(Subtitle *p_subtitle);
  void remSubtitle(Subtitle *p_subtitle);
  void clearSubtitles();
  void toggleHide(bool state);
  void toggleHideDesktop(bool state);
  void screenResizable(bool state);
  void changeGeometry(int, const QRect &);
  void rotate(double);
  void color(QColor);
  void outline(QColor, int);

protected:
  void changeGeometry(const QRect &);
  void paintEvent(QPaintEvent *p_event);
  void mouseMoveEvent(QMouseEvent *e);
  void mousePressEvent(QMouseEvent *e);
  void mouseReleaseEvent(QMouseEvent *e);
  void wheelEvent(QWheelEvent *e);

private:
  Ui::SubtitlesForm *ui;
  QList<Subtitle *> m_currentSubtitles;
  bool m_visible;
  QPoint m_mouseOffset;
  QRect m_screenGeom;
  QRect m_subtitlesGeom;
  bool m_hideDesktop;
  int m_monitor;
  bool m_resizable;
  qreal m_rotation;
  QColor m_color;
  QPen m_outline;
};

#endif // SUBTITLESFORM_H
