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
#include <QtCore/QThread>
#include <QtGui/QFileDialog>
#include <QtGui/QDesktopWidget>
#include <QtGui/QKeyEvent>
#include <QtGui/QMessageBox>
#include <QtGui/QDesktopServices>
#include <QtGui/QScrollBar>
#include <QtGui/QPainter>
#include <QtGui/QStyledItemDelegate>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "configeditor.h"
#include "player.h"


/**
 * A small delegate class to allow rich text rendering in main table cells.
 */
class EventTableDelegate : public QStyledItemDelegate
{
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        QTextDocument document;
        QVariant value = index.data(Qt::DisplayRole);
        // Draw background with cell style
        QStyleOptionViewItemV4 options = option;
        initStyleOption(&options, index);
        options.text = "";
        options.widget->style()->drawControl(QStyle::CE_ItemViewItem, &options, painter);
        // Render rich text
        if (value.isValid() && !value.isNull()) {
            document.setHtml(value.toString());
            painter->translate(option.rect.topLeft());
            document.drawContents(painter);
            painter->translate(-option.rect.topLeft());
        }
    }
};


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_script(0),
    m_player(new Player()),
    m_playerThread(new QThread()),
    m_preferences(new ConfigEditor(this)),
    m_selectEvent(true),
    m_rowChanged(false),
    m_filewatcher(new QFileSystemWatcher),
    m_scriptProperties(new QLabel(this))
{
    ui->setupUi(this);
    ui->tableWidget->setItemDelegateForColumn(COLUMN_TEXT, new EventTableDelegate());
    ui->tableWidget->installEventFilter(this);
    ui->speedFactor->installEventFilter(this);
    m_preferences->installEventFilter(this);
    connect(ui->tableWidget->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(disableEventSelection()));

    m_player->moveToThread(m_playerThread);
    m_playerThread->start();
    connect(m_player, SIGNAL(pulse(qint64)), this, SLOT(playPulse(qint64)));
    connect(m_player, SIGNAL(changed()), this, SLOT(eventChanged()));
    connect(m_player, SIGNAL(changed()), this, SLOT(disableActionNext()));

    connect(ui->actionAddDelay, SIGNAL(triggered()), m_player, SLOT(addDelay()));
    connect(ui->actionSubDelay, SIGNAL(triggered()), m_player, SLOT(subDelay()));
    connect(ui->enableSpeedFactor, SIGNAL(toggled(bool)), m_player, SLOT(enableSpeedFactor(bool)));
    connect(ui->speedFactor, SIGNAL(valueChanged(double)), m_player, SLOT(setSpeedFactor(double)));

    setState(NODATA);
    ui->tableWidget->setFocus();
    setAcceptDrops(true);

    // Add preferences dock
    m_preferences->setVisible(false);
    ui->mainLayout->addWidget(m_preferences);
    ui->statusBar->addPermanentWidget(m_scriptProperties);

    // Selection timer (disables event highlighting for a while)
    m_timerSelection.setSingleShot(true);
    m_timerSelection.setInterval(1000);
    connect(&m_timerSelection, SIGNAL(timeout()), this, SLOT(enableEventSelection()));

    // Action Next timer (disables next action for a while)
    m_timerNext.setSingleShot(true);
    m_timerNext.setInterval(300);
    connect(&m_timerNext, SIGNAL(timeout()), this, SLOT(enableActionNext()));

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
    if (m_script) delete m_script;
    delete m_player;
    delete m_playerThread;
    delete m_preferences;
    delete m_filewatcher;
    delete m_scriptProperties;
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
        QList<QUrl> urlList = mimeData->urls();
        // extract the local paths of the files
        for (int i = 0; i < urlList.size() && i < 32; ++i) {
            QString fileName = urlList.at(i).toLocalFile();
            QFileInfo fileInfo = QFileInfo(fileName);
            if(fileInfo.isFile() && fileInfo.isReadable() &&
                    (fileInfo.suffix().toLower() == "ass" ||
                     fileInfo.suffix().toLower() == "srt")) {
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
    settings.setValue("durationCorrection", ui->actionDurationCorrection->isChecked());
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
    QPoint pos(100, 100);
    move(settings.value("pos", pos).toPoint());
    m_preferences->reset();

    m_reloadEnabled = settings.value("reloadEnabled", false).toBool();
    ui->actionEnableReload->setChecked(m_reloadEnabled);
    m_autoHideEnabled = settings.value("autoHideEnabled", false).toBool();
    ui->actionAutoHideEnded->setChecked(m_autoHideEnabled);
    ui->actionPreferences->setChecked(settings.value("showPreferences", false).toBool());
    ui->actionDurationCorrection->setChecked(settings.value("durationCorrection", false).toBool());
    settings.endGroup();
}

const ConfigEditor* MainWindow::configEditor()
{
    return m_preferences;
}

const Player* MainWindow::player()
{
    return m_player;
}

void MainWindow::actionShowCalibration(bool p_state)
{
    if (p_state) {
        if (m_script)
            m_lastScript = m_script->fileName();
        openFile(":/samples/M.ass");
        jumpToSubtitle(0);
        m_timerAutoHide.stop(); // disable auto-hide for calibration
        actionToggleHide(false);
    }
    else {
        if (!m_lastScript.isEmpty())
            openFile(m_lastScript);
        else {
            closeFile();
        }
    }
}

void MainWindow::openFile (const QString &p_fileName)
{
    // Save on load file
    m_preferences->save();
    closeFile();

    // Create the script & setup the GUI
    m_script = new Script(p_fileName, this);
    m_player->setScript(m_script);
    m_preferences->setScript(m_script);  // will reset()
    // Set the window title from the file name, without extention
    setWindowTitle(QFileInfo(p_fileName).baseName());

    // Show script properties
    qlonglong count = m_script->eventsCount();
    if (count > 0) {
        QString firsttime = QTime().addMSecs(m_script->eventAt(0)->msseStart()).toString();
        QString lasttime = QTime().addMSecs(m_script->eventAt(count-1)->msseStart()).toString();
        m_scriptProperties->setText(tr("%1 events, %2 - %3").arg(count).arg(firsttime).arg(lasttime));
    }

    // Update the table
    ui->tableWidget->setRowCount(count);
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
    actionDurationCorrection(ui->actionDurationCorrection->isChecked());

	setState(STOPPED);

    // Watch file changes
    if (!p_fileName.startsWith(":"))
        m_filewatcher->addPath(p_fileName);
    // Reset search field
    ui->searchField->setEnabled(row > 0);
    ui->searchField->setText("");
    m_timerAutoHide.stop();
}

void MainWindow::closeFile()
{
    actionStop();
    setState(NODATA);

    // Clean-up previously allocated resources & reset GUI
    if(m_script != 0) {
        m_filewatcher->removePath(m_script->fileName());
        m_preferences->setScript(0);
        delete m_script;
        m_script = 0;
        m_player->setScript(m_script);
    }
    m_tableMapping.clear();

    setWindowTitle(tr("Subtivals"));
    m_scriptProperties->setText("");
    ui->tableWidget->setRowCount(0);
}

void MainWindow::refreshDurations()
{
    int row = 0;
    foreach(Event *event, m_script->events()) {
        QTableWidgetItem *endItem = ui->tableWidget->item(row, COLUMN_END);
        endItem->setText(QTime().addMSecs(event->msseEnd()).toString());
        QFont f = endItem->font();
        f.setItalic(event->isCorrected());
        endItem->setFont(f);
        row++;
    }
}

void MainWindow::actionDurationCorrection(bool state)
{
    if (!m_script)
        return;
    m_script->correctEventsDuration(state);
    refreshDurations();
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
    qint64 msseStartTime = m_player->elapsedTime();
    qint64 userDelay = m_player->delay();
    // Store current playing state
    State previous = m_state;

    // Reload file path
    openFile(QString(m_script->fileName())); //force copy
    statusBar()->showMessage(tr("Subtitle file reloaded."), 5000);

    // Restore position
    m_player->setElapsedTime(msseStartTime);
    m_player->addDelay(userDelay);
    // Restore state
    setState(previous);
}

void MainWindow::actionOpen()
{
    ui->actionShowCalibration->setChecked(false);
    // Ask the user for an subtitle file
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Open subtitles"),
                                                    m_lastFolder,
                                                    tr("Subtitle Files (*.ass *.srt)"));
    // Subtitle file selected ?
    if (!fileName.isEmpty()) {
        m_lastFolder = QFileInfo(fileName).absoluteDir().absolutePath();
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
        if (row >= 0) {
            jumpToSubtitle(row);
        }
        actionToggleHide(false);
        ui->actionDurationCorrection->setChecked(false);
        break;
    case PAUSED:
        setState(PLAYING);
        break;
    case NODATA:
    case PLAYING:
        break;
    }
}

void MainWindow::actionStop()
{
	setState(STOPPED);
    ui->timer->setText("-");
    ui->userDelay->setText("-");
}

bool MainWindow::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::FocusOut) {
        ui->tableWidget->clearSelection();
    }

    if (event->type() == QEvent::KeyRelease) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        QKeySequence keySequence(keyEvent->key());

        if (object == ui->speedFactor || object == m_preferences) {
            // If key pressed matches any of the main window actions,
            // trigger it ! (capture keys in all widgets with this filter)
            QList<QAction*> allActions = this->findChildren<QAction*>();
            foreach(QAction* action, allActions) {
                if (!action->shortcut().isEmpty() &&
                    action->shortcut().matches(keySequence) &&
                    action->isEnabled()) {
                    // Trigger action manually
                    action->trigger();
                }
            }
        }

        // With key Up/Down : behave the way single mouse clics do.
        if (object == ui->tableWidget) {
            if (keyEvent->key() == Qt::Key_Up || keyEvent->key() == Qt::Key_Down) {
                actionEventClic(ui->tableWidget->currentIndex());
            }
        }

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

