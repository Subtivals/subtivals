#include "styleadvanced.h"
#include "ui_styleadvanced.h"

#include "script.h"
#include "style.h"

StyleAdvanced::StyleAdvanced(Style *p_style, QWidget *parent)
    : QDialog(parent), ui(new Ui::StyleAdvanced), m_style(p_style) {
  ui->setupUi(this);

  ui->title->setText(m_style->name());

  ui->lineSpacing->setValue(m_style->lineSpacing());
  if (m_style->alignment().testFlag(Qt::AlignTop))
    ui->verticalAlign->setCurrentIndex(0);
  else if (m_style->alignment().testFlag(Qt::AlignVCenter))
    ui->verticalAlign->setCurrentIndex(1);
  else if (m_style->alignment().testFlag(Qt::AlignBottom))
    ui->verticalAlign->setCurrentIndex(2);

  if (m_style->alignment().testFlag(Qt::AlignLeft))
    ui->horizontalAlign->setCurrentIndex(0);
  else if (m_style->alignment().testFlag(Qt::AlignHCenter))
    ui->horizontalAlign->setCurrentIndex(1);
  else if (m_style->alignment().testFlag(Qt::AlignRight))
    ui->horizontalAlign->setCurrentIndex(2);

  ui->marginL->setValue(m_style->marginL());
  ui->marginR->setValue(m_style->marginR());
  ui->marginV->setValue(m_style->marginV());

  connect(ui->lineSpacing, SIGNAL(valueChanged(double)), SLOT(apply()));
  connect(ui->verticalAlign, SIGNAL(currentIndexChanged(int)), SLOT(apply()));
  connect(ui->horizontalAlign, SIGNAL(currentIndexChanged(int)), SLOT(apply()));
  connect(ui->marginL, SIGNAL(valueChanged(int)), SLOT(apply()));
  connect(ui->marginR, SIGNAL(valueChanged(int)), SLOT(apply()));
  connect(ui->marginV, SIGNAL(valueChanged(int)), SLOT(apply()));
}

StyleAdvanced::~StyleAdvanced() { delete ui; }

void StyleAdvanced::apply() {
  m_style->setLineSpacing(ui->lineSpacing->value());

  Qt::Alignment vertical;
  switch (ui->verticalAlign->currentIndex()) {
  case 0:
    vertical = Qt::AlignTop;
    break;
  case 1:
    vertical = Qt::AlignVCenter;
    break;
  case 2:
    vertical = Qt::AlignBottom;
    break;
  default:
    break;
  }
  Qt::Alignment horizontal;
  switch (ui->horizontalAlign->currentIndex()) {
  case 0:
    horizontal = Qt::AlignLeft;
    break;
  case 1:
    horizontal = Qt::AlignHCenter;
    break;
  case 2:
    horizontal = Qt::AlignRight;
    break;
  default:
    break;
  }

  m_style->setAlignment(vertical | horizontal);
  m_style->setMargins(ui->marginL->value(), ui->marginR->value(),
                      ui->marginV->value());
  emit styleChanged();
}
