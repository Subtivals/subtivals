#include <QtCore/QSettings>
#include <QtGui/QDesktopWidget>
#include <QtGui/QPushButton>

#include "configeditor.h"
#include "ui_configeditor.h"

#include "script.h"
#include "styleeditor.h"


#define NB_PRESETS 6


ConfigEditor::ConfigEditor(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::ConfigEditor),
    m_styleEditor(new StyleEditor()),
    m_preset(-1)
{
    ui->setupUi(this);
    setFeatures(ConfigEditor::NoDockWidgetFeatures);
    ui->tabStyles->setLayout(m_styleEditor->layout());
    connect(m_styleEditor, SIGNAL(styleChanged()), this, SIGNAL(styleChanged()));
    connect(m_styleEditor, SIGNAL(styleChanged()), this, SLOT(enableButtonBox()));
    adjustSize();

    QDesktopWidget *dw = QApplication::desktop();
    for(int i = 0; i < dw->screenCount(); i++)
    {
        ui->screens->addItem(QString(tr("Monitor %1")).arg(i));
    }

    for(int i = 1; i <= NB_PRESETS; i++)
    {
        ui->presets->addItem(QString(tr("Preset %1")).arg(i));
    }
    // Use last preset, this will trigger presetChanged()
    QSettings settings;
    ui->presets->setCurrentIndex(settings.value("preset", 0).toInt());
}

ConfigEditor::~ConfigEditor()
{
    // Save last preset
    QSettings settings;
    settings.setValue("preset", m_preset);
    delete m_styleEditor;
    delete ui;
}

void ConfigEditor::setScript(Script* script)
{
    m_styleEditor->setScript(script);
    reset();
}

void ConfigEditor::presetChanged(int p_preset)
{
    if (m_preset >= 0)  // Do not save while constructing
        save();
    m_styleEditor->setPreset(p_preset);
    m_preset = p_preset;
    reset();
}

void ConfigEditor::screenChanged(const QRect& r)
{
    // Show values in form
    ui->x->setValue(r.x());
    ui->y->setValue(r.y());
    ui->w->setValue(r.width());
    ui->h->setValue(r.height());
}

void ConfigEditor::restore()
{
    // Apply default screen size
    if (ui->tabs->currentWidget() == ui->tabScreen) {
        QSettings settings;
        settings.remove(QString("ScreenGeometry-%1").arg(m_preset));
    }
    else {
        m_styleEditor->restore();
    }
    reset();
    enableButtonBox(false, false, false);
}

void ConfigEditor::reset()
{
    // Reload from settings
    QSettings settings;
    settings.beginGroup(QString("ScreenGeometry-%1").arg(m_preset));
    int screen = settings.value("screen", 0).toInt();
    int width = QApplication::desktop()->screenGeometry(screen).width();

    int x = settings.value("x", 0).toInt();
    int y = settings.value("y", 0).toInt();
    int w = settings.value("w", width).toInt();
    int h = settings.value("h", 200).toInt();
    double rotation = settings.value("rotation", 0).toDouble();
    settings.endGroup();

    ui->screens->setCurrentIndex(screen);
    screenChanged(QRect(x, y, w, h));
    ui->rotation->setValue(rotation);
    m_styleEditor->reset();
    enableButtonBox(true, false, false);
}

void ConfigEditor::save()
{
    // Save settings
    QSettings settings;
    settings.beginGroup(QString("ScreenGeometry-%1").arg(m_preset));
    settings.setValue("screen", ui->screens->currentIndex());
    settings.setValue("x", ui->x->text());
    settings.setValue("y", ui->y->text());
    settings.setValue("w", ui->w->text());
    settings.setValue("h", ui->h->text());
    settings.setValue("rotation", ui->rotation->text());
    settings.endGroup();
    m_styleEditor->save();

    enableButtonBox(true, false, false);
}

void ConfigEditor::apply()
{
    int screen = ui->screens->currentIndex();
    QRect r(ui->x->text().toInt(),
            ui->y->text().toInt(),
            ui->w->text().toInt(),
            ui->h->text().toInt());
    emit changeScreen(screen, r);
    emit rotate(ui->rotation->value());
    m_styleEditor->apply();
    enableButtonBox(true, true, true);
}

void ConfigEditor::onClicked(QAbstractButton* btn)
{
    if (ui->buttonBox->buttonRole(btn) == ui->buttonBox->ResetRole) {
        restore();  // ResetRole == RestoreDefaults button (sic)
    }
    else if (ui->buttonBox->buttonRole(btn) == ui->buttonBox->AcceptRole) {
        save();
    }
    else if (ui->buttonBox->buttonRole(btn) == ui->buttonBox->RejectRole) {
        reset();
    }
}

void ConfigEditor::enableButtonBox(bool restore, bool cancel, bool save)
{
    ui->buttonBox->button(QDialogButtonBox::RestoreDefaults)->setEnabled(restore);
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setEnabled(cancel);
    ui->buttonBox->button(QDialogButtonBox::Save)->setEnabled(save);
}
