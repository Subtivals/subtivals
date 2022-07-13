#include "wizard.h"
#include "ui_wizard.h"

Wizard::Wizard(QWidget *parent) : QWizard(parent), ui(new Ui::Wizard) {
  ui->setupUi(this);
  adjustSize();
}

Wizard::~Wizard() { delete ui; }
