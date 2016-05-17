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
#include <QtCore/QByteArray>
#include <QtCore/QTextCodec>
#include <QtCore/QFile>
#include <QtCore/QTextStream>

#include <QFileDialog>
#include <QDesktopWidget>
#include <QKeyEvent>
#include <QMessageBox>
#include <QDesktopServices>
#include <QScrollBar>
#include <QPainter>
#include <QMimeData>
#include <QStyledItemDelegate>
#include <QStyle>
#include <QDebug>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "configeditor.h"
#include "player.h"
#include "shortcuteditor.h"
#include "wizard.h"

/**
 * A small delegate class to allow rich text rendering in main table cells.
 */
class SubtitleTextDelegate : public QStyledItemDelegate
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
            QString html(value.toString());
            // Bold if selected
			if (index.data(Qt::UserRole).toBool())
				html = QString("<b>%1</b>").arg(html);
            document.setHtml(html);
            painter->translate(option.rect.topLeft());
            document.drawContents(painter);
            painter->translate(-option.rect.topLeft());
        }
        // Draw icon on the right
        if (!index.data(Qt::DecorationRole).isNull())
            options.widget->style()->drawItemPixmap(painter, option.rect.translated(QPoint(-5, 0)),
                                                    Qt::AlignRight|Qt::AlignVCenter,
                                                    QPixmap(index.data(Qt::DecorationRole).toString()));

    }
};

class SubtitleDurationDelegate : public QStyledItemDelegate
{
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        QStyledItemDelegate::paint(painter, option, index);
        // Progression is stored in cell user data
        qreal progression = index.data(Qt::UserRole).toReal();
        if (progression == 0.0)
            return;
        // Paint progression of subtitle
        painter->save();
        QPen pen(option.palette.highlightedText().color());
        pen.setWidth(3);

