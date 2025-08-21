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
#include <QtCore/QTextStream>
#include <QtCore/QThread>
#include <QtCore/QUrl>
#include <QtCore/QtGlobal>

#include <QDesktopServices>
#include <QFile>
#include <QFileDialog>
#include <QFileSystemWatcher>
#include <QKeyEvent>
#include <QMessageBox>
#include <QMimeData>
#include <QPainter>
#include <QScrollBar>
#include <QSettings>
#include <QStyle>
#include <QStyledItemDelegate>
#include <QStringDecoder>
#include <QThread>
#include <QWidget>
#include <QStyleHints>
#include <QStyleFactory>
#include <QAbstractTextDocumentLayout>

#include "configeditor.h"
#include "mainwindow.h"
#include "player.h"
#include "shortcuteditor.h"
#include "remoteoptionsdialog.h"
#include "ui_mainwindow.h"
#include "wizard.h"

static QColor bestContrastingTextColor(const QColor &bg) {
  auto channel = [](double c) {
    c /= 255.0;
    return (c <= 0.03928) ? (c / 12.92) : std::pow((c + 0.055) / 1.055, 2.4);
  };

  double r = channel(bg.red());
  double g = channel(bg.green());
  double b = channel(bg.blue());
  double luminance = 0.2126 * r + 0.7152 * g + 0.0722 * b;

  double contrastBlack = (luminance + 0.05) / 0.05;
  double contrastWhite = (1.05) / (luminance + 0.05);

  return (contrastBlack > contrastWhite) ? QColor(Qt::black)
                                         : QColor(Qt::white);
}

/**
 * A small delegate class to allow rich text rendering in main table cells.
 */
class SubtitleTextDelegate : public QStyledItemDelegate {
  void paint(QPainter *painter, const QStyleOptionViewItem &option,
             const QModelIndex &index) const override {
    QTextDocument document;
    QVariant value = index.data(Qt::DisplayRole);

    // Decide text color (respecting foreground role)
    QColor textColor;
    if (option.state & QStyle::State_Selected) {
      textColor = option.palette.color(QPalette::HighlightedText);
    } else {
      QVariant fg = index.data(Qt::ForegroundRole);
      if (fg.isValid() && fg.canConvert<QColor>()) {
        textColor = fg.value<QColor>();
      } else {
        textColor = option.palette.color(QPalette::Text);
      }
    }

    // Draw background only
    QStyleOptionViewItem opt(option);
    initStyleOption(&opt, index);
    opt.text = QString(); // remove default text drawing

    if (opt.widget)
      opt.widget->style()->drawControl(QStyle::CE_ItemViewItem, &opt, painter,
                                       opt.widget);

    // Render rich text
    if (value.isValid() && !value.isNull()) {
      QString html = value.toString();
      // Show current subtitle with bold font.
      if (index.data(Qt::UserRole).toBool()) {
        html = QString("<b>%2</b>").arg(html);
      }
      document.setHtml(QString("<span style='color:%1;'>%2</span>")
                           .arg(textColor.name(), html));

      QFont font = document.defaultFont();
      font.setPointSizeF(font.pointSizeF() * m_zoomFactor);
      document.setDefaultFont(font);

      painter->save();
      // Center text vertically in the cell.
      QAbstractTextDocumentLayout *layout = document.documentLayout();
      QSizeF docSize = layout->documentSize();
      QPointF paintPos(option.rect.left(),
                       option.rect.top() +
                           (option.rect.height() - docSize.height()) / 2.0);
      painter->translate(paintPos);
      document.drawContents(painter);
      painter->restore();
    }

    // Show icon if too many chars per sec.
    if (!index.data(Qt::DecorationRole).isNull()) {
      QPixmap icon(index.data(Qt::DecorationRole).toString());
      opt.widget->style()->drawItemPixmap(
          painter, option.rect.translated(QPoint(-5, 0)),
          Qt::AlignRight | Qt::AlignVCenter, icon);
    }
  }

public:
  void setZoomFactor(qreal factor) { m_zoomFactor = factor; }

private:
  qreal m_zoomFactor = 1.0;
};

