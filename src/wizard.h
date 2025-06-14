#ifndef WIZARD_H
#define WIZARD_H

#include <QWizard>

namespace Ui {
  class Wizard;
}

class Wizard : public QWizard {
  Q_OBJECT

public:
  explicit Wizard(QWidget *parent = nullptr);
  ~Wizard();

private:
  Ui::Wizard *ui;
};

#endif // WIZARD_H