void MainWindow::actionPause()
{
    setState(PAUSED);
}

bool MainWindow::canPrevious()
{
    return ui->tableWidget->currentRow() > 0;
}

void MainWindow::actionPrevious()
{
    if (canPrevious()) {
        int i = ui->tableWidget->currentRow();
        m_selectEvent = true;
        jumpToSubtitle(i - 1);
        ui->actionHide->setChecked(false);
    }
    ui->actionPrevious->setEnabled(canPrevious());
    ui->actionNext->setEnabled(canNext());
}

bool MainWindow::canNext()
{
    return !m_timerNext.isActive() && ui->tableWidget->currentRow() < ui->tableWidget->rowCount() - 1;
}

void MainWindow::actionNext()
{
    if (canNext()) {
        int row = ui->tableWidget->currentRow();
        bool isRowDisplayed = false;
        foreach(Event* e, m_player->current())
            if (m_tableMapping[e] == row)
                isRowDisplayed = true;

        // Jump next if selected is being viewed. Otherwise activate it.
        m_selectEvent = true;
        if (isRowDisplayed && !m_rowChanged)
            jumpToSubtitle(row + 1);
        else
            jumpToSubtitle(row);
        ui->actionHide->setChecked(false);
    }
    ui->actionPrevious->setEnabled(canPrevious());
    ui->actionNext->setEnabled(canNext());
}