class SubtitleDurationDelegate : public QStyledItemDelegate {
  void paint(QPainter *painter, const QStyleOptionViewItem &option,
             const QModelIndex &index) const override {
    QStyledItemDelegate::paint(painter, option, index);
    qreal progression = index.data(Qt::UserRole).toReal();
    if (progression == 0.0)
      return;

    // Paint progression of subtitle
    painter->save();
    QPen pen(option.palette.highlightedText().color());
    pen.setWidth(3);

    QPoint startLine(option.rect.bottomLeft());
    startLine.setX(startLine.x() + int(option.rect.width() * progression) - 1);
    QPoint endLine = option.rect.bottomRight();

    if (progression < 0) {
      pen.setColor(option.palette.text().color());
      startLine = option.rect.bottomLeft();
      endLine = option.rect.bottomRight();
      endLine.setX(endLine.x() + int(option.rect.width() * progression) + 1);
    }

    painter->setPen(pen);
    painter->drawLine(startLine, endLine);
    painter->restore();
  }
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), m_script(nullptr),
      m_player(new Player()), m_playerThread(new QThread()),
      m_preferences(new ConfigEditor(this)),
      m_previewpanel(new SubtitlesForm(this)),
      m_shortcutEditor(new ShortcutEditor(this)),
      m_remoteOptionsDialog(new RemoteOptionsDialog(this)),
      m_selectSubtitle(true), m_rowChanged(false), m_reloadEnabled(false),
      m_filewatcher(new QFileSystemWatcher),
      m_scriptProperties(new QLabel(this)), m_countDown(new QLabel(this)),
      m_windowShown(false) {
  ui->setupUi(this);
  m_defaultPalette = qApp->palette();

  // Precompute contrasting text colors
  QColor offColor = m_defaultPalette.color(QPalette::Base);
  QColor onColor = m_defaultPalette.color(QPalette::Highlight).lighter(130);
  QColor nextColor = m_defaultPalette.color(QPalette::Highlight).lighter(170);
  m_itemColorOffBackground = QBrush(offColor);
  m_itemColorOnBackground = QBrush(onColor);
  m_itemColorNextBackground = QBrush(nextColor);
  m_itemColorOffText = bestContrastingTextColor(offColor);
  m_itemColorOnText = bestContrastingTextColor(onColor);
  m_itemColorNextText = bestContrastingTextColor(nextColor);

  ui->tableWidget->setItemDelegateForColumn(COLUMN_START,
                                            new SubtitleDurationDelegate());
  ui->tableWidget->setItemDelegateForColumn(COLUMN_END,
                                            new SubtitleDurationDelegate());
  ui->tableWidget->setItemDelegateForColumn(COLUMN_TEXT,
                                            new SubtitleTextDelegate());

  ui->tableWidget->hideColumn(COLUMN_COMMENTS);

  ui->tableWidget->installEventFilter(this);
  ui->speedFactor->installEventFilter(this);
  m_previewpanel->installEventFilter(this);
  m_preferences->installEventFilter(this);
  connect(ui->tableWidget->verticalScrollBar(), SIGNAL(valueChanged(int)), this,
          SLOT(disableSubtitleSelection()));

  m_player->moveToThread(m_playerThread);
  m_playerThread->start();
  qRegisterMetaType<QList<Subtitle *>>("QList<Subtitle*>");
  connect(m_player, SIGNAL(pulse(int)), this, SLOT(playPulse(int)));
  connect(m_player, SIGNAL(changed(QList<Subtitle *>)), this,
          SLOT(subtitleChanged(QList<Subtitle *>)));
  connect(m_player, SIGNAL(changed(QList<Subtitle *>)), this,
          SLOT(disableActionNext()));
  connect(m_player, SIGNAL(autoHide()), this, SLOT(actionToggleHide()));

  connect(ui->actionHideDesktop, SIGNAL(toggled(bool)), this,
          SIGNAL(hideDesktop(bool)));
  connect(ui->actionAddDelay, SIGNAL(triggered()), m_player, SLOT(addDelay()));
  connect(ui->actionSubDelay, SIGNAL(triggered()), m_player, SLOT(subDelay()));
  connect(ui->enableSpeedFactor, SIGNAL(toggled(bool)), m_player,
          SLOT(enableSpeedFactor(bool)));
  connect(ui->speedFactor, SIGNAL(valueChanged(double)), m_player,
          SLOT(setSpeedFactor(double)));

  // Prevent hiding desktop if only one screen!
  ui->actionHideDesktop->setEnabled(QGuiApplication::screens().size() > 1);

  setState(NODATA);
  ui->tableWidget->setFocus();
  setAcceptDrops(true);

  // Adjust the timer width based on font.
  int pixelsWide = ui->timer->fontMetrics().horizontalAdvance("+44:44:44.444");
  ui->timer->setMinimumWidth(pixelsWide + 5);

  // Adjust the known factors minimum height to neighbour
  ui->knownFactors->setMinimumHeight(ui->speedFactor->height() - 6);

  // Build the list of known factors.
  ui->knownFactors->addItem("", 100.0);
  foreach (Factor conv, FACTORS_VALUES) {
    ui->knownFactors->addItem(
        QString("%1 fps → %2 fps").arg(conv.first).arg(conv.second),
        conv.first * 100.0 / conv.second);
  }

  // Disable print out by default.
  ui->actionOperatorPrintout->setEnabled(false);

  // Preview panel
  m_previewpanel->setVisible(false);
  m_previewpanel->setMinimumHeight(50);
  this->connectProjectionEvents(m_previewpanel);
  ui->tableAndPreview->replaceWidget(1, m_previewpanel);

  // Add preferences dock
  m_preferences->setVisible(false);
  ui->mainLayout->addWidget(m_preferences);
  ui->statusBar->addPermanentWidget(m_countDown);
  ui->statusBar->addPermanentWidget(m_scriptProperties);
  connect(m_preferences, SIGNAL(styleOverriden(bool)), this,
          SLOT(showStyleOverriden(bool)));

  // Selection timer (disables subtitle highlighting for a while)
  m_timerSelection.setSingleShot(true);
  m_timerSelection.setInterval(1000);
  connect(&m_timerSelection, SIGNAL(timeout()), this,
          SLOT(enableSubtitleSelection()));

  // Action Next timer (disables next action for a while)
  m_timerNext.setSingleShot(true);
  m_timerNext.setInterval(300);
  connect(&m_timerNext, SIGNAL(timeout()), this, SLOT(enableActionNext()));

  // Script file watching :
  connect(m_filewatcher, SIGNAL(fileChanged(QString)), this,
          SLOT(fileChanged(QString)));
  m_timerFileChange.setSingleShot(true);
  connect(&m_timerFileChange, SIGNAL(timeout()), this, SLOT(reloadScript()));

  // Initialize recent files actions.
  for (int i = 0; i < MAX_RECENT_FILES; ++i) {
    QAction *action = new QAction(this);
    action->setVisible(false);
    connect(action, SIGNAL(triggered()), this, SLOT(openRecentFile()));
    m_recentFileActions.insert(i, action);
    ui->menuFile->insertAction(ui->actionExit, action);
  }
  ui->menuFile->insertSeparator(ui->actionExit);
  updateRecentFileActions();

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
  m_shortcutEditor->registerAction(ui->actionShowPreview);
  m_shortcutEditor->registerAction(ui->actionEnableReload);
  m_shortcutEditor->registerAction(ui->actionShowHelp);
  m_shortcutEditor->registerAction(ui->actionDarkMode);
  m_shortcutEditor->registerAction(ui->actionExit);
}

