#include "subtitlesform.h"
#include "ui_subtitlesform.h"

#include <QPainter>
#include <QSettings>
#include <QDesktopWidget>

#include "style.h"

SubtitlesForm::SubtitlesForm(QWidget *parent) :
        QWidget(parent, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::X11BypassWindowManagerHint ),
    ui(new Ui::SubtitlesForm),
    m_maxEvents(2),
    m_visible(true)
{
    ui->setupUi(this);
    applyConfig();
}

SubtitlesForm::~SubtitlesForm()
{
    delete ui;
}

void SubtitlesForm::addEvent(Event *p_event)
{
    m_currentEvents.append(p_event);
    if(m_currentEvents.size() > m_maxEvents)
    {
        m_currentEvents.removeFirst();
    }
    repaint();
}

void SubtitlesForm::remEvent(Event *p_event)
{
    m_currentEvents.removeOne(p_event);
    repaint();
}

void SubtitlesForm::toggleHide(bool state)
{
    m_visible = !state;
    repaint();
}

void SubtitlesForm::applyConfig()
{
    QSettings settings;
    settings.beginGroup("SubtitlesForm");
    int screen = settings.value("screen", 0).toInt();
    int x = settings.value("x", 0).toInt();
    settings.setValue("x", x);
    int y = settings.value("y", 0).toInt();
    settings.setValue("y", y);
    int w = settings.value("w", qApp->desktop()->screen(qApp->desktop()->primaryScreen())->width()).toInt();
    settings.setValue("w", w);
    int h = settings.value("h", 300).toInt();
    settings.setValue("h", h);
    settings.endGroup();
    QRect geom = QApplication::desktop()->screenGeometry(screen);
    setGeometry(x + geom.x(), y + geom.y(), w, h);
}

void SubtitlesForm::paintEvent(QPaintEvent*)
{
    QRect bounds(0, 0, width(), height());
    QPainter p(this);
    // Black background
    p.fillRect(bounds, Qt::black);
    // Draw text only if visible
    if (!m_visible){
        return;
    }
    for(int i = 0; i < m_maxEvents && i < m_currentEvents.size(); i++)
    {
        Event *e = m_currentEvents.at(i);
        e->style()->drawEvent(&p, *e, bounds);
    }
}
