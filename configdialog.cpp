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
    // Connect value changing to apply (live preview)
    connect(ui->x, SIGNAL(valueChanged(int)), this, SLOT(saveConfig()));
    connect(ui->y, SIGNAL(valueChanged(int)), this, SLOT(saveConfig()));
    connect(ui->w, SIGNAL(valueChanged(int)), this, SLOT(saveConfig()));
    connect(ui->h, SIGNAL(valueChanged(int)), this, SLOT(saveConfig()));
    connect(ui->screens, SIGNAL(currentIndexChanged(int)), this, SLOT(saveConfig()));
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
    ui->x->setValue(m_rect.x());
    ui->y->setValue(m_rect.y());
    ui->w->setValue(m_rect.width());
    ui->h->setValue(m_rect.height());
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