        QPoint startLine(option.rect.bottomLeft());
        startLine.setX(startLine.x() + option.rect.width() * progression -1);
        QPoint endLine = option.rect.bottomRight();
        if (progression < 0) {
            pen.setColor(option.palette.text().color());
            startLine = option.rect.bottomLeft();
            endLine = option.rect.bottomRight();
            endLine.setX(endLine.x() + option.rect.width() * progression +1);
        }
        painter->setPen(pen);
        painter->drawLine(startLine, endLine);
        painter->restore();
   }
};


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_script(0),
    m_player(new Player()),
    m_playerThread(new QThread()),
    m_preferences(new ConfigEditor(this)),
    m_shortcutEditor(new ShortcutEditor(this)),
    m_selectSubtitle(true),
    m_rowChanged(false),
    m_filewatcher(new QFileSystemWatcher),
    m_scriptProperties(new QLabel(this)),
    m_countDown(new QLabel(this)),
    logFile(new QFile("log_file.txt"))
{
    ui->setupUi(this);
    ui->tableWidget->setItemDelegateForColumn(COLUMN_START, new SubtitleDurationDelegate());
    ui->tableWidget->setItemDelegateForColumn(COLUMN_END, new SubtitleDurationDelegate());
    ui->tableWidget->setItemDelegateForColumn(COLUMN_TEXT, new SubtitleTextDelegate());
    ui->tableWidget->hideColumn(COLUMN_COMMENTS);

    ui->tableWidget->installEventFilter(this);
    ui->speedFactor->installEventFilter(this);
    m_preferences->installEventFilter(this);
    connect(ui->tableWidget->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(disableSubtitleSelection()));

    m_player->moveToThread(m_playerThread);
    m_playerThread->start();
    qRegisterMetaType<QList<Subtitle*> >("QList<Subtitle*>");
    connect(m_player, SIGNAL(pulse(qint64)), this, SLOT(playPulse(qint64)));
    connect(m_player, SIGNAL(changed(QList<Subtitle*>)), this, SLOT(subtitleChanged(QList<Subtitle*>)));
    connect(m_player, SIGNAL(changed(QList<Subtitle*>)), this, SLOT(disableActionNext()));
    connect(m_player, SIGNAL(autoHide()), this, SLOT(actionToggleHide()));

    connect(ui->actionAddDelay, SIGNAL(triggered()), m_player, SLOT(addDelay()));
    connect(ui->actionSubDelay, SIGNAL(triggered()), m_player, SLOT(subDelay()));
    connect(ui->enableSpeedFactor, SIGNAL(toggled(bool)), m_player, SLOT(enableSpeedFactor(bool)));
    connect(ui->speedFactor, SIGNAL(valueChanged(double)), m_player, SLOT(setSpeedFactor(double)));

    setState(NODATA);
    ui->tableWidget->setFocus();
    setAcceptDrops(true);

    // Disable print out by default.
    ui->actionOperatorPrintout->setEnabled(false);

    // Add preferences dock
    m_preferences->setVisible(false);
    ui->mainLayout->addWidget(m_preferences);
    ui->statusBar->addPermanentWidget(m_countDown);
    ui->statusBar->addPermanentWidget(m_scriptProperties);
    connect(m_preferences, SIGNAL(styleOverriden(bool)), this, SLOT(showStyleOverriden(bool)));

    // Selection timer (disables subtitle highlighting for a while)
    m_timerSelection.setSingleShot(true);
    m_timerSelection.setInterval(1000);
    connect(&m_timerSelection, SIGNAL(timeout()), this, SLOT(enableSubtitleSelection()));

    // Action Next timer (disables next action for a while)
    m_timerNext.setSingleShot(true);
    m_timerNext.setInterval(300);
    connect(&m_timerNext, SIGNAL(timeout()), this, SLOT(enableActionNext()));

    // Script file watching :
    connect(m_filewatcher, SIGNAL(fileChanged(QString)), this, SLOT(fileChanged(QString)));
    m_timerFileChange.setSingleShot(true);
    connect(&m_timerFileChange, SIGNAL(timeout()), this, SLOT(reloadScript()));

    // Initialize shortcuts
    m_shortcutEditor->registerAction(ui->actionPlay);
    m_shortcutEditor->registerAction(ui->actionPause);
    m_shortcutEditor->registerAction(ui->actionStop);
    m_shortcutEditor->registerAction(ui->actionPrevious);
    m_shortcutEditor->registerAction(ui->actionNext);
    m_shortcutEditor->registerAction(ui->actionHide);
    m_shortcutEditor->registerAction(ui->actionAddDelay);
    m_shortcutEditor->registerAction(ui->actionSubDelay);
    m_shortcutEditor->registerAction(ui->actionSpeedUp);
    m_shortcutEditor->registerAction(ui->actionSlowDown);
    m_shortcutEditor->registerAction(ui->actionAutoHideEnded);
    m_shortcutEditor->registerAction(ui->actionShowCalibration);
    m_shortcutEditor->registerAction(ui->actionPreferences);
    m_shortcutEditor->registerAction(ui->actionEnableReload);
    m_shortcutEditor->registerAction(ui->actionShowHelp);
    m_shortcutEditor->registerAction(ui->actionExit);
}

MainWindow::~MainWindow()
{
    m_playerThread->quit();
    m_playerThread->wait();
    delete ui;
    if (m_script) delete m_script;
    delete m_player;
    delete m_playerThread;
    delete m_preferences;
    delete m_filewatcher;
    delete m_scriptProperties;
    delete m_countDown;
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
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
                     fileInfo.suffix().toLower() == "srt" ||
                     fileInfo.suffix().toLower() == "txt")) {
                openFile(fileName);
                return;
            }
        }
    }
}

