#include <QtCore/QtGlobal>
#include <QtCore/QSettings>
#include <QtGui/QFileDialog>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "configdialog.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_filewatcher(new QFileSystemWatcher)
{
    ui->setupUi(this);
    m_selectEvent = true;
    m_script = 0;
    m_pauseTotal = 0;
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(timeout()));
    setState(NODATA);

    // Restore settings
    QSettings settings;
    m_reloadEnabled = settings.value("MainWindow/reloadEnabled", false).toBool();
    ui->actionEnableReload->setChecked(m_reloadEnabled);

    // Script file watching :
    connect(m_filewatcher, SIGNAL(fileChanged(QString)), this, SLOT(fileChanged(QString)));
    m_timerFileChange.setSingleShot(true);
    connect(&m_timerFileChange, SIGNAL(timeout()), this, SLOT(reloadScript()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::openFile (const QString &p_fileName)
{
    emit eventClear();
    // Clean-up previously allocated resources & reset GUI
    if(m_script != 0)
    {
        m_filewatcher->removePath(m_script->fileName());
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
    m_script = new Script(p_fileName, this);
    m_filewatcher->addPath(p_fileName);
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
        QTableWidgetItem *textItem = new QTableWidgetItem(event->prettyText());
        ui->tableWidget->setItem(row, COLUMN_TEXT, textItem);
        row++;
    }
    actionStop();
}

void MainWindow::actionEnableReload(bool state)
{
    m_reloadEnabled = state;
    if (!state)
        m_timerFileChange.stop();
}

void MainWindow::fileChanged(QString path)
{
    // Script file is being modified.
    // Wait that no change is made during 1sec before warning the user.
    if (m_reloadEnabled && path == m_script->fileName() && QFile(path).exists())
        m_timerFileChange.start(1000);
    else
        m_timerFileChange.stop();
}

void MainWindow::reloadScript()
{
    // Script file has changed, reload.
    // Store current position
    qint64 msseStartTime = m_msseStartTime;
    qint64 userDelay = m_userDelay;
    // Store current playing state
    State previous = m_state;
    // Hide current subtitles
    QListIterator<Event*> it(m_lastEvents);
    while(it.hasNext())
        emit eventEnd(it.next());

    // Reload file path
    openFile(QString(m_script->fileName())); //force copy
    statusBar()->showMessage(tr("Subtitle file reloaded."), 5000);

    // Restore state
    setState(previous);
    // Restore position
    m_msseStartTime = msseStartTime;
    m_userDelay = userDelay;
    if (m_state == PLAYING)
        m_timer.start(100);
}

void MainWindow::actionOpen()
{
    // Ask the user for an *.ass file
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open subtitles"), QString::null, tr("Subtitle Files (*.ass)"));
    // Ass file selected ?
    if (!fileName.isEmpty())
    {
        actionStop();
        openFile(fileName);
    }
}

void MainWindow::actionPlay()
{
    switch(m_state)
    {
    case STOPPED:
        setState(PLAYING);
        if (!m_msseStartTime) m_msseStartTime = tick();
        m_userDelay = 0;
        m_pauseTotal = 0;
        m_timer.start(100);
        break;
    case PAUSED:
        setState(PLAYING);
        m_pauseTotal += tick() - m_pauseStart;
        m_timer.start(100);
        break;
    case NODATA:
    case PLAYING:
        break;
    }

}

void MainWindow::actionStop()
{
    setState(STOPPED);
    m_timer.stop();
    ui->timer->setText("-");
    ui->userDelay->setText("-");
    m_msseStartTime = 0;
    ui->tableWidget->selectRow(0);
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

void MainWindow::actionAddDelay()
{
    // Add 250 msecs
    m_userDelay += 250;
}

void MainWindow::actionSubDelay()
{
    // Sub 250 msecs
    m_userDelay -= 250;
}

void MainWindow::actionPause()
{
    setState(PAUSED);
    m_pauseStart = tick();
    m_timer.stop();
}

bool MainWindow::canPrevious()
{
    return ui->tableWidget->currentRow() > 0;
}

void MainWindow::actionPrevious()
{
    if (canPrevious()) {
        m_userDelay = 0;
        int i = ui->tableWidget->currentRow();
        updateCurrentEventAt(i - 1);
        ui->actionHide->setChecked(false);
    }
    ui->actionPrevious->setEnabled(canPrevious());
    ui->actionNext->setEnabled(canNext());
}

bool MainWindow::canNext()
{
    return ui->tableWidget->currentRow() < ui->tableWidget->rowCount() - 1;
}

void MainWindow::actionNext()
{
    if (canNext()){
        m_userDelay = 0;
        int i = ui->tableWidget->currentRow();
        if (elapsedTime() < m_script->eventAt(i)->msseStart())
            updateCurrentEventAt(i);
        else
            updateCurrentEventAt(i + 1);
        ui->actionHide->setChecked(false);
    }
    ui->actionPrevious->setEnabled(canPrevious());
    ui->actionNext->setEnabled(canNext());
}

void MainWindow::actionToggleHide(bool state)
{
    emit toggleHide(state);
}

void MainWindow::enableEventSelection()
{
    m_selectEvent = true;
}

void MainWindow::actionEventClic(QModelIndex)
{
    // Disable selection of events for some time
    // in order to let the user perform a double-clic
    m_selectEvent = false;
    QTimer::singleShot(1000, this, SLOT(enableEventSelection()));
}

void MainWindow::actionEventSelected(QModelIndex index)
{
    updateCurrentEventAt(index.row());
    ui->actionHide->setChecked(false);
}

void MainWindow::timeout()
{
    updateCurrentEvent(elapsedTime());
}

void MainWindow::updateCurrentEventAt(int i)
{
    // Get event in script
    m_timer.stop();
    qint64 start_mss = m_script->eventAt(i)->msseStart();
    // Show it !
    updateCurrentEvent(start_mss + 1);
    // Continuous play, even while pause
    m_msseStartTime = tick() - start_mss - m_pauseTotal;
    switch(m_state)
    {
    case PLAYING:
        m_timer.start(100);
        break;
    case PAUSED:
        m_pauseTotal = 0;
        m_pauseStart = tick();
        break;
    case NODATA:
    case STOPPED:
        break;
    }
}

void MainWindow::updateCurrentEvent(qint64 msecsElapsed)
{
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
    if(m_selectEvent && currentEvents.size() > 0)
    {
        ui->tableWidget->selectRow(m_tableMapping[currentEvents.last()]);
    }
}

void MainWindow::closeEvent(QCloseEvent *)
{
    // Restore settings
    QSettings settings;
    settings.beginGroup("MainWindow");
    settings.setValue("reloadEnabled", m_reloadEnabled);
    settings.endGroup();
    // When the main window is close : end of the app
    qApp->exit();
}

QString MainWindow::ts2tc(qint64 p_ts)
{
    if (p_ts >= 0)
    {
        return "+" + QTime().addMSecs(p_ts).toString("hh:mm:ss.zzz");
    } else {
        return "-" + QTime().addMSecs(-p_ts).toString("hh:mm:ss.zzz");
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
        ui->actionPrevious->setEnabled(false);
        ui->actionNext->setEnabled(false);
        ui->actionAddDelay->setEnabled(false);
        ui->actionSubDelay->setEnabled(false);
        break;
    case STOPPED:
        ui->actionPlay->setEnabled(true);
        ui->actionStop->setEnabled(false);
        ui->actionPause->setEnabled(false);
        ui->actionPrevious->setEnabled(canPrevious());
        ui->actionNext->setEnabled(canNext());
        ui->actionAddDelay->setEnabled(false);
        ui->actionSubDelay->setEnabled(false);
        break;
    case PLAYING:
        ui->actionPlay->setEnabled(false);
        ui->actionStop->setEnabled(true);
        ui->actionPause->setEnabled(true);
        ui->actionPrevious->setEnabled(canPrevious());
        ui->actionNext->setEnabled(canNext());
        ui->actionAddDelay->setEnabled(true);
        ui->actionSubDelay->setEnabled(true);
        break;
    case PAUSED:
        ui->actionPlay->setEnabled(true);
        ui->actionStop->setEnabled(true);
        ui->actionPause->setEnabled(false);
        ui->actionPrevious->setEnabled(canPrevious());
        ui->actionNext->setEnabled(canNext());
        ui->actionAddDelay->setEnabled(false);
        ui->actionSubDelay->setEnabled(false);
        break;
    }
}

qint64 MainWindow::tick()
{
    QDateTime dateTime = QDateTime::currentDateTime();
    qint64 dt=dateTime.date().daysTo(QDate(1978, 9, 9));
    QTime tt=dateTime.time();
    return 86400000 * dt + 3600000 * tt.hour() + 60000 * tt.minute() + 1000 * tt.second() + tt.msec();
}

qint64 MainWindow::elapsedTime()
{
    // Gets the elapsed time in milliseconds
    qint64 msseCurrentTime = tick();
    return (msseCurrentTime - m_msseStartTime) + m_userDelay - m_pauseTotal;
}
