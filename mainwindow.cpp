#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtGlobal>
#include <QFileDialog>
#include <QDebug>

#include "configdialog.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    m_script = 0;
    m_pauseTotal = 0;
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(timeout()));
    setState(NODATA);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::actionOpen() {
    // Ask the user for an *.ass file
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open subtitles"), QString::null, tr("Subtitle Files (*.ass)"));
    // Ass file selected ?
    if (!fileName.isEmpty()) {
        // Clean-up previously allocated resources & reset GUI
        if(m_script != 0)
        {
            delete m_script;
        }
        if(m_timer.isActive())
        {
            m_timer.stop();
        }
        m_msseStartTime = 0;
        m_userDelay = 0;
        m_lastEvents.clear();
        m_tableMapping.clear();
        // Create the script & setup the GUI
        m_script = new Script(fileName, this);
        setWindowTitle(m_script->title());
        ui->tableWidget->setRowCount(m_script->eventsCount());
        QListIterator<Event *> i = m_script->events();
        int row = 0;
        while (i.hasNext())
        {
            Event *event = i.next();
            m_tableMapping[event] = row;
            QTableWidgetItem *startItem = new QTableWidgetItem(QTime().addMSecs(event->msseStart()).toString());
            ui->tableWidget->setItem(row, COLUMN_START, startItem);
            QTableWidgetItem *endItem = new QTableWidgetItem(QTime().addMSecs(event->msseEnd()).toString());
            ui->tableWidget->setItem(row, COLUMN_END, endItem);
            QTableWidgetItem *styleItem = new QTableWidgetItem(event->style()->name());
            ui->tableWidget->setItem(row, COLUMN_STYLE, styleItem);
            QTableWidgetItem *textItem = new QTableWidgetItem(event->text());
            ui->tableWidget->setItem(row, COLUMN_TEXT, textItem);
            row++;
        }
        setState(STOPPED);
    }
}

void MainWindow::actionPlay()
{
    switch(m_state)
    {
    case STOPPED:
        setState(PLAYING);
        m_msseStartTime = QDateTime::currentMSecsSinceEpoch();
        m_userDelay = 0;
        m_pauseTotal = 0;
        m_timer.start(100);
        break;
    case PAUSED:
        setState(PLAYING);
        m_pauseTotal += QDateTime::currentMSecsSinceEpoch() - m_pauseStart;
        m_timer.start(100);
        break;
    }

}

void MainWindow::actionStop()
{
    setState(STOPPED);
    m_timer.stop();
    ui->timer->setText("-");
    ui->userDelay->setText("-");
}

void MainWindow::actionConfig()
{
    // Show the config dialog
    ConfigDialog *d = new ConfigDialog(this);
    d->setModal(true);
    d->show();
    // Config changed, emit signal
    QObject::connect(d, SIGNAL(accepted()), this, SIGNAL(configChanged()));
}

void MainWindow::actionAdd1Sec()
{
    // Add 1000 msecs
    m_userDelay += 1000;
}

void MainWindow::actionSub1Sec()
{
    // Sub 100 msecs
    m_userDelay -= 1000;
}

void MainWindow::actionPause()
{
    setState(PAUSED);
    m_pauseStart = QDateTime::currentMSecsSinceEpoch();
    m_timer.stop();
}

void MainWindow::actionNext()
{
    qDebug() << ui->tableWidget->currentRow();
    updateCurrentEventAt(ui->tableWidget->currentRow() + 1);
}

void MainWindow::actionEventSelected(QModelIndex index)
{
    updateCurrentEventAt(index.row());
}

void MainWindow::timeout()
{
    // Gets the elapsed time in milliseconds
    qint64 msseCurrentTime = QDateTime::currentMSecsSinceEpoch();
    qint64 msecsElapsed = (msseCurrentTime - m_msseStartTime) + m_userDelay - m_pauseTotal;
    updateCurrentEvent(msecsElapsed);
}

void MainWindow::updateCurrentEventAt(int i) {
    // Get event in script
    m_timer.stop();
    qint64 start_mss = m_script->eventAt(i)->msseStart();
    // Show it !
    updateCurrentEvent(start_mss + 1);
    // Continuous play, even while pause
    m_msseStartTime = QDateTime::currentMSecsSinceEpoch() - start_mss;
    switch(m_state)
    {
    case PLAYING:
        m_timer.start(100);
        break;
    case PAUSED:
        m_pauseTotal = 0;
        m_pauseStart = QDateTime::currentMSecsSinceEpoch();
        break;
    }
}

void MainWindow::updateCurrentEvent(qint64 msecsElapsed) {
    // Sanity check
    if(m_script == 0)
    {
        return;
    }
    // Find events that match elapsed time
    QList<Event *> currentEvents;
    QListIterator<Event *> i = m_script->events();
    while(i.hasNext())
    {
        Event *e = i.next();
        if(e->match(msecsElapsed))
        {
            currentEvents.append(e);
        }

    }
    // Compare events that match elapsed time with events that matched elapsed time last
    // time the timer was fired to find the differences
    i = QListIterator<Event *>(m_lastEvents);
    while (i.hasNext())
    {
        // Events that where presents and that are no more presents : suppress
        Event *e = i.next();
        if(!currentEvents.contains(e))
        {
            emit eventEnd(e);
        }
    }
    i = QListIterator<Event *>(currentEvents);
    while(i.hasNext())
    {
        // Events that are presents and that were not presents : add
        Event *e = i.next();
        if(!m_lastEvents.contains(e))
        {
            emit eventStart(e);
        }
    }
    // Update the GUI
    m_lastEvents = currentEvents;
    ui->timer->setText(ts2tc(msecsElapsed));
    ui->userDelay->setText(ts2tc(m_userDelay));
    if(currentEvents.size() > 0)
    {
        ui->tableWidget->selectRow(m_tableMapping[currentEvents.last()]);
    }
}

void MainWindow::closeEvent(QCloseEvent *)
{
    // When the main window is close : end of the app
    qApp->exit();
}

QString MainWindow::ts2tc(qint64 p_ts)
{
    if (p_ts >= 0)
    {
        return "+" + QTime().addMSecs(p_ts).toString();
    } else {
        return "-" + QTime().addMSecs(-p_ts).toString();
    }
}

void MainWindow::setState(State p_state)
{
    m_state = p_state;
    switch(m_state)
    {
    case NODATA:
        ui->actionPlay->setEnabled(false);
        ui->actionStop->setEnabled(false);
        ui->actionPause->setEnabled(false);
        ui->actionNext->setEnabled(false);
        ui->actionAdd1Sec->setEnabled(false);
        ui->actionSub1Sec->setEnabled(false);
        break;
    case STOPPED:
        ui->actionPlay->setEnabled(true);
        ui->actionStop->setEnabled(false);
        ui->actionPause->setEnabled(false);
        ui->actionNext->setEnabled(false);
        ui->actionAdd1Sec->setEnabled(false);
        ui->actionSub1Sec->setEnabled(false);
        break;
    case PLAYING:
        ui->actionPlay->setEnabled(false);
        ui->actionStop->setEnabled(true);
        ui->actionPause->setEnabled(true);
        ui->actionNext->setEnabled(true);
        ui->actionAdd1Sec->setEnabled(true);
        ui->actionSub1Sec->setEnabled(true);
        break;
    case PAUSED:
        ui->actionPlay->setEnabled(true);
        ui->actionStop->setEnabled(false);
        ui->actionPause->setEnabled(false);
        ui->actionNext->setEnabled(true);
        ui->actionAdd1Sec->setEnabled(false);
        ui->actionSub1Sec->setEnabled(false);
        break;
    }
}