MainWindow::~MainWindow() {
  m_playerThread->quit();
  m_playerThread->wait();
  delete ui;
  if (m_script)
    delete m_script;
  delete m_player;
  delete m_playerThread;
  delete m_preferences;
  delete m_previewpanel;
  delete m_filewatcher;
  delete m_scriptProperties;
  delete m_countDown;
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
  event->acceptProposedAction();
}

void MainWindow::dragMoveEvent(QDragMoveEvent *event) {
  event->acceptProposedAction();
}

void MainWindow::dragLeaveEvent(QDragLeaveEvent *event) { event->accept(); }

void MainWindow::dropEvent(QDropEvent *event) {
  const QMimeData *mimeData = event->mimeData();
  // check for our needed mime type, here a file or a list of files
  if (mimeData->hasUrls()) {
    QList<QUrl> urlList = mimeData->urls();
    // extract the local paths of the files
    for (int i = 0; i < urlList.size() && i < 32; ++i) {
      QString fileName = urlList.at(i).toLocalFile();
      QFileInfo fileInfo = QFileInfo(fileName);
      if (fileInfo.isFile() && fileInfo.isReadable() &&
          (fileInfo.suffix().toLower() == "ass" ||
           fileInfo.suffix().toLower() == "srt" ||
           fileInfo.suffix().toLower() == "txt")) {
        openFile(fileName);
        return;
      }
    }
  }
}

void MainWindow::closeEvent(QCloseEvent *) {
  setState(STOPPED);
  // Save settings
  QSettings settings;
  settings.beginGroup("MainWindow");
  settings.setValue("darkMode", ui->actionDarkMode->isChecked());
  settings.setValue("lastFolder", m_lastFolder);
  settings.setValue("size", size());
  settings.setValue("pos", pos());
  settings.setValue("reloadEnabled", m_reloadEnabled);
  settings.setValue("autoHideEnabled", m_player->isAutoHideEnabled());
  settings.setValue("showPreferences", ui->actionPreferences->isChecked());
  settings.setValue("showPreview", ui->actionShowPreview->isChecked());
  settings.setValue("durationCorrection",
                    ui->actionDurationCorrection->isChecked());
  settings.setValue("showMilliseconds",
                    ui->actionShowMilliseconds->isChecked());
  settings.setValue("persistentHide", ui->actionPersistentHide->isChecked());
  settings.setValue("largeTextPercent", m_largeTextPercent);
  settings.setValue("showLargeText", ui->actionLargeText->isChecked());
  settings.setValue("lockScreenOnPlay",
                    ui->actionLockScreenOnPlay->isChecked());
  settings.endGroup();

  settings.beginGroup("AdvancedOptions");
  settings.setValue("warnCharsRate", m_warnCharsRate);
  settings.setValue("errorCharsRate", m_errorCharsRate);
  settings.setValue("charsRate", m_charsRate);
  settings.setValue("subtitleInterval", m_subtitleInterval);
  settings.setValue("subtitleMinDuration", m_subtitleMinDuration);
  settings.setValue("delayMilliseconds", m_delayMilliseconds);
  settings.endGroup();

  settings.beginGroup("PreviewPanel");
  QList<int> sizes = ui->tableAndPreview->sizes();
  settings.setValue("opacity", m_previewpanel->opacity());
  settings.setValue("tableHeight", sizes[0]);
  settings.setValue("previewHeight", sizes[1]);
  settings.endGroup();

  // When the main window is close : end of the app
  qApp->exit();
}

void MainWindow::showEvent(QShowEvent *) {
  if (m_windowShown) {
    // No need to restore settings on each show event.
    // (eg. when restoring window after minized state)
    return;
  }

  // Restore settings
  QSettings settings;

  settings.beginGroup("MainWindow");
  m_lastFolder = settings.value("lastFolder", "").toString();
  resize(settings.value("size", size()).toSize());

  // Do our best to avoid overlapping of black screen.
  QRect screenGeom = qApp->primaryScreen()->geometry();
  int decorationHeight = style()->pixelMetric(QStyle::PM_TitleBarHeight);
  int centerH = (screenGeom.width() - geometry().width()) / 2;
  QPoint pos(centerH,
             screenGeom.height() - geometry().height() - decorationHeight);
  move(settings.value("pos", pos).toPoint());

  m_preferences->reset();

  ui->actionDarkMode->setChecked(settings.value("darkMode", false).toBool());
  m_reloadEnabled = settings.value("reloadEnabled", false).toBool();
  ui->actionEnableReload->setChecked(m_reloadEnabled);
  bool autoHide = settings.value("autoHideEnabled", false).toBool();
  m_player->enableAutoHide(autoHide);
  ui->actionAutoHideEnded->setChecked(autoHide);
  ui->actionPreferences->setChecked(
      settings.value("showPreferences", true).toBool());
  bool showPreview = settings.value("showPreview", false).toBool();
  ui->actionShowPreview->setChecked(showPreview);
  ui->actionDurationCorrection->setChecked(
      settings.value("durationCorrection", false).toBool());
  ui->actionShowMilliseconds->setChecked(
      settings.value("showMilliseconds", false).toBool());
  ui->actionPersistentHide->setChecked(
      settings.value("persistentHide", false).toBool());
  m_largeTextPercent = settings.value("largeTextPercent", 150).toInt();
  ui->actionLargeText->setChecked(
      settings.value("showLargeText", false).toBool());
  ui->actionLockScreenOnPlay->setChecked(
      settings.value("lockScreenOnPlay", false).toBool());

  if (settings.value("wizard", true).toBool()) {
    ui->actionShowWizard->trigger();
    settings.setValue("wizard", false);
  }
  settings.endGroup();

  settings.beginGroup("AdvancedOptions");
  m_warnCharsRate = settings.value("warnCharsRate", 14).toInt();
  m_errorCharsRate = settings.value("warnCharsRate", 18).toInt();
  m_charsRate = settings.value("charsRate", 12).toInt();
  m_subtitleInterval = settings.value("subtitleInterval", 1000).toInt();
  m_subtitleMinDuration = settings.value("subtitleMinDuration", 1000).toInt();
  m_delayMilliseconds = settings.value("delayMilliseconds", 250).toInt();
  settings.endGroup();

  settings.beginGroup("PreviewPanel");
  QList<int> sizes;
  sizes.append(settings.value("tableHeight", 100).toInt());
  sizes.append(settings.value("previewHeight", 30).toInt());
  if (showPreview) {
    ui->tableAndPreview->setSizes(sizes);
  }
  m_previewpanel->opacity(settings.value("opacity", 0.7).toDouble());
  settings.endGroup();

  // Reflect the configured add/sub delay on the action text.
  QString text;
  switch (m_delayMilliseconds) {
  case 100:
    text = tr("1/10 sec");
    break;
  case 250:
    text = tr("1/4 sec");
    break;
  case 500:
    text = tr("1/2 sec");
    break;
  case 1000:
    text = tr("1 sec");
    break;
  default:
    text = QString::number(m_delayMilliseconds) + tr(" msec");
    break;
  }
  ui->actionAddDelay->setText("+" + text);
  ui->actionSubDelay->setText("-" + text);

  m_windowShown = true;
}

