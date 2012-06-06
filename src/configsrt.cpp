#include "configsrt.h"
#include "ui_configsrt.h"

#include "script.h"
#include "style.h"

ConfigSrt::ConfigSrt(Script* p_script, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ConfigSrt),
    m_script(p_script)
{
    ui->setupUi(this);
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

    Style* defaultStyle = m_script->styles().at(0);
    defaultStyle->setAlignment(vertical | horizontal);
    defaultStyle->setMargins(ui->marginL->value(),
                             ui->marginR->value(),
                             ui->maringV->value());

    close();
}
