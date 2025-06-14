#ifndef STYLEADVANCED_H
#define STYLEADVANCED_H

#include <QDialog>

class SubtitleStyle;

namespace Ui {
  class StyleAdvanced;
}

class StyleAdvanced : public QDialog {
  Q_OBJECT

public:
  explicit StyleAdvanced(SubtitleStyle *p_style, QWidget *parent = nullptr);
  ~StyleAdvanced();
signals:
  void styleChanged();
protected slots:
  void apply();

private:
  Ui::StyleAdvanced *ui;
  SubtitleStyle *m_style;
};

#endif // STYLEADVANCED_H