void MainWindow::closeEvent(QCloseEvent *)
{
    setState(STOPPED);
    // Save settings
    QSettings settings;
    settings.beginGroup("MainWindow");
    settings.setValue("lastFolder", m_lastFolder);
    settings.setValue("size", size());
    settings.setValue("pos", pos());
    settings.setValue("reloadEnabled", m_reloadEnabled);
    settings.setValue("autoHideEnabled", m_player->isAutoHideEnabled());
    settings.setValue("showPreferences", ui->actionPreferences->isChecked());
    settings.setValue("durationCorrection", ui->actionDurationCorrection->isChecked());
    settings.setValue("showMilliseconds", ui->actionShowMilliseconds->isChecked());
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

    // Place window at center, below black screen by default.
    QRect screenGeom = qApp->desktop()->screenGeometry();
    int center = (screenGeom.width() - geometry().width()) / 2;
    int decorationHeight = style()->pixelMetric(QStyle::PM_TitleBarHeight);
    QPoint pos(center, DEFAULT_HEIGHT + decorationHeight);
    move(settings.value("pos", pos).toPoint());

    m_preferences->reset();

    m_reloadEnabled = settings.value("reloadEnabled", false).toBool();
    ui->actionEnableReload->setChecked(m_reloadEnabled);
    bool autoHide = settings.value("autoHideEnabled", false).toBool();
    m_player->enableAutoHide(autoHide);
    ui->actionAutoHideEnded->setChecked(autoHide);
    ui->actionPreferences->setChecked(settings.value("showPreferences", true).toBool());
    ui->actionDurationCorrection->setChecked(settings.value("durationCorrection", false).toBool());
    ui->actionShowMilliseconds->setChecked(settings.value("showMilliseconds", false).toBool());

    if (settings.value("wizard", true).toBool()) {
        ui->actionShowWizard->trigger();
        settings.setValue("wizard", false);
    }

    settings.endGroup();
}

ConfigEditor* MainWindow::configEditor()
{
    return m_preferences;
}

const Player* MainWindow::player()
{
    return m_player;
}

void MainWindow::actionShowWizard()
{
    Wizard wizard;
    wizard.setPixmap(Wizard::LogoPixmap, QPixmap(":/icons/subtivals.svg"));
    wizard.setWizardStyle(QWizard::ClassicStyle);
    wizard.exec();
}

void MainWindow::actionOperatorPrintout()
{
    QString fileName = QFileDialog::getSaveFileName(this,
            tr("Save list as spreadsheet"),
            m_lastFolder + "/" + tr("spreadsheet") + ".csv",
            tr("Comma-separated files (*.csv)"));

    if (fileName.isEmpty()) {
        return;
    }
    if (!fileName.endsWith(".csv")) {
        fileName.append(".csv");
    }
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::information(this, tr("Unable to write file"), file.errorString());
        return;
    }

    QTextStream out(&file);
    out << m_script->exportList(Script::CSV);
    file.close();
    QMessageBox::information(this,
                             tr("Saved successfully"),
                             tr("Subtitles exported to <i>%1</i>").arg(fileName));

}

void MainWindow::actionShowCalibration(bool p_state)
{
    if (p_state) {
        QString fileExt = "ass";
        if (m_script) {
            m_lastScript = m_script->fileName();
            if (m_script->format() == Script::SRT) fileExt = "srt";
        }
        openFile(QString(":/samples/M.%1").arg(fileExt));
        m_player->jumpTo(0);
        m_player->enableAutoHide(false); // disable auto-hide for calibration
        actionToggleHide(false);
    }
    else {
        if (!m_lastScript.isEmpty())
            openFile(m_lastScript);
        else {
            closeFile();
        }
        m_player->enableAutoHide(ui->actionAutoHideEnded->isChecked()); // restore auto-hide
    }
}



