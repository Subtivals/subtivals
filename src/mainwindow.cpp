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
#include <QtCore/QtGlobal>
#include <QtCore/QSettings>
#include <QtCore/QUrl>
#include <QtGui/QFileDialog>
#include <QtGui/QDesktopWidget>
#include <QtGui/QKeyEvent>
#include <QtGui/QMessageBox>
#include <QtGui/QDesktopServices>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "configeditor.h"

#define DELAY_OFFSET 250

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_preferences(new ConfigEditor(this)),
    m_speedFactor(1.0),
    m_speedFactorEnabled(false),
    m_filewatcher(new QFileSystemWatcher)
{
    ui->setupUi(this);
    ui->tableWidget->installEventFilter(this);
    m_selectEvent = true;
    m_script = 0;
    m_pauseTotal = 0;
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(timeout()));
    setState(NODATA);
    ui->tableWidget->setFocus();
    setAcceptDrops(true);

    // Add preferences dock
    m_preferences->setVisible(false);
    addDockWidget(Qt::RightDockWidgetArea, m_preferences);

    // Selection timer (disables event highlighting for a while)
    m_timerSelection.setSingleShot(true);
    m_timerSelection.setInterval(1000);
    connect(&m_timerSelection, SIGNAL(timeout()), this, SLOT(enableEventSelection()));

    // Script file watching :
    connect(m_filewatcher, SIGNAL(fileChanged(QString)), this, SLOT(fileChanged(QString)));
    m_timerFileChange.setSingleShot(true);
    connect(&m_timerFileChange, SIGNAL(timeout()), this, SLOT(reloadScript()));

    // Timer for auto-hiding ended events
    m_timerAutoHide.setSingleShot(true);
    connect(&m_timerAutoHide, SIGNAL(timeout()), this, SLOT(actionToggleHide()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event) {
    event->acceptProposedAction();
}

void MainWindow::dragMoveEvent(QDragMoveEvent* event)
{
    event->acceptProposedAction();
}

void MainWindow::dragLeaveEvent(QDragLeaveEvent* event)
{
    event->accept();
}

void MainWindow::dropEvent(QDropEvent* event)
{
    const QMimeData* mimeData = event->mimeData();
    // check for our needed mime type, here a file or a list of files
    if (mimeData->hasUrls()) {
        QStringList pathList;
        QList<QUrl> urlList = mimeData->urls();
        // extract the local paths of the files
        for (int i = 0; i < urlList.size() && i < 32; ++i) {
            QString fileName = urlList.at(i).toLocalFile();
            QFileInfo fileInfo = QFileInfo(fileName);
            if(fileInfo.isFile() && fileInfo.isReadable() && fileInfo.suffix().toLower() == "ass") {
                openFile(fileName);
                return;
            }
        }
    }
}

void MainWindow::closeEvent(QCloseEvent *)
{
    // Save settings
    QSettings settings;
    settings.beginGroup("MainWindow");
    settings.setValue("lastFolder", m_lastFolder);
    settings.setValue("size", size());
    settings.setValue("pos", pos());
    settings.setValue("reloadEnabled", m_reloadEnabled);
    settings.setValue("autoHideEnabled", m_autoHideEnabled);
    settings.setValue("showPreferences", ui->actionPreferences->isChecked());
    settings.endGroup();
    // When the main window is close : end of the app
    qApp->exit();
}

void MainWindow::showEvent(QShowEvent *)
{
    // Restore settings
    QSettings settings;
    settings.beginGroup("MainWindow");
    m_lastFolder = settings.value("lastFolder", "").toString();
    resize(settings.value("size", size()).toSize());
    QPoint pos(0, qApp->desktop()->screenCount() > 1 ? 100 : 350);
    move(settings.value("pos", pos).toPoint());
    m_reloadEnabled = settings.value("reloadEnabled", false).toBool();
    ui->actionEnableReload->setChecked(m_reloadEnabled);
    m_autoHideEnabled = settings.value("autoHideEnabled", false).toBool();
    ui->actionAutoHideEnded->setChecked(m_autoHideEnabled);
    ui->actionPreferences->setChecked(settings.value("showPreferences", false).toBool());
    settings.endGroup();

    m_preferences->apply();
}

const ConfigEditor* MainWindow::configEditor()
{
    return m_preferences;
}

void MainWindow::actionShowCalibration()
{
    openFile(":/samples/M.ass");
    updateCurrentEventAt(0);
    m_timerAutoHide.stop(); // disable auto-hide for calibration
    actionToggleHide(false);
}

void MainWindow::openFile (const QString &p_fileName)
{
    emit eventClear();
    // Save on load file
    m_preferences->save();
    // Clean-up previously allocated resources & reset GUI
    if(m_script != 0) {
        m_filewatcher->removePath(m_script->fileName());
        delete m_script;
    }
    if(m_timer.isActive()) {
        m_timer.stop();
    }
    m_msseStartTime = 0;
    m_userDelay = 0;
    m_lastEvents.clear();
    m_tableMapping.clear();
    // Create the script & setup the GUI
    m_script = new Script(p_fileName, this);
    m_preferences->setScript(m_script);  // will reset()
    // Set the window title from the file name, withour the ASS extention
    QString winTitle = QFileInfo(p_fileName).fileName();
    if (winTitle.endsWith(".ASS") || winTitle.endsWith(".ass")) {
        winTitle = winTitle.left(winTitle.length() - 4);
    }
    setWindowTitle(winTitle);
    // Update the table
    ui->tableWidget->setRowCount(m_script->eventsCount());
    QListIterator<Event *> i = m_script->events();
    int row = 0;
    while (i.hasNext()) {
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
    // Watch file changes
    if (!p_fileName.startsWith(":"))
        m_filewatcher->addPath(p_fileName);
    // Reset search field
    ui->searchField->setEnabled(row > 0);
    ui->searchField->setText("");
    m_timerAutoHide.stop();
}

void MainWindow::actionEnableReload(bool state)
{
    m_reloadEnabled = state;
    if (!state)
        m_timerFileChange.stop();
}

void MainWindow::actionAutoHideEnded(bool p_state)
{
    m_autoHideEnabled = p_state;
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
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open subtitles"), m_lastFolder, tr("Subtitle Files (*.ass)"));
    // Ass file selected ?
    if (!fileName.isEmpty()) {
        m_lastFolder = QFileInfo(fileName).absoluteDir().absolutePath();
        actionStop();
        openFile(fileName);
    }
}

void MainWindow::actionPlay()
{
    int row = ui->tableWidget->currentRow();
    m_timerAutoHide.stop();
    switch(m_state) {
    case STOPPED:
        setState(PLAYING);
        m_userDelay = 0;
        m_pauseTotal = 0;
        if (!m_msseStartTime) m_msseStartTime = tick();
        if (row > 0) {
            updateCurrentEventAt(row);
        }
        actionToggleHide(false);
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
    emit eventClear();
    setState(STOPPED);
    m_timer.stop();
    ui->timer->setText("-");
    ui->userDelay->setText("-");
    m_msseStartTime = 0;
    ui->tableWidget->selectRow(0);
}

bool MainWindow::eventFilter(QObject *object, QEvent *event)
{
    if (object == ui->tableWidget && event->type() == QEvent::KeyPress) {
        // With key Up/Down : behave the way single mouse clics do.
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Up || keyEvent->key() == Qt::Key_Down) {
            actionEventClic(ui->tableWidget->currentIndex());
        }
        //TODO: move to event(QEvent*) ?
        if (keyEvent->key() == Qt::Key_F3) {
            search();
        }
        if (ui->enableSpeedFactor->isChecked()) {
            int factor = 10;
            if (keyEvent->modifiers().testFlag(Qt::ShiftModifier))
                factor = 1;
            if (keyEvent->key() == Qt::Key_Right)
                ui->speedFactor->stepBy(factor);
            if (keyEvent->key() == Qt::Key_Left)
                ui->speedFactor->stepBy(-factor);
        }
    }
    return false;
}

void MainWindow::actionConfig(bool state)
{
    // Show/Hide the config dialog
    m_preferences->setVisible(state);
    emit screenResizable(state);
    // Save when user hides it
    if (!state) m_preferences->save();
}

void MainWindow::actionAddDelay()
{
    // Add 250 msecs
    m_userDelay += DELAY_OFFSET;
}

void MainWindow::actionSubDelay()
{
    // Sub 250 msecs
    m_userDelay -= DELAY_OFFSET;
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
        m_selectEvent = true;
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
        int row = ui->tableWidget->currentRow();
        bool isRowDisplayed = false;
        foreach(Event* e, m_lastEvents)
            if (m_tableMapping[e] == row)
                isRowDisplayed = true;

        // Jump next if selected is being viewed. Otherwise activate it.
        m_selectEvent = true;
        if (isRowDisplayed)
            updateCurrentEventAt(row + 1);
        else
            updateCurrentEventAt(row);
        ui->actionHide->setChecked(false);
    }
    ui->actionPrevious->setEnabled(canPrevious());
    ui->actionNext->setEnabled(canNext());
}

void MainWindow::actionToggleHide(bool state)
{
    if(QObject::sender() != ui->actionHide)
        ui->actionHide->setChecked(state);
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
    m_timerSelection.start();
}

void MainWindow::actionEventSelected(QModelIndex index)
{
    updateCurrentEventAt(index.row());
    ui->actionHide->setChecked(false);
    ui->actionPrevious->setEnabled(canPrevious());
    ui->actionNext->setEnabled(canNext());
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
    setElapsedTime(start_mss);
    // Show it !
    updateCurrentEvent(start_mss + 1);
    // Continuous play, even while pause
    switch(m_state) {
    case PLAYING:
        m_timer.start(100);
        break;
    case PAUSED:
        m_pauseTotal = 0;
        m_pauseStart = tick();
    case STOPPED:
        if (m_autoHideEnabled) {
            m_timerAutoHide.setInterval(m_script->eventAt(i)->duration());
            m_timerAutoHide.start();
        }
    case NODATA:
        break;
    }
}

void MainWindow::updateCurrentEvent(qint64 msecsElapsed)
{
    // Sanity check
    if(m_script == 0)
        return;
    // Find events that match elapsed time
    QList<Event *> currentEvents;
    QListIterator<Event *> i = m_script->events();
    while(i.hasNext())
    {
        Event *e = i.next();
        if(e->match(msecsElapsed))
            currentEvents.append(e);
    }
    // Compare events that match elapsed time with events that matched elapsed time last
    // time the timer was fired to find the differences
    i = QListIterator<Event *>(m_lastEvents);
    while (i.hasNext()) {
        // Events that where presents and that are no more presents : suppress
        Event *e = i.next();
        if(!currentEvents.contains(e))
            emit eventEnd(e);
    }
    i = QListIterator<Event *>(currentEvents);
    while(i.hasNext()) {
        // Events that are presents and that were not presents : add
        Event *e = i.next();
        if(!m_lastEvents.contains(e))
            emit eventStart(e);
    }
    // Update the GUI
    m_lastEvents = currentEvents;
    ui->timer->setText(ts2tc(msecsElapsed));
    ui->userDelay->setText(ts2tc(m_userDelay));
    if(m_selectEvent && currentEvents.size() > 0) {
        ui->tableWidget->selectRow(m_tableMapping[currentEvents.last()]);
        ui->tableWidget->scrollTo(ui->tableWidget->currentIndex(),
                                  QAbstractItemView::PositionAtCenter);
    }
}

QString MainWindow::ts2tc(qint64 p_ts)
{
    if (p_ts >= 0)
        return "+" + QTime().addMSecs(p_ts).toString("hh:mm:ss.zzz");
    else
        return "-" + QTime().addMSecs(-p_ts).toString("hh:mm:ss.zzz");
}

void MainWindow::setState(State p_state)
{
    m_state = p_state;
    switch(m_state) {
    case NODATA:
        ui->actionPlay->setEnabled(false);
        ui->actionStop->setEnabled(false);
        ui->actionPause->setEnabled(false);
        ui->actionPrevious->setEnabled(false);
        ui->actionNext->setEnabled(false);
        ui->actionAddDelay->setEnabled(false);
        ui->actionSubDelay->setEnabled(false);
        ui->actionAutoHideEnded->setEnabled(false);
        break;
    case STOPPED:
        ui->actionPlay->setEnabled(true);
        ui->actionStop->setEnabled(false);
        ui->actionPause->setEnabled(false);
        ui->actionPrevious->setEnabled(canPrevious());
        ui->actionNext->setEnabled(canNext());
        ui->actionAddDelay->setEnabled(false);
        ui->actionSubDelay->setEnabled(false);
        ui->actionAutoHideEnded->setEnabled(true);
        break;
    case PLAYING:
        ui->actionPlay->setEnabled(false);
        ui->actionStop->setEnabled(true);
        ui->actionPause->setEnabled(true);
        ui->actionPrevious->setEnabled(canPrevious());
        ui->actionNext->setEnabled(canNext());
        ui->actionAddDelay->setEnabled(true);
        ui->actionSubDelay->setEnabled(true);
        ui->actionAutoHideEnded->setEnabled(false);
        break;
    case PAUSED:
        ui->actionPlay->setEnabled(true);
        ui->actionStop->setEnabled(true);
        ui->actionPause->setEnabled(false);
        ui->actionPrevious->setEnabled(canPrevious());
        ui->actionNext->setEnabled(canNext());
        ui->actionAddDelay->setEnabled(false);
        ui->actionSubDelay->setEnabled(false);
        ui->actionAutoHideEnded->setEnabled(true);
        break;
    }
}

qint64 MainWindow::tick()
{
    QDateTime dateTime = QDateTime::currentDateTime();
    qint64 dt=QDate(1982, 5, 8).daysTo(dateTime.date());
    QTime tt=dateTime.time();
    return 86400000 * dt + 3600000 * tt.hour() + 60000 * tt.minute() + 1000 * tt.second() + tt.msec();
}

void MainWindow::setElapsedTime(qint64 p_elapsed)
{
    double factor = m_speedFactorEnabled ? m_speedFactor : 1.0;
    m_msseStartTime = tick() - p_elapsed/factor + m_userDelay - m_pauseTotal;
}

qint64 MainWindow::elapsedTime()
{
    // Gets the elapsed time in milliseconds
    double factor = m_speedFactorEnabled ? m_speedFactor : 1.0;
    return (tick() - m_msseStartTime + m_userDelay - m_pauseTotal) * factor;
}

void MainWindow::setSpeedFactor(double p_factor)
{
    qint64 elapsed = elapsedTime();
    m_speedFactor = p_factor/100.0;
    setElapsedTime(elapsed);
}

void MainWindow::enableSpeedFactor(bool p_state)
{
    qint64 elapsed = elapsedTime();
    m_speedFactorEnabled = p_state;
    setElapsedTime(elapsed);
}

void MainWindow::search()
{
    if (!m_script)
        return;

    QString search = ui->searchField->text();
    int found = -1;

    // Loop over the whole list, start from current
    int nb = m_script->eventsCount();
    int i = ui->tableWidget->currentRow() + 1;
    int max = i + nb;
    for (; i < max; i++) {
        const Event* e = m_script->eventAt(i % nb);
        if (e->text().contains(search, Qt::CaseInsensitive)) {
            found = i % nb;
            break;
        }
    }
    // Select the event in the list
    if (found < 0) {
        ui->searchField->setStyleSheet("QLineEdit {color: red;}");
    } else {
        ui->tableWidget->selectRow(found);
        ui->tableWidget->setFocus();
        actionEventClic(QModelIndex());
    }
}

void MainWindow::searchTextChanged(QString)
{
    ui->searchField->setStyleSheet("");
    ui->searchButton->setEnabled(!ui->searchField->text().isEmpty());
}

void MainWindow::actionAbout()
{
    QMessageBox::about(this,
                       tr("About Subtivals"),
                       tr("<h1>Subtivals</h1>"
                          "<p>Subtivals, a program to project *.ass subtitles.</p>"
                          "<h2>Authors</h2>"
                          "<li>Lilian Lefranc</li>"
                          "<li>Arnaud Rolly</li>"
                          "<li>Mathieu Leplatre</li>"));
}

void MainWindow::actionShowHelp()
{
    QDesktopServices::openUrl(QUrl("https://github.com/traxtech/subtivals/wiki/User-Manual"));
}