ConfigEditor *MainWindow::configEditor() { return m_preferences; }

const Player *MainWindow::player() { return m_player; }

const RemoteOptionsDialog *MainWindow::remoteOptionsDialog() {
  return m_remoteOptionsDialog;
}

void MainWindow::openRecentFile() {
  QAction *action = qobject_cast<QAction *>(sender());
  if (action) {
    openFile(action->data().toString());
  }
}

void MainWindow::actionShowWizard() {
  Wizard wizard;
  wizard.setPixmap(Wizard::LogoPixmap, QPixmap(":/icons/subtivals.svg"));
  wizard.setWizardStyle(QWizard::ClassicStyle);
  wizard.exec();
}

void MainWindow::actionOperatorPrintout() {
  QString fileName = QFileDialog::getSaveFileName(
      this, tr("Save list as spreadsheet"),
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
    QMessageBox::information(this, tr("Unable to write file"),
                             file.errorString());
    return;
  }

  QTextStream out(&file);
  out.setGenerateByteOrderMark(true);
  out << m_script->exportList(Script::CSV);
  file.close();
  QMessageBox::information(this, tr("Saved successfully"),
                           tr("Subtitles exported to <a href=\"%1\">%2</a>")
                               .arg(QUrl::fromLocalFile(fileName).toString())
                               .arg(fileName));
}

void MainWindow::actionShowCalibration(bool p_state) {
  if (p_state) {
    openFile(QString(":/samples/Calibration.ass"));
    m_player->jumpTo(0);
    m_player->enableAutoHide(false); // disable auto-hide for calibration
    actionToggleHide(false);
  } else {
    if (!m_lastScript.isEmpty())
      openFile(m_lastScript);
    else {
      closeFile();
    }
    m_player->enableAutoHide(
        ui->actionAutoHideEnded->isChecked()); // restore auto-hide
  }
}

void MainWindow::openFile(const QString &p_fileName) {
  // Save on load file
  m_preferences->save();
  closeFile();

  // Check file UTF-8 validity
  QFile file(p_fileName);
  if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    QByteArray byteArray = file.readAll();

    QStringDecoder decoder(QStringDecoder::Utf8);
    QString decoded = decoder(byteArray);

    // Check for decoding errors
    if (decoder.hasError()) {
      QMessageBox::warning(
          this, tr("Encoding error"),
          tr("Looks like the subtitles were not saved in a valid UTF-8 file."));
      return;
    }
  } else {
    QMessageBox::warning(this, tr("Error with subtitles"),
                         tr("Could not read the file specified."));
    return;
  }

  // Keep track of last opened files.
  QSettings settings;
  QStringList files = settings.value("recentFileList").toStringList();
  files.removeAll(p_fileName);
  files.prepend(p_fileName);
  while (files.size() > MAX_RECENT_FILES) {
    files.removeLast();
  }
  settings.setValue("recentFileList", files);
  updateRecentFileActions();

  // Create the script & setup the GUI
  m_script = new Script(p_fileName, m_charsRate, m_subtitleInterval,
                        m_subtitleMinDuration, this);
  m_player->setScript(m_script);
  m_preferences->setScript(m_script); // will reset()
  // Set the window title from the file name, without extention
  setWindowTitle(QFileInfo(p_fileName).baseName());

  // Show script properties
  int count = m_script->subtitlesCount();
  if (count > 0) {
    QString firsttime = QTime(0, 0, 0)
                            .addMSecs(int(m_script->subtitleAt(0)->msseStart()))
                            .toString();
    QString lasttime =
        QTime(0, 0, 0)
            .addMSecs(int(m_script->subtitleAt(count - 1)->msseStart()))
            .toString();
    m_scriptProperties->setText(
        tr("%1 subtitles, %2 - %3").arg(count).arg(firsttime).arg(lasttime));
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
    QTableWidgetItem *styleItem =
        new QTableWidgetItem(subtitle->style()->name());
    ui->tableWidget->setItem(row, COLUMN_STYLE, styleItem);
    QTableWidgetItem *textItem = new QTableWidgetItem(subtitle->prettyText());
    ui->tableWidget->setItem(row, COLUMN_TEXT, textItem);
    QTableWidgetItem *commentsItem = new QTableWidgetItem(subtitle->comments());
    ui->tableWidget->setItem(row, COLUMN_COMMENTS, commentsItem);

    // Show chars/sec
    textItem->setToolTip(tr("%1 chars/sec").arg(subtitle->charsRate()));
    // Warn if too fast !
    if (subtitle->charsRate() > m_warnCharsRate) {
      QString icon(":/icons/chars-rate-warn.png");
      textItem->setToolTip(
          tr("Fast (%1 chars/sec)").arg(subtitle->charsRate()));
      if (subtitle->charsRate() > m_errorCharsRate) {
        icon = ":/icons/chars-rate-error.png";
        textItem->setToolTip(
            tr("Unreadable (%1 chars/sec)").arg(subtitle->charsRate()));
      }
      // This is drawn by the delegate.
      textItem->setData(Qt::DecorationRole, icon);
    }
    row++;
  }

  refreshDurations();
  ui->tableWidget->resizeColumnsToContents();

  // Refresh the state of the comments column
  actionConfig(m_preferences->isVisible());

  // File opened, enable print out.
  ui->actionOperatorPrintout->setEnabled(true);
  ui->actionJumpLongest->setEnabled(true);

  actionDurationCorrection(ui->actionDurationCorrection->isChecked());

  setState(STOPPED);

  // Watch file changes
  if (!p_fileName.startsWith(":"))
    m_filewatcher->addPath(p_fileName);
  // Reset search field
  ui->searchField->setEnabled(row > 0);
  ui->searchField->setText("");
}

