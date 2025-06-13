#ifndef SHORTCUTEDITOR_H
#define SHORTCUTEDITOR_H

#include <QAction>
#include <QDialog>
#include <QWidget>

#define COLUMN_DESCRIPTION 0
#define COLUMN_SHORTCUT 1

class QTableWidgetItem;
class QAbstractButton;

namespace Ui {
  class ShortcutEditor;
}

class ShortcutEditor : public QDialog {
  Q_OBJECT

public:
  explicit ShortcutEditor(QWidget *parent = nullptr);
  ~ShortcutEditor();
  void registerAction(QAction *);
signals:
  void changed();
protected slots:
  void apply();
  void reset();
  void restore();
  void onClicked(QAbstractButton *);
  void recordAction(QTableWidgetItem *);
  void validateAction(QTableWidgetItem *);

private:
  Ui::ShortcutEditor *ui;
  QList<QAction *> m_actions;
  QHash<QAction *, QKeySequence> m_backup;
  QString m_oldAccelText;
};

#endif // SHORTCUTEDITOR_H
