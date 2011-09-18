#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QList>
#include <QTime>
#include <QMap>
#include <QTableWidget>
#include <QModelIndex>
#include <QString>
#include <QFileSystemWatcher>

#include "script.h"


#define COLUMN_START    0
#define COLUMN_END      1
#define COLUMN_STYLE    2
#define COLUMN_TEXT     3


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
    void openFile (const QString &p_fileName);
signals:
    void eventStart(Event *p_event);
    void eventEnd(Event *p_event);
    void eventClear();
    void configChanged();
    void toggleHide(bool state);
public slots:
    void actionOpen();
    void actionPlay();
    void actionStop();
    void actionConfig();
    void actionAddDelay();
    void actionSubDelay();
    void actionPause();
    void actionNext();
    void actionPrevious();
    void actionToggleHide(bool);
    void timeout();
    void actionEnableReload(bool);
    void fileChanged(QString path);
    void reloadScript();
    void actionEventClic(QModelIndex);
    void actionEventSelected(QModelIndex);
    void updateCurrentEvent(qint64);
    void updateCurrentEventAt(int);
    void enableEventSelection();
protected:
    bool canNext();
    bool canPrevious();
    void closeEvent(QCloseEvent *);
    QString ts2tc(qint64 p_timestamp);
    void setState(State p_state);
    qint64 tick();
    qint64 elapsedTime();
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
    bool m_selectEvent;
    bool m_reloadEnabled;
    QFileSystemWatcher* m_filewatcher;
    QTimer m_timerFileChange;
};

#endif // MAINWINDOW_H