void MainWindow::closeFile() {
  actionStop();
  setState(NODATA);

  // Clean-up previously allocated resources & reset GUI
  if (m_script != nullptr) {
    m_filewatcher->removePath(m_script->fileName());
    m_preferences->setScript(nullptr);
    delete m_script;
    m_script = nullptr;
    m_player->setScript(m_script);
  }
  m_tableMapping.clear();
  m_currentSubtitles.clear();
  // No file, disable print out.
  ui->actionOperatorPrintout->setEnabled(false);
  ui->actionJumpLongest->setEnabled(false);

  setWindowTitle(tr("Subtivals"));
  m_scriptProperties->setText("");
  ui->tableWidget->setRowCount(0);
}

void MainWindow::updateRecentFileActions() {
  QSettings settings;
  QStringList files = settings.value("recentFileList").toStringList();

  int numRecentFiles = qMin(files.size(), MAX_RECENT_FILES);
  for (int i = 0; i < numRecentFiles; ++i) {
    QString strippedName = QFileInfo(files[i]).fileName();
    QString text = tr("&%1 %2").arg(i + 1).arg(strippedName);
    m_recentFileActions[i]->setText(text);
    m_recentFileActions[i]->setData(files[i]);
    m_recentFileActions[i]->setVisible(true);
  }
  for (int j = numRecentFiles; j < MAX_RECENT_FILES; ++j) {
    m_recentFileActions[j]->setVisible(false);
  }
  // ui->separatorRecentFilesEnd->setVisible(numRecentFiles > 0);
}