void MainWindow::openFile (const QString &p_fileName)
{
    // Save on load file
    m_preferences->save();
    closeFile();

    // Check file UTF-8 validity
    QFile file(p_fileName);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QByteArray byteArray = file.readAll();
        QTextCodec::ConverterState state;
        QTextCodec *codec = QTextCodec::codecForName("UTF-8");
        codec->toUnicode(byteArray.constData(), byteArray.size(), &state);
        if (state.invalidChars > 0) {
            QMessageBox::warning(this,
                                 tr("Encoding error"),
                                 tr("Looks like the subtitles were not saved in a valid UTF-8 file."));
        }
    }
    else {
        QMessageBox::warning(this,
                             tr("Error with subtitles"),
                             tr("Could not read the file specified."));
    }

    // Create the script & setup the GUI
    m_script = new Script(p_fileName, this);
    m_player->setScript(m_script);
    m_preferences->setScript(m_script);  // will reset()
    // Set the window title from the file name, without extention
    setWindowTitle(QFileInfo(p_fileName).baseName());

    // Show script properties
    qlonglong count = m_script->subtitlesCount();
    if (count > 0) {
        QString firsttime = QTime(0, 0, 0).addMSecs(m_script->subtitleAt(0)->msseStart()).toString();
        QString lasttime = QTime(0, 0, 0).addMSecs(m_script->subtitleAt(count-1)->msseStart()).toString();
        m_scriptProperties->setText(tr("%1 subtitles, %2 - %3").arg(count).arg(firsttime).arg(lasttime));
    }

    // Update the table
    ui->tableWidget->setRowCount(count);
    QListIterator<Subtitle *> i = m_script->subtitles();
    int row = 0;
    while (i.hasNext()) {
        Subtitle *subtitle = i.next();
        m_tableMapping[subtitle] = row;
        QTableWidgetItem *startItem = new QTableWidgetItem("");
        ui->tableWidget->setItem(row, COLUMN_START, startItem);
        QTableWidgetItem *endItem = new QTableWidgetItem("");
        ui->tableWidget->setItem(row, COLUMN_END, endItem);
        QTableWidgetItem *styleItem = new QTableWidgetItem(subtitle->style()->name());
        ui->tableWidget->setItem(row, COLUMN_STYLE, styleItem);
        QTableWidgetItem *textItem = new QTableWidgetItem(subtitle->prettyText());
        ui->tableWidget->setItem(row, COLUMN_TEXT, textItem);
        QTableWidgetItem *commentsItem = new QTableWidgetItem(subtitle->comments());
        ui->tableWidget->setItem(row, COLUMN_COMMENTS, commentsItem);

        // Show chars/sec
        textItem->setToolTip(tr("%1 chars/sec").arg(subtitle->charsRate()));
        // Warn if too fast !
        if (subtitle->charsRate() > 14) {
            QString icon(":/icons/chars-rate-warn.png");
            textItem->setToolTip(tr("Fast (%1 chars/sec)").arg(subtitle->charsRate()));
            if (subtitle->charsRate() > 18) {
                icon = ":/icons/chars-rate-error.png";
                textItem->setToolTip(tr("Unreadable (%1 chars/sec)").arg(subtitle->charsRate()));
            }
            textItem->setData(Qt::DecorationRole, icon);
        }
        row++;
    }

    refreshDurations();

    // Refresh the state of the comments column
    actionConfig(m_preferences->isVisible());

    // File opened, enable print out.
    ui->actionOperatorPrintout->setEnabled(true);

    actionDurationCorrection(ui->actionDurationCorrection->isChecked());

	setState(STOPPED);

    // Watch file changes
    if (!p_fileName.startsWith(":"))
        m_filewatcher->addPath(p_fileName);
    // Reset search field
    ui->searchField->setEnabled(row > 0);
    ui->searchField->setText("");
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
    m_currentSubtitles.clear();
    // No file, disable print out.
    ui->actionOperatorPrintout->setEnabled(false);

    setWindowTitle(tr("Subtivals"));
    m_scriptProperties->setText("");
    ui->tableWidget->setRowCount(0);
}

