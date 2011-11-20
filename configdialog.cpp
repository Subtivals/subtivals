#include <QtCore/QSettings>
#include <QtGui/QDesktopWidget>

#include "configdialog.h"
#include "ui_configdialog.h"

ConfigDialog::ConfigDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ConfigDialog)
{
    ui->setupUi(this);
    QDesktopWidget *dw = QApplication::desktop();
    for(int i = 0; i < dw->screenCount(); i++)
    {
        ui->screens->addItem(QString("Screen %1").arg(i));
    }
    QSettings settings;
    settings.beginGroup("SubtitlesForm");
    m_screen = settings.value("screen", 0).toInt();
    m_rect.setX(settings.value("x", 0).toInt());
    m_rect.setY(settings.value("y", 0).toInt());
    m_rect.setWidth(settings.value("w", 0).toInt());
    m_rect.setHeight(settings.value("h", 0).toInt());
    settings.endGroup();
    resetConfig();
}

ConfigDialog::~ConfigDialog()
{
    delete ui;
}

void ConfigDialog::buttonClicked(QAbstractButton* btn)
{
    if (QDialogButtonBox::ApplyRole == ui->buttonBox->buttonRole(btn))
        saveConfig();
    if (QDialogButtonBox::RejectRole == ui->buttonBox->buttonRole(btn))
        resetConfig();
}

void ConfigDialog::resetConfig()
{
    ui->screens->setCurrentIndex(m_screen);
    ui->x->setText(QString("%1").arg(m_rect.x()));
    ui->y->setText(QString("%1").arg(m_rect.y()));
    ui->w->setText(QString("%1").arg(m_rect.width()));
    ui->h->setText(QString("%1").arg(m_rect.height()));
    saveConfig();
}

void ConfigDialog::saveConfig()
{
    QSettings settings;
    settings.beginGroup("SubtitlesForm");
    settings.setValue("screen", ui->screens->currentIndex());
    settings.setValue("x", ui->x->text());
    settings.setValue("y", ui->y->text());
    settings.setValue("w", ui->w->text());
    settings.setValue("h", ui->h->text());
    settings.endGroup();
    emit configChanged();
}
