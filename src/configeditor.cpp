/**
  *  This file is part of Subtivals.
  *
  *  Subtivals is free software: you can redistribute it and/or modify
  *  it under the terms of the GNU General Public License as published by
  *  the Free Software Foundation, either version 3 of the License, or
  *  (at your option) any later version.
  *
  *  Subtivals is distributed in the hope that it will be useful,
  *  but WITHOUT ANY WARRANTY; without even the implied warranty of
  *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  *  GNU General Public License for more details.
  *
  *  You should have received a copy of the GNU General Public License
  *  along with Subtivals.  If not, see <http://www.gnu.org/licenses/>
  **/
#include <QtCore/QSettings>
#include <QtGui/QDesktopWidget>
#include <QtGui/QPushButton>
#include <QtGui/QPainter>
#include <QtGui/QColorDialog>

#include "configeditor.h"
#include "ui_configeditor.h"

#include "script.h"
#include "styleeditor.h"


#define NB_PRESETS 6
#define DEFAULT_HEIGHT 200
#define DEFAULT_COLOR "#000000"

ConfigEditor::ConfigEditor(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ConfigEditor),
    m_styleEditor(new StyleEditor()),
    m_preset(-1),
    m_parentWidget(parent)
{
    ui->setupUi(this);
    ui->tabStyles->setLayout(m_styleEditor->layout());
    connect(m_styleEditor, SIGNAL(styleChanged()), this, SIGNAL(styleChanged()));
    connect(m_styleEditor, SIGNAL(styleChanged()), this, SLOT(enableButtonBox()));
    adjustSize();
    setMaximumSize(size());

    QDesktopWidget *dw = QApplication::desktop();
    for(int i = 0; i < dw->screenCount(); i++) {
        ui->screens->addItem(QString(tr("Monitor %1")).arg(i));
    }

    for(int i = 1; i <= NB_PRESETS; i++) {
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

void ConfigEditor::chooseColor()
{
    // Show color chooser
    QColor chosen = QColorDialog::getColor(m_color, this
    #if (QT_VERSION >= QT_VERSION_CHECK(4, 5, 0))
        , tr("Select Color"), QColorDialog::ShowAlphaChannel
    #endif
        );
    if (chosen.isValid()) {
        setColor(chosen);
        apply();
    }
}

void ConfigEditor::setColor(const QColor& c)
{
    m_color = c;
    QPixmap pm(24, 24);
    QPainter p(&pm);
    p.setPen(c);
    p.setBrush(c);
    p.drawRect(0, 0, 24, 24);
    ui->btnColor->setIcon(pm);
}

void ConfigEditor::restore()
{
    // Apply default screen size
    if (ui->tabs->currentWidget() == ui->tabScreen) {
        QSettings settings;
        settings.remove(QString("ScreenGeometry-%1").arg(m_preset));
    } else {
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
    QRect screenGeom = qApp->desktop()->screenGeometry(screen);
    int width = screenGeom.width();
    int left = 0;
    int top = 0;

    // If only one screen, make it less obstrusive
    // place screen under the main window
    if (qApp->desktop()->screenCount() == 1) {
        left = m_parentWidget->geometry().left();
        width = m_parentWidget->geometry().width() + 2;
        top = m_parentWidget->geometry().bottom() + 5;
        // Prsubtitle exceeding bottom of desktop
        if ((top + DEFAULT_HEIGHT) > screenGeom.height())
            top = screenGeom.height() - DEFAULT_HEIGHT;
    }

    int x = settings.value("x", left).toInt();
    int y = settings.value("y", top).toInt();
    int w = settings.value("w", width).toInt();
    int h = settings.value("h", DEFAULT_HEIGHT).toInt();
    double rotation = settings.value("rotation", 0).toDouble();
    QColor color = QColor(settings.value("color", DEFAULT_COLOR).toString());
    settings.endGroup();
    // Update the UI with the reloaded settings
    ui->screens->setCurrentIndex(screen);
    screenChanged(QRect(x, y, w, h));
    ui->rotation->setValue(rotation);
    setColor(color);
    apply();
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
    settings.setValue("color", m_color.name());
    settings.endGroup();
    m_styleEditor->save();
    // Settings saved, update the UI
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
    emit color(m_color);
    m_styleEditor->apply();
    enableButtonBox(true, true, true);
}

void ConfigEditor::onClicked(QAbstractButton* btn)
{
    if (ui->buttonBox->buttonRole(btn) == ui->buttonBox->ResetRole) {
        restore();  // ResetRole == RestoreDefaults button (sic)
    } else if (ui->buttonBox->buttonRole(btn) == ui->buttonBox->AcceptRole) {
        save();
    } else if (ui->buttonBox->buttonRole(btn) == ui->buttonBox->RejectRole) {
        reset();
    }
}

void ConfigEditor::enableButtonBox(bool restore, bool cancel, bool save)
{
    ui->buttonBox->button(QDialogButtonBox::RestoreDefaults)->setEnabled(restore);
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setEnabled(cancel);
    ui->buttonBox->button(QDialogButtonBox::Save)->setEnabled(save);
}