void MainWindow::refreshDurations()
{
    if (!m_script)
        return;

    QString format("hh:mm:ss");
    if (ui->actionShowMilliseconds->isChecked()) {
        format.append(".zzz");
    }
    int row = 0;
    foreach(Subtitle *subtitle, m_script->subtitles()) {
        QTableWidgetItem *startItem = ui->tableWidget->item(row, COLUMN_START);
        QTableWidgetItem *endItem = ui->tableWidget->item(row, COLUMN_END);
        QTime start = QTime(0, 0, 0).addMSecs(subtitle->msseStart());
        QTime end = QTime(0, 0, 0).addMSecs(subtitle->msseEnd());
        startItem->setText(start.toString(format));
        endItem->setText(end.toString(format));
        // Distinct apparence if corrected
        QFont f = endItem->font();
        f.setItalic(subtitle->isCorrected());
        endItem->setFont(f);
        row++;
    }

    ui->tableWidget->resizeColumnToContents(COLUMN_START);
    ui->tableWidget->resizeColumnToContents(COLUMN_END);
    // Increase slightly columns widths to avoid ellipsing
    ui->tableWidget->setColumnWidth(COLUMN_START,
                                    1.1*ui->tableWidget->columnWidth(COLUMN_START));
    ui->tableWidget->setColumnWidth(COLUMN_END,
                                    1.1*ui->tableWidget->columnWidth(COLUMN_END));
}

void MainWindow::actionDurationCorrection(bool state)
{
    if (!m_script)
        return;
    m_script->correctSubtitlesDuration(state);
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
    m_player->enableAutoHide(p_state);
}

void MainWindow::showStyleOverriden(bool p_state)
{
    QTableWidgetItem* item = new QTableWidgetItem();
    item->setText(ui->tableWidget->horizontalHeaderItem(COLUMN_STYLE)->text());
    if (p_state) {
        item->setIcon(QIcon(":/icons/important.svg"));
        item->setToolTip(tr("Some styles are currently overriden in preferences."));
    }
    ui->tableWidget->setHorizontalHeaderItem(COLUMN_STYLE, item);
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
                                                    tr("Subtitle Files (*.ass *.srt *.txt *.xml)"));
    // Subtitle file selected ?
    if (!fileName.isEmpty()) {
        m_lastFolder = QFileInfo(fileName).absoluteDir().absolutePath();
        openFile(fileName);
        logFile->open(QIODevice::WriteOnly | QIODevice::Append);
        QTextStream ts(logFile);
        QDateTime current = QDateTime::currentDateTime();
        QString sub_info = QString("[%1] Opened file: %2\r\n").arg(current.toString("yyyy-MM-dd hh:mm:ss"), fileName);
        QString duration_info = QString("[%1] Total duration: %2\r\n").arg(current.toString("yyyy-MM-dd hh:mm:ss"), ts2tc(m_script->totalDuration(), "hh:mm:ss"));
        ts << sub_info << duration_info;
        logFile->close();
    }
}

