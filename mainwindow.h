#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QList>
#include <QTime>
#include <QMap>
#include <QTableWidget>

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
signals:
    void eventStart(Event *p_event);
    void eventEnd(Event *p_event);
    void configChanged();
public slots:
    void actionOpen();
    void actionPlay();
    void actionStop();
    void actionConfig();
    void timeout();
protected:
    void closeEvent(QCloseEvent *);
private:
    Ui::MainWindow *ui;
    Script *m_script;
    QTimer m_timer;
    qint64 m_msseStartTime;
    QList<Event *> m_lastEvents;
    QMap<Event *, int> m_tableMapping;
};

#endif // MAINWINDOW_H