void MainWindow::refreshDurations() {
  if (!m_script)
    return;

  QString format("hh:mm:ss");
  if (ui->actionShowMilliseconds->isChecked()) {
    format.append(".zzz");
  }
  int row = 0;
  foreach (Subtitle *subtitle, m_script->subtitles()) {
    QTableWidgetItem *startItem = ui->tableWidget->item(row, COLUMN_START);
    QTableWidgetItem *endItem = ui->tableWidget->item(row, COLUMN_END);
    QTime start = QTime(0, 0, 0).addMSecs(int(subtitle->msseStart()));
    QTime end = QTime(0, 0, 0).addMSecs(int(subtitle->msseEnd()));
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
  ui->tableWidget->setColumnWidth(
      COLUMN_START, int(1.1 * ui->tableWidget->columnWidth(COLUMN_START)));
  ui->tableWidget->setColumnWidth(
      COLUMN_END, int(1.1 * ui->tableWidget->columnWidth(COLUMN_END)));
}

void MainWindow::actionDurationCorrection(bool state) {
  if (!m_script)
    return;
  m_script->correctSubtitlesDuration(state);
  refreshDurations();
}

void MainWindow::actionEnableReload(bool state) {
  m_reloadEnabled = state;
  if (!state)
    m_timerFileChange.stop();
}

void MainWindow::actionAutoHideEnded(bool p_state) {
  m_player->enableAutoHide(p_state);
}

void MainWindow::showStyleOverriden(bool p_state) {
  QTableWidgetItem *item = new QTableWidgetItem();
  item->setText(ui->tableWidget->horizontalHeaderItem(COLUMN_STYLE)->text());
  if (p_state) {
    item->setIcon(QIcon(":/icons/important.svg"));
    item->setToolTip(tr("Some styles are currently overriden in preferences."));
  }
  ui->tableWidget->setHorizontalHeaderItem(COLUMN_STYLE, item);
}

void MainWindow::fileChanged(QString path) {
  // Script file is being modified.
  // Wait that no change is made during 1sec before warning the user.
  if (m_reloadEnabled && path == m_script->fileName() && QFile(path).exists())
    m_timerFileChange.start(1000);
  else
    m_timerFileChange.stop();
}

void MainWindow::reloadScript() {
  // Script file has changed, reload.
  // Store current position
  qint64 msseStartTime = m_player->elapsedTime();
  int userDelay = m_player->delay();
  QModelIndex currentIndex = ui->tableWidget->currentIndex();
  // Store current playing state
  State previous = m_state;

  // Reload file path
  openFile(QString(m_script->fileName())); // force copy
  statusBar()->showMessage(tr("Subtitle file reloaded."), 5000);

  // Restore position
  m_player->setElapsedTime(msseStartTime);
  m_player->addDelay(userDelay);

  // Restore state
  setState(previous);

  // Simulate double click on same row (if playing, don't disturb flow)
  if (previous != PLAYING) {
    if (currentIndex.row() > 0) {
      emit ui->tableWidget->doubleClicked(currentIndex);
    }
  }
}

void MainWindow::actionOpen() {
  ui->actionShowCalibration->setChecked(false);
  // Ask the user for an subtitle file
  QString fileName = QFileDialog::getOpenFileName(
      this, tr("Open subtitles"), m_lastFolder,
      tr("Subtitle Files (*.ass *.srt *.txt *.xml)"));
  // Subtitle file selected ?
  if (!fileName.isEmpty()) {
    m_lastFolder = QFileInfo(fileName).absoluteDir().absolutePath();
    openFile(fileName);
  }
}

void MainWindow::actionPlay() {
  int row = ui->tableWidget->currentRow();
  switch (m_state) {
  case STOPPED:
    setState(PLAYING);
    if (row < 0)
      row = 0; // Activate first subtitle on play
    m_player->jumpTo(row);
    actionToggleHide(false);
    ui->actionDurationCorrection->setChecked(false);
    // Projection window is resizable option disabled or single screen.
    emit screenResizable(!ui->actionLockScreenOnPlay->isChecked() ||
                         QGuiApplication::screens().size() < 2);
    m_preferences->enableTabScreen(!ui->actionLockScreenOnPlay->isChecked() ||
                                   QGuiApplication::screens().size() < 2);
    break;
  case PAUSED:
    setState(PLAYING);
    break;
  case NODATA:
  case PLAYING:
    break;
  }
}

void MainWindow::actionStop() {
  setState(STOPPED);
  highlightSubtitles(0);
  playPulse(0);
  ui->timer->setText("-");
  ui->userDelay->setText("-");
  m_countDown->setText("");
  // Projection window is resizable option disabled or single screen.
  emit screenResizable(true);
  m_preferences->enableTabScreen(true);
}

bool MainWindow::eventFilter(QObject *object, QEvent *event) {
  if (event->type() == QEvent::FocusOut) {
    ui->tableWidget->clearSelection();
  }

  if (event->type() == QEvent::KeyRelease) {
    QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
    QKeySequence keySequence(keyEvent->key());

    if (object == ui->speedFactor || object == m_preferences) {
      // If key pressed matches any of the main window actions,
      // trigger it ! (capture keys in all widgets with this filter)
      QList<QAction *> allActions = this->findChildren<QAction *>();
      // We treat of couple of actions differently below
      allActions.removeAll(ui->actionNext);
      allActions.removeAll(ui->actionSpeedUp);
      allActions.removeAll(ui->actionSlowDown);
      foreach (QAction *action, allActions) {
        if (!action->shortcut().isEmpty() &&
            action->shortcut().matches(keySequence) && action->isEnabled()) {
          // Trigger action manually
          action->trigger();
        }
      }
    }

    // With Space, behave like next()
    if (ui->actionNext->shortcut().matches(keySequence)) {
      actionNext(); // Allow to trigger the action, even if disabled (activate
                    // last row).
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

void MainWindow::actionConfig(bool state) {
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

  // Save when user hides it
  if (!state)
    m_preferences->save();
}

void MainWindow::connectProjectionEvents(SubtitlesForm *f) {
  if (f != m_previewpanel) {
    QObject::connect(f, SIGNAL(geometryChanged(int, QRect)), m_preferences,
                     SLOT(projectionWindowChanged(int, QRect)));
    QObject::connect(f, SIGNAL(geometryChanged(int, QRect)), m_previewpanel,
                     SLOT(changeGeometry(int, QRect)));
  }
  QObject::connect(m_player, SIGNAL(on(Subtitle *)), f,
                   SLOT(addSubtitle(Subtitle *)));
  QObject::connect(m_player, SIGNAL(off(Subtitle *)), f,
                   SLOT(remSubtitle(Subtitle *)));
  QObject::connect(m_player, SIGNAL(clear()), f, SLOT(clearSubtitles()),
                   Qt::DirectConnection);
  QObject::connect(this, SIGNAL(toggleHide(bool)), f, SLOT(toggleHide(bool)));

  QObject::connect(m_preferences, SIGNAL(changeScreen(int, QRect)), f,
                   SLOT(changeGeometry(int, QRect)));
  QObject::connect(m_preferences, SIGNAL(color(QColor)), f,
                   SLOT(color(QColor)));
  QObject::connect(m_preferences, SIGNAL(outline(QColor, int)), f,
                   SLOT(outline(QColor, int)));
  QObject::connect(m_preferences, SIGNAL(styleChanged()), f, SLOT(repaint()));
  QObject::connect(m_preferences, SIGNAL(rotate(double)), f,
                   SLOT(rotate(double)));
}

void MainWindow::actionShowPreview(bool state) {
  m_previewpanel->setVisible(state);
}

void MainWindow::actionPause() { setState(PAUSED); }

bool MainWindow::canPrevious() { return ui->tableWidget->currentRow() > 0; }

void MainWindow::actionPrevious() {
  if (canPrevious()) {
    int i = ui->tableWidget->currentRow();
    m_selectSubtitle = true;
    m_player->jumpTo(i - 1);
    if (!ui->actionPersistentHide->isChecked()) {
      ui->actionHide->setChecked(false);
    }
  }
  ui->actionPrevious->setEnabled(canPrevious());
  ui->actionNext->setEnabled(canNext());
}

bool MainWindow::canNext() {
  return !m_timerNext.isActive() &&
         ui->tableWidget->currentRow() < ui->tableWidget->rowCount() - 1;
}

void MainWindow::actionNext() {
  int row = ui->tableWidget->currentRow();
  if (row < 0) {
    // If no row selected, consider first one if no current subtitles
    row = m_currentSubtitles.size() > 0
              ? m_tableMapping[m_currentSubtitles.last()]
              : 0;
  }
  bool isRowDisplayed = false;
  foreach (Subtitle *e, m_currentSubtitles)
    if (m_tableMapping[e] == row)
      isRowDisplayed = true;

  // Jump next if selected is being viewed. Otherwise activate it.
  m_selectSubtitle = true;
  if (canNext() && isRowDisplayed && !m_rowChanged)
    m_player->jumpTo(row + 1);
  else
    m_player->jumpTo(row);
  // If hide is not persistent, then show text on jump/next.
  if (!ui->actionPersistentHide->isChecked()) {
    ui->actionHide->setChecked(false);
  }
  ui->actionPrevious->setEnabled(canPrevious());
  ui->actionNext->setEnabled(canNext());
}

void MainWindow::actionToggleHide(bool state) {
  if (QObject::sender() != ui->actionHide)
    ui->actionHide->setChecked(state);
  highlightSubtitles(m_player->elapsedTime());
  emit toggleHide(state);
}

void MainWindow::actionToggleLargeText(bool state) {
  qreal largeTextFactor = m_largeTextPercent / 100.0;

  SubtitleTextDelegate *textDelegate =
      (SubtitleTextDelegate *) ui->tableWidget->itemDelegateForColumn(
          COLUMN_TEXT);
  textDelegate->setZoomFactor(state ? largeTextFactor : 1.0);

  QFont font = ui->tableWidget->font();
  qreal zoomFactor = state ? largeTextFactor : 1 / largeTextFactor;
  font.setPointSizeF(font.pointSizeF() * zoomFactor);
  ui->tableWidget->setFont(font);

  // Recalculate and resize columns to fit new font size
  ui->tableWidget->resizeColumnsToContents();
  ui->tableWidget->resizeRowsToContents();
  ui->tableWidget->viewport()->update();
}

void MainWindow::actionToggleLockScreenOnPlay(bool state) {
  // If currently playing, then lock.
  if (state) {
    bool lock = m_state == PLAYING && QGuiApplication::screens().size() > 1;
    emit screenResizable(!lock);
    m_preferences->enableTabScreen(!lock);
  } else {
    emit screenResizable(true);
    m_preferences->enableTabScreen(true);
  }
}

void MainWindow::disableSubtitleSelection() {
  // Disable selection of subtitles for some time
  // in order to let the user perform a double-clic
  m_selectSubtitle = false;
  m_timerSelection.start(); // will timeout on enableSubtitleSelection
}

void MainWindow::enableSubtitleSelection() { m_selectSubtitle = true; }

void MainWindow::disableActionNext() {
  // Disable next for a while
  ui->actionNext->setEnabled(false);
  m_timerNext.start();
}

void MainWindow::enableActionNext() { ui->actionNext->setEnabled(canNext()); }

void MainWindow::actionSubtitleClic(QModelIndex index) {
  disableSubtitleSelection();
  // Keep track of row selection change
  QList<int> currentRows;
  foreach (Subtitle *e, m_currentSubtitles) {
    currentRows.append(m_tableMapping[e]);
  }
  if (!currentRows.contains(index.row())) {
    m_rowChanged = true;
  }
  ui->actionPrevious->setEnabled(canPrevious());
  ui->actionNext->setEnabled(canNext());
}

void MainWindow::actionSubtitleSelected(QModelIndex index) {
  // Switch to the selected subtitle.
  // Force select subtitle (double-clic is two clics)
  m_selectSubtitle = true;
  m_player->jumpTo(index.row());
  // Force subtitle change, since double-clic should always
  // last subtitle of current subtitles
  subtitleChanged(m_currentSubtitles);
  // Update the UI
  if (!ui->actionPersistentHide->isChecked()) {
    ui->actionHide->setChecked(false);
  }
  ui->actionPrevious->setEnabled(canPrevious());
  ui->actionNext->setEnabled(canNext());
}

void MainWindow::playPulse(int msecsElapsed) {
  if (m_state == PLAYING) {
    ui->timer->setText(ts2tc(msecsElapsed));
    ui->userDelay->setText(ts2tc(m_player->delay()));
    m_countDown->setText(
        tr("Remaining: %1")
            .arg(ts2tc(msecsElapsed - m_script->totalDuration(), "hh:mm:ss")));
  }

  if (!m_script)
    return;

  // Update progression of subtitles
  QList<Subtitle *> previousSubtitles =
      m_script->previousSubtitles(msecsElapsed);
  QList<Subtitle *> nextSubtitles = m_script->nextSubtitles(msecsElapsed);
  foreach (Subtitle *subtitle, m_script->subtitles()) {
    qreal progressionCurrent = 0.0;
    qreal progressionNext = 0.0;
    if (m_currentSubtitles.contains(subtitle)) {
      qreal remaining = (msecsElapsed - subtitle->msseStart()) /
                        qreal(m_player->duration(subtitle));
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
    ui->tableWidget->item(row, COLUMN_START)
        ->setData(Qt::UserRole, progressionNext);
    ui->tableWidget->item(row, COLUMN_END)
        ->setData(Qt::UserRole, progressionCurrent);
  }
}

void MainWindow::subtitleChanged(QList<Subtitle *> p_currentSubtitles) {
  m_currentSubtitles = p_currentSubtitles;
  qint64 msecsElapsed = m_player->elapsedTime();
  highlightSubtitles(msecsElapsed);
  if (m_selectSubtitle) {
    int subtitleRow = -1;
    if (m_currentSubtitles.size() > 0) {
      subtitleRow = m_tableMapping[m_currentSubtitles.last()];
    } else {
      QList<Subtitle *> nextSubtitles = m_script->nextSubtitles(msecsElapsed);
      if (nextSubtitles.size() > 0)
        subtitleRow = m_tableMapping[nextSubtitles.first()];
    }
    int scrollRow = subtitleRow > 2 ? subtitleRow - 2 : 0;
    ui->tableWidget->scrollTo(
        ui->tableWidget->currentIndex().sibling(scrollRow, 0),
        QAbstractItemView::PositionAtTop);

    QWidget *withFocus = qApp->focusWidget();
    ui->tableWidget->selectRow(subtitleRow);
    if (withFocus)
      withFocus->setFocus(); // restore
  }
  m_rowChanged = false;
}

void MainWindow::highlightSubtitles(qlonglong elapsed) {
  // Reset all
  for (int row = 0; row < ui->tableWidget->rowCount(); row++) {
    for (int col = 0; col < ui->tableWidget->columnCount(); col++) {
      QTableWidgetItem *item = ui->tableWidget->item(row, col);
      item->setBackground(m_itemColorOffBackground);
      item->setForeground(m_itemColorOffText);
      QFont f = item->font();
      f.setBold(false);
      item->setFont(f);
      if (col == COLUMN_TEXT)
        item->setData(Qt::UserRole, false);
    }
  }

  if (m_script) {
    // Highlight next subtitles
    foreach (Subtitle *e, m_script->nextSubtitles(elapsed)) {
      int row = m_tableMapping[e];
      for (int col = 0; col < ui->tableWidget->columnCount(); col++) {
        QTableWidgetItem *item = ui->tableWidget->item(row, col);
        item->setBackground(m_itemColorNextBackground);
        item->setForeground(m_itemColorNextText);
      }
    }

    // Highlight current subtitles
    foreach (Subtitle *e, m_currentSubtitles) {
      int row = m_tableMapping[e];
      for (int col = 0; col < ui->tableWidget->columnCount(); col++) {
        QTableWidgetItem *item = ui->tableWidget->item(row, col);
        if (!ui->actionHide->isChecked()) {
          QFont f = item->font();
          f.setBold(true);
          item->setFont(f);
        }
        item->setBackground(m_itemColorOnBackground);
        item->setForeground(m_itemColorOnText);
        if (col == COLUMN_TEXT)
          item->setData(Qt::UserRole, !ui->actionHide->isChecked());
      }
    }
  }
}

QString MainWindow::ts2tc(int p_ts, QString format) {
  if (p_ts >= 0)
    return "+" + QTime(0, 0, 0).addMSecs(p_ts).toString(format);
  else
    return "-" + QTime(0, 0, 0).addMSecs(-p_ts).toString(format);
}

void MainWindow::setState(State p_state) {
  m_state = p_state;
  switch (m_state) {
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

void MainWindow::search() {
  if (!m_script)
    return;

  QString search = ui->searchField->text();

  int found = -1;

  if (search.startsWith(":")) {
    found = search.replace(":", "").toInt() - 1;
  } else {
    // Loop over the whole list, start from current
    int nb = m_script->subtitlesCount();
    int i = ui->tableWidget->currentRow() + 1;
    int max = i + nb;
    for (; i < max; i++) {
      const Subtitle *e = m_script->subtitleAt(i % nb);
      if (e->text().contains(search, Qt::CaseInsensitive)) {
        found = i % nb;
        break;
      }
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

void MainWindow::searchTextChanged(QString) {
  ui->searchField->setStyleSheet("");
}

void MainWindow::actionJumpLongest() {
  if (m_script == nullptr) {
    return;
  }
  int longest = -1;
  for (int i = 0; i < m_script->subtitlesCount(); i++) {
    if (longest < 0 || m_script->subtitleAt(i)->charsWidth() >
                           m_script->subtitleAt(longest)->charsWidth()) {
      longest = i;
    }
  }
  if (longest > 0) {
    ui->tableWidget->selectRow(longest);
    ui->tableWidget->setFocus();
    actionSubtitleClic(QModelIndex());
  }
}

void MainWindow::actionEditShortcuts() { m_shortcutEditor->exec(); }

void MainWindow::actionShowRemoteOptions() { m_remoteOptionsDialog->show(); }

void MainWindow::actionShowMilliseconds(bool) { refreshDurations(); }

void MainWindow::actionAbout() {
  QMessageBox::about(this, tr("About Subtivals"),
                     tr("<h1>Subtivals %1</h1>"
                        "<p>Subtivals, a program to project subtitles.</p>"
                        "<h2>Authors</h2>"
                        "<li>Lilian Lefranc</li>"
                        "<li>Arnaud Rolly</li>"
                        "<li>Mathieu Leplatre</li>"
                        "<li>Emmanuel Digiaro</li>"
                        "<h2>© 2011 - %2</h2>")
                         .arg(VERSION)
                         .arg(QDate::currentDate().year()));
}

void MainWindow::actionShowHelp() {
  QDesktopServices::openUrl(QUrl("http://help.subtivals.org"));
}

void MainWindow::speedFactorChanged(double p_factor) {
  if (qAbs(p_factor - ui->knownFactors->currentData().toDouble()) > 0.001) {
    ui->knownFactors->setCurrentIndex(-1);
  }
}

void MainWindow::knownFactorChosen(int p_chosen) {
  // Adjust spinbox to known factor.
  if (p_chosen >= 0) {
    ui->speedFactor->setValue(ui->knownFactors->itemData(p_chosen).toDouble());
  }
}

void MainWindow::enableKnownFactors(bool p_state) {
  if (p_state) {
    ui->knownFactors->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  } else {
    ui->knownFactors->setSizeAdjustPolicy(
        QComboBox::AdjustToMinimumContentsLengthWithIcon);
  }
}

void MainWindow::actionAdvancedSettings() {
  QSettings settings;
  QString filename = settings.fileName();

  QMessageBox msgBox;
  msgBox.setText("Opening external editor...");
  msgBox.setInformativeText(
      QString("Will open configuration file at<br><a href=\"file://%1\">%2</a>")
          .arg(QUrl::fromLocalFile(filename).toString())
          .arg(filename));
  msgBox.exec();

  QDesktopServices::openUrl(QUrl::fromLocalFile(filename));
}

void MainWindow::actionToggleDarkMode(bool p_enabled) {
  if (p_enabled) {
    qApp->styleHints()->setColorScheme(Qt::ColorScheme::Dark);
  } else {
    // System default.
    qApp->styleHints()->setColorScheme(Qt::ColorScheme::Unknown);
  }
}
