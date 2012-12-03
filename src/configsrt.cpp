#include "configsrt.h"
#include "ui_configsrt.h"

#include "script.h"
#include "style.h"

ConfigSrt::ConfigSrt(Style* p_style, QWidget *parent):
    QDialog(parent),
    ui(new Ui::ConfigSrt),
    m_style(p_style)
{
    ui->setupUi(this);

    if (m_style->alignment() & Qt::AlignTop)
        ui->verticalAlign->setCurrentIndex(0);
    else if (m_style->alignment() & Qt::AlignVCenter)
        ui->verticalAlign->setCurrentIndex(1);
    else if (m_style->alignment() & Qt::AlignBottom)
        ui->verticalAlign->setCurrentIndex(2);
    if (m_style->alignment() & Qt::AlignLeft)
        ui->horizontalAlign->setCurrentIndex(0);
    else if (m_style->alignment() & Qt::AlignHCenter)
        ui->horizontalAlign->setCurrentIndex(1);
    else if (m_style->alignment() & Qt::AlignRight)
        ui->horizontalAlign->setCurrentIndex(2);
    ui->marginL->setValue(m_style->marginL());
    ui->marginR->setValue(m_style->marginR());
    ui->marginV->setValue(m_style->marginV());
}

ConfigSrt::~ConfigSrt()
{
    delete ui;
}

void ConfigSrt::accept()
{
    Qt::Alignment vertical;
    switch(ui->verticalAlign->currentIndex()) {
    case 0: vertical = Qt::AlignTop; break;
    case 1: vertical = Qt::AlignVCenter; break;
    case 2: vertical = Qt::AlignBottom; break;
    default: break;
    }
    Qt::Alignment horizontal;
    switch(ui->horizontalAlign->currentIndex()) {
    case 0: horizontal = Qt::AlignLeft; break;
    case 1: horizontal = Qt::AlignHCenter; break;
    case 2: horizontal = Qt::AlignRight; break;
    default: break;
    }

    m_style->setAlignment(vertical | horizontal);
    m_style->setMargins(ui->marginL->value(),
                        ui->marginR->value(),
                        ui->marginV->value());

    QDialog::accept();
}