void MainWindow::actionToggleHide(bool state)
{
    if(QObject::sender() != ui->actionHide)
        ui->actionHide->setChecked(state);
    highlightEvents(m_player->elapsedTime());
    emit toggleHide(state);
}

void MainWindow::disableEventSelection()
{
    // Disable selection of events for some time
    // in order to let the user perform a double-clic
    m_selectEvent = false;
    m_timerSelection.start(); // will timeout on enableEventSelection
}

void MainWindow::enableEventSelection()
{
    m_selectEvent = true;
}

void MainWindow::disableActionNext()
{
    // Disable next for a while
    ui->actionNext->setEnabled(false);
    m_timerNext.start();
}

void MainWindow::enableActionNext()
{
    ui->actionNext->setEnabled(canNext());
}

void MainWindow::actionEventClic(QModelIndex index)
{
    disableEventSelection();
    if (index.row() != ui->tableWidget->currentRow())
        m_rowChanged = true;
}

void MainWindow::actionEventSelected(QModelIndex index)
{
    // Switch to the selected event
    jumpToSubtitle(index.row());
    // Update the UI
    ui->actionHide->setChecked(false);
    ui->actionPrevious->setEnabled(canPrevious());
    ui->actionNext->setEnabled(canNext());
}

void MainWindow::jumpToSubtitle(int row)
{
    m_player->jumpTo(row);
    switch (m_state) {
    case PLAYING:
        break;
    case PAUSED:
    case STOPPED:
        if (m_autoHideEnabled) {
            const Event* event = m_script->eventAt(row);
            qint64 d = event->duration();
            if (event->autoDuration() > d)
                d = event->autoDuration();
            m_timerAutoHide.setInterval(d);
            m_timerAutoHide.start();
        }
        break;
    case NODATA:
        break;
    }
}