void MainWindow::actionPlay()
{
    int row = ui->tableWidget->currentRow();
    switch(m_state) {
    case STOPPED:
        setState(PLAYING);
        if (row <0) row = 0;  // Activate first subtitle on play
        m_player->jumpTo(row);
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
    highlightSubtitles(0);
    playPulse(0);
    ui->timer->setText("-");
    ui->userDelay->setText("-");
    m_countDown->setText("");
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
            // We treat of couple of actions differently below
            allActions.removeAll(ui->actionNext);
            allActions.removeAll(ui->actionSpeedUp);
            allActions.removeAll(ui->actionSlowDown);
            foreach(QAction* action, allActions) {
                if (!action->shortcut().isEmpty() &&
                    action->shortcut().matches(keySequence) &&
                    action->isEnabled()) {
                    // Trigger action manually
                    action->trigger();
                }
            }
        }

        // With Space, behave like next()
        if (ui->actionNext->shortcut().matches(keySequence)) {
            actionNext();  // Allow to trigger the action, even if disabled (activate last row).
            event->accept();
            return true;
        }

        // With key Up/Down : behave the way single mouse clics do.
        if (object == ui->tableWidget) {
            if (keyEvent->key() == Qt::Key_Up || keyEvent->key() == Qt::Key_Down) {
                actionSubtitleClic(ui->tableWidget->currentIndex());
            }
        }

        if (keyEvent->key() == Qt::Key_F3) {
            search();
        }

        if (ui->enableSpeedFactor->isChecked()) {
            int factor = 10;
            if (keyEvent->modifiers().testFlag(Qt::ShiftModifier))
                factor = 1;
            if (ui->actionSpeedUp->shortcut().matches(keySequence))
                ui->speedFactor->stepBy(factor);
            if (ui->actionSlowDown->shortcut().matches(keySequence))
                ui->speedFactor->stepBy(-factor);
        }
    }
    return QMainWindow::eventFilter(object, event);
}

void MainWindow::actionConfig(bool state)
{
    // Hide comments when preferences is shown
    if (m_script && m_script->hasComments()) {
        if (state)
            ui->tableWidget->hideColumn(COLUMN_COMMENTS);
        else {
            ui->tableWidget->showColumn(COLUMN_COMMENTS);
        }
        ui->tableWidget->resizeColumnToContents(COLUMN_STYLE);
        ui->tableWidget->resizeColumnToContents(COLUMN_COMMENTS);
        ui->tableWidget->resizeColumnToContents(COLUMN_TEXT);
        ui->tableWidget->horizontalHeader()->setStretchLastSection(true);
    }
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
        m_selectSubtitle = true;
        m_player->jumpTo(i - 1);
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
    int row = ui->tableWidget->currentRow();
    if (row < 0) {
        // If no row selected, consider first one if no current subtitles
        row = m_currentSubtitles.size() > 0 ? m_tableMapping[m_currentSubtitles.last()] : 0;
    }
    bool isRowDisplayed = false;
    foreach(Subtitle* e, m_currentSubtitles)
        if (m_tableMapping[e] == row)
            isRowDisplayed = true;

    // Jump next if selected is being viewed. Otherwise activate it.
    m_selectSubtitle = true;
    if (canNext() && isRowDisplayed && !m_rowChanged)
        m_player->jumpTo(row + 1);
    else
        m_player->jumpTo(row);
    ui->actionHide->setChecked(false);
    ui->actionPrevious->setEnabled(canPrevious());
    ui->actionNext->setEnabled(canNext());
}

void MainWindow::actionToggleHide(bool state)
{
    if(QObject::sender() != ui->actionHide)
        ui->actionHide->setChecked(state);
    highlightSubtitles(m_player->elapsedTime());
    emit toggleHide(state);
}

void MainWindow::disableSubtitleSelection()
{
    // Disable selection of subtitles for some time
    // in order to let the user perform a double-clic
    m_selectSubtitle = false;
    m_timerSelection.start(); // will timeout on enableSubtitleSelection
}

