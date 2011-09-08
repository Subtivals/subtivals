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
    ui->screens->setCurrentIndex(settings.value("screen", 0).toInt());
    ui->x->setText(settings.value("x", 0).toString());
    ui->y->setText(settings.value("y", 0).toString());
    ui->w->setText(settings.value("w", 0).toString());
    ui->h->setText(settings.value("h", 0).toString());
    settings.endGroup();

}

ConfigDialog::~ConfigDialog()
{
    delete ui;
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
}
