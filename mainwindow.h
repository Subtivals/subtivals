#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QList>
#include <QTime>
#include <QMap>
#include <QTableWidget>
#include <QString>

#include "script.h"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    enum State { NODATA, STOPPED, PLAYING, PAUSED};
signals:
    void eventStart(Event *p_event);
    void eventEnd(Event *p_event);
    void configChanged();
public slots:
    void actionOpen();
    void actionPlay();
    void actionStop();
    void actionConfig();
    void actionAdd1Sec();
    void actionSub1Sec();
    void actionPause();
    void timeout();
protected:
    void closeEvent(QCloseEvent *);
    QString ts2tc(qint64 p_timestamp);
    void setState(State p_state);
private:
    State m_state;
    Ui::MainWindow *ui;
    Script *m_script;
    QTimer m_timer;
    qint64 m_msseStartTime;
    qint64 m_pauseStart;
    qint64 m_pauseTotal;
    qint64 m_userDelay;
    QList<Event *> m_lastEvents;
    QMap<Event *, int> m_tableMapping;
};

#endif // MAINWINDOW_H