void MainWindow::enableSubtitleSelection()
{
    m_selectSubtitle = true;
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

void MainWindow::actionSubtitleClic(QModelIndex index)
{
    disableSubtitleSelection();
    // Keep track of row selection change
    QList<int> currentRows;
    foreach(Subtitle *e, m_currentSubtitles) {
        currentRows.append(m_tableMapping[e]);
    }
    if (!currentRows.contains(index.row())) {
        m_rowChanged = true;
    }
    ui->actionPrevious->setEnabled(canPrevious());
    ui->actionNext->setEnabled(canNext());
}

void MainWindow::actionSubtitleSelected(QModelIndex index)
{
    // Switch to the selected subtitle.
    // Force select subtitle (double-clic is two clics)
    m_selectSubtitle = true;
    m_player->jumpTo(index.row());
    // Force subtitle change, since double-clic should always
    // last subtitle of current subtitles
    subtitleChanged(m_currentSubtitles);
    // Update the UI
    ui->actionHide->setChecked(false);
    ui->actionPrevious->setEnabled(canPrevious());
    ui->actionNext->setEnabled(canNext());
}

void MainWindow::playPulse(qint64 msecsElapsed)
{
    if (m_state == PLAYING) {
        ui->timer->setText(ts2tc(msecsElapsed));
        ui->userDelay->setText(ts2tc(m_player->delay()));
        m_countDown->setText(tr("Remaining: %1").arg(ts2tc(msecsElapsed - m_script->totalDuration(), "hh:mm:ss")));
    }

    if (!m_script) return;

    // Update progression of subtitles
    QList<Subtitle*> previousSubtitles = m_script->previousSubtitles(msecsElapsed);
    QList<Subtitle*> nextSubtitles = m_script->nextSubtitles(msecsElapsed);
    foreach(Subtitle* subtitle, m_script->subtitles()) {
        qreal progressionCurrent = 0.0;
        qreal progressionNext = 0.0;
        if (m_currentSubtitles.contains(subtitle)) {
            qreal remaining = (msecsElapsed - subtitle->msseStart()) / qreal(m_player->duration(subtitle));
            progressionCurrent = qBound(0.0, remaining, 1.0);
        }
        if (m_state == PLAYING) {
            if (m_currentSubtitles.isEmpty() && nextSubtitles.contains(subtitle)) {
                qint64 lastEnd = 0;
                if (!previousSubtitles.isEmpty())
                    lastEnd = previousSubtitles.last()->msseEnd();

                qint64 interval = subtitle->msseStart() - lastEnd;
                qreal missing = qreal(subtitle->msseStart() - msecsElapsed) / interval;
                progressionNext = -qBound(0.0, missing, 1.0);
            }
        }
        int row = m_tableMapping[subtitle];
        ui->tableWidget->item(row, COLUMN_START)->setData(Qt::UserRole, progressionNext);
        ui->tableWidget->item(row, COLUMN_END)->setData(Qt::UserRole, progressionCurrent);
    }
}

void MainWindow::subtitleChanged(QList<Subtitle*> p_currentSubtitles)
{
    m_currentSubtitles = p_currentSubtitles;
    qint64 msecsElapsed = m_player->elapsedTime();
    highlightSubtitles(msecsElapsed);
    if(m_selectSubtitle) {
        int subtitleRow = -1;
        if (m_currentSubtitles.size() > 0) {
            subtitleRow = m_tableMapping[m_currentSubtitles.last()];
        }
        else {
            QList<Subtitle*> nextSubtitles = m_script->nextSubtitles(msecsElapsed);
            if (nextSubtitles.size() > 0)
                subtitleRow = m_tableMapping[nextSubtitles.first()];
        }
        int scrollRow = subtitleRow > 2 ? subtitleRow - 2 : 0;
        ui->tableWidget->scrollTo(ui->tableWidget->currentIndex().sibling(scrollRow, 0),
                                  QAbstractItemView::PositionAtTop);

        QWidget* withFocus = qApp->focusWidget();
        ui->tableWidget->selectRow(subtitleRow);
        if (withFocus) withFocus->setFocus();  // restore

        logFile->open(QIODevice::WriteOnly | QIODevice::Append);
        QTextStream ts(logFile);
        QString remaining_info = tr("[%1] Remaining: %2\r\n").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"), ts2tc(m_script->totalDuration() - msecsElapsed, "hh:mm:ss"));
        ts << remaining_info;
        logFile->close();
    }
    m_rowChanged = false;
}

void MainWindow::highlightSubtitles(qlonglong elapsed)
{
    QColor off = qApp->palette().color(QPalette::Base);
    QColor on = qApp->palette().color(QPalette::Highlight).lighter(130);
    QColor next = qApp->palette().color(QPalette::Highlight).lighter(170);

    // First reset all
    for(int row=0; row<ui->tableWidget->rowCount(); row++) {
        for (int col=0; col<ui->tableWidget->columnCount(); col++) {
            QTableWidgetItem* item = ui->tableWidget->item(row, col);
            item->setBackgroundColor(off);
            QFont f = item->font();
            f.setBold(false);
            item->setFont(f);
            if (col == COLUMN_TEXT) item->setData(Qt::UserRole, false);
        }
    }

    // Then highlight next subtitles
    if (m_script) {
        foreach(Subtitle *e, m_script->nextSubtitles(elapsed)) {
            int row = m_tableMapping[e];
            for (int col=0; col<ui->tableWidget->columnCount(); col++) {
                QTableWidgetItem* item = ui->tableWidget->item(row, col);
                item->setBackgroundColor(next);
            }
        }
        // Finally highlight current subtitles
        foreach(Subtitle *e, m_currentSubtitles) {
            int row = m_tableMapping[e];
            for (int col=0; col<ui->tableWidget->columnCount(); col++) {
                QTableWidgetItem* item = ui->tableWidget->item(row, col);
                if (!ui->actionHide->isChecked()) {
                    QFont f = item->font();
                    f.setBold(true);
                    item->setFont(f);
                }
                item->setBackgroundColor(on);
                if (col == COLUMN_TEXT) item->setData(Qt::UserRole, !ui->actionHide->isChecked());
            }
        }
    }
}

QString MainWindow::ts2tc(qint64 p_ts, QString format)
{
    if (p_ts >= 0)
        return "+" + QTime(0, 0, 0).addMSecs(p_ts).toString(format);
    else
        return "-" + QTime(0, 0, 0).addMSecs(-p_ts).toString(format);
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
        ui->actionShowCalibration->setEnabled(true);
        ui->actionEditShortcuts->setEnabled(true);
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
        ui->actionShowCalibration->setEnabled(true);
        ui->actionEditShortcuts->setEnabled(true);
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
        ui->actionShowCalibration->setEnabled(false);
        ui->actionEditShortcuts->setEnabled(false);
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
    int nb = m_script->subtitlesCount();
    int i = ui->tableWidget->currentRow() + 1;
    int max = i + nb;
    for (; i < max; i++) {
        const Subtitle* e = m_script->subtitleAt(i % nb);
        if (e->text().contains(search, Qt::CaseInsensitive)) {
            found = i % nb;
            break;
        }
    }
    // Select the subtitle in the list
    if (found < 0) {
        ui->searchField->setStyleSheet("QLineEdit {color: red;}");
    } else {
        ui->tableWidget->selectRow(found);
        ui->tableWidget->setFocus();
        actionSubtitleClic(QModelIndex());
    }
}

void MainWindow::searchTextChanged(QString)
{
    ui->searchField->setStyleSheet("");
    ui->searchButton->setEnabled(!ui->searchField->text().isEmpty());
}

void MainWindow::actionEditShortcuts()
{
    m_shortcutEditor->exec();
}

void MainWindow::actionShowMilliseconds(bool)
{
    refreshDurations();
}

void MainWindow::actionAbout()
{
    QMessageBox::about(this,
                       tr("About Subtivals"),
                       tr("<h1>Subtivals %1</h1>"
                          "<p>Subtivals, a program to project subtitles.</p>"
                          "<h2>Authors</h2>"
                          "<li>Lilian Lefranc</li>"
                          "<li>Arnaud Rolly</li>"
                          "<li>Mathieu Leplatre</li>"
                          "<li>Emmanuel Digiaro</li>").arg(VERSION));
}

void MainWindow::actionShowHelp()
{
    QDesktopServices::openUrl(QUrl("http://help.subtivals.org"));
}
