#ifndef SUBTITLESFORM_H
#define SUBTITLESFORM_H

#include <QWidget>
#include <QList>
#include <QMouseEvent>

#include "event.h"

namespace Ui {
    class SubtitlesForm;
}

class SubtitlesForm : public QWidget
{
    Q_OBJECT

public:
    explicit SubtitlesForm(QWidget *parent = 0);
    ~SubtitlesForm();

public slots:
    void addEvent(Event *p_event);
    void remEvent(Event *p_event);
    void clearEvents();
    void toggleHide(bool state);
    void applyConfig();
protected:
    void saveConfig(const QRect& r);
    void paintEvent(QPaintEvent* p_event);
    void mouseMoveEvent(QMouseEvent *e);
    void mousePressEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    void mouseDoubleClickEvent(QMouseEvent *e);
private:
    Ui::SubtitlesForm *ui;
    int m_maxEvents;
    QList<Event *> m_currentEvents;
    bool m_visible;
    QPoint m_mouseOffset;
    QRect m_screenGeom;
};

#endif // SUBTITLESFORM_H
