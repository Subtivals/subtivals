#ifndef SUBTITLESFORM_H
#define SUBTITLESFORM_H

#include <QWidget>
#include <QList>

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
    void applyConfig();
protected:
    void paintEvent(QPaintEvent* p_event);
private:
    Ui::SubtitlesForm *ui;
    int m_maxEvents;
    QList<Event *> m_currentEvents;
};

#endif // SUBTITLESFORM_H