void MainWindow::playPulse(qint64 msecsElapsed)
{
    ui->timer->setText(ts2tc(msecsElapsed));
    ui->userDelay->setText(ts2tc(m_player->delay()));
}

void MainWindow::eventChanged()
{
    qint64 msecsElapsed = m_player->elapsedTime();
    highlightEvents(msecsElapsed);
    if(m_selectEvent) {
        int eventRow = -1;
        QList<Event*> currentEvents = m_player->current();
        if (currentEvents.size() > 0) {
            eventRow = m_tableMapping[currentEvents.last()];
            int scrollRow = eventRow > 2 ? eventRow - 2 : 0;
            ui->tableWidget->scrollTo(ui->tableWidget->currentIndex().sibling(scrollRow, 0),
                                      QAbstractItemView::PositionAtTop);
        }
        if (ui->tableWidget->hasFocus())
            ui->tableWidget->selectRow(eventRow);
    }
    m_rowChanged = false;
}

void MainWindow::highlightEvents(qlonglong elapsed)
{
    QColor off = qApp->palette().color(QPalette::Base);
    QColor on = qApp->palette().color(QPalette::Highlight).lighter(120);
    QColor next = qApp->palette().color(QPalette::Highlight).lighter(160);

    for(int row=0; row<ui->tableWidget->rowCount(); row++) {
        for (int col=0; col<ui->tableWidget->columnCount(); col++) {
            QTableWidgetItem* item = ui->tableWidget->item(row, col);
            item->setBackgroundColor(off);
            QFont f = item->font();
            f.setBold(false);
            item->setFont(f);
        }
    }

    foreach(Event *e, m_script->nextEvents(elapsed)) {
        int row = m_tableMapping[e];
        for (int col=0; col<ui->tableWidget->columnCount(); col++) {
            QTableWidgetItem* item = ui->tableWidget->item(row, col);
            item->setBackgroundColor(next);
        }
    }

    foreach(Event *e, m_player->current()) {
        int row = m_tableMapping[e];
        for (int col=0; col<ui->tableWidget->columnCount(); col++) {
            QTableWidgetItem* item = ui->tableWidget->item(row, col);
            if (!ui->actionHide->isChecked()) {
                QFont f = item->font();
                f.setBold(true);
                item->setFont(f);
            }
            item->setBackgroundColor(on);
        }
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
        m_player->stop();
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
        m_player->play();
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
        m_player->pause();
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
                          "<p>Subtivals, a program to project subtitles.</p>"
                          "<h2>Authors</h2>"
                          "<li>Lilian Lefranc</li>"
                          "<li>Arnaud Rolly</li>"
                          "<li>Mathieu Leplatre</li>"
                          "<li>Emmanuel Digiaro</li>"));
}

void MainWindow::actionShowHelp()
{
    QDesktopServices::openUrl(QUrl("https://github.com/traxtech/subtivals/wiki/User-Manual"));
}
