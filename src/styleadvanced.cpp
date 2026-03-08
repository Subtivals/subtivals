#include "styleadvanced.h"
#include "ui_styleadvanced.h"

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QSpinBox>

#include "script.h"
#include "subtitlestyle.h"

StyleAdvanced::StyleAdvanced(SubtitleStyle *p_style, QWidget *parent)
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

  connect(ui->lineSpacing, &QDoubleSpinBox::valueChanged, this, &StyleAdvanced::apply);
  connect(ui->verticalAlign, &QComboBox::currentIndexChanged, this, &StyleAdvanced::apply);
  connect(ui->horizontalAlign, &QComboBox::currentIndexChanged, this, &StyleAdvanced::apply);
  connect(ui->marginL, &QSpinBox::valueChanged, this, &StyleAdvanced::apply);
  connect(ui->marginR, &QSpinBox::valueChanged, this, &StyleAdvanced::apply);
  connect(ui->marginV, &QSpinBox::valueChanged, this, &StyleAdvanced::apply);
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
