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
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QFileSystemWatcher>
#include <QLabel>
#include <QList>
#include <QMainWindow>
#include <QMap>
#include <QModelIndex>
#include <QString>
#include <QTableWidget>
#include <QTime>
#include <QTimer>

#include "script.h"

#define COLUMN_START 0
#define COLUMN_END 1
#define COLUMN_STYLE 2
#define COLUMN_TEXT 3
#define COLUMN_COMMENTS 4

#define MAX_RECENT_FILES 7

static QList<double> FACTORS_VALUES = {23.976, 24, 25, 29.97, 30};
static QStringList FACTORS_LABELS = {"23,976 fps", "24 fps", "25 fps",
                                     "29,97 fps", "30 fps"};

class ConfigEditor;
class ShortcutEditor;
class Player;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow();
  enum State { NODATA, STOPPED, PLAYING, PAUSED };
  void openFile(const QString &p_fileName);
  void closeFile();
  ConfigEditor *configEditor();
  const Player *player();
signals:
  void toggleHide(bool state);
  void screenResizable(bool state);
public slots:
  void refreshDurations();
  void actionDurationCorrection(bool);
  void actionShowCalibration(bool);
  void actionOpen();
  void actionPlay();
  void actionStop();
  void actionConfig(bool);
  void actionPause();
  void actionNext();
  void actionPrevious();
  void actionToggleHide(bool state = true);
  void actionAbout();
  void actionShowHelp();
  void playPulse(int msecsElapsed);
  void subtitleChanged(QList<Subtitle *>);
  void actionEnableReload(bool);
  void fileChanged(QString path);
  void reloadScript();
  void actionSubtitleClic(QModelIndex);
  void actionSubtitleSelected(QModelIndex);
  void enableSubtitleSelection();
  void disableSubtitleSelection();
  void enableActionNext();
  void disableActionNext();
  void search();
  void searchTextChanged(QString);
  void actionAutoHideEnded(bool);
  void showStyleOverriden(bool);
  void actionEditShortcuts();
  void actionShowMilliseconds(bool);
  void actionShowWizard();
  void actionOperatorPrintout();
  void actionJumpLongest();
  void knownFactorChosen(int);
  void speedFactorChanged(double);
  void actionAdvancedSettings();
  void openRecentFile();

protected:
  bool canNext();
  bool canPrevious();
  bool eventFilter(QObject *, QEvent *);
  void showEvent(QShowEvent *);
  void dragEnterEvent(QDragEnterEvent *event);
  void dragMoveEvent(QDragMoveEvent *event);
  void dragLeaveEvent(QDragLeaveEvent *event);
  void dropEvent(QDropEvent *event);
  void closeEvent(QCloseEvent *);
  void highlightSubtitles(qlonglong elapsed);
  QString ts2tc(int p_timestamp, QString format = "hh:mm:ss.zzz");
  void setState(State p_state);
  void updateRecentFileActions();

private:
  State m_state;
  Ui::MainWindow *ui;
  Script *m_script;
  Player *m_player;
  QThread *m_playerThread;
  QString m_lastScript;
  QString m_lastFolder;
  ConfigEditor *m_preferences;
  ShortcutEditor *m_shortcutEditor;
  QMap<Subtitle *, int> m_tableMapping;
  bool m_selectSubtitle;
  QTimer m_timerSelection;
  QTimer m_timerNext;
  bool m_rowChanged;
  bool m_reloadEnabled;
  QFileSystemWatcher *m_filewatcher;
  QTimer m_timerFileChange;
  QLabel *m_scriptProperties;
  QLabel *m_countDown;
  QList<Subtitle *> m_currentSubtitles;
  QList<QAction *> m_recentFileActions;
  int m_warnCharsRate;
  int m_errorCharsRate;
  int m_charsRate;
  int m_subtitleInterval;
  int m_subtitleMinDuration;
};

#endif // MAINWINDOW_H
