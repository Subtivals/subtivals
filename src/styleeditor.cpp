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
#include "styleeditor.h"
#include "ui_styleeditor.h"

#include <QColorDialog>
#include <QPainter>
#include <QSettings>

#include "script.h"
#include "style.h"
#include "styleadvanced.h"


StyleEditor::StyleEditor(QWidget *parent) :
    QWidget(parent),
    m_script(0),
    m_preset(-1),
    ui(new Ui::StyleEditor)
{
    ui->setupUi(this);
}

StyleEditor::~StyleEditor()
{
    delete ui;
}

void StyleEditor::setPreset(int p_preset)
{
    m_preset = p_preset;
}

void StyleEditor::advancedConfig()
{
    QString styleName = ui->stylesNames->selectedItems().first()->text();
    Style* style = m_script->style(styleName);
    StyleAdvanced config(style, this);
    connect(&config, SIGNAL(styleChanged()), SLOT(apply()));
    config.move(this->parentWidget()->geometry().center());
    config.exec();
}

void StyleEditor::setScript(Script* script)
{
    m_script = script;
    foreach(Style* style, m_backup)
        delete style;
    m_backup.clear();
    if (m_script) {
        foreach(Style* style, m_script->styles()) {
            m_backup.append(new Style(*style, style->font()));
        }
    }
    initComponents();
}

void StyleEditor::styleSelected()
{
    if (!m_script)
        return;

    QList<QListWidgetItem*> selected = ui->stylesNames->selectedItems();
    int nbselected = selected.size();
    ui->groupFont->setEnabled(nbselected > 0);
    ui->btnAdvanced->setEnabled(nbselected == 1);
    if (nbselected == 0)
        return;

    // Block signals to avoid apply() to be called.
    ui->fontName->blockSignals(true);
    ui->fontSize->blockSignals(true);

    // Load style from script
    Style* first = m_script->style(selected.first()->text());
    m_colour = first->primaryColour();
    QFont font(first->font());
    font.setPixelSize(12);  // fixed size in combo
    ui->fontName->setCurrentFont(font);
    ui->fontSize->setValue(first->font().pixelSize());
    ui->btnBold->setChecked(font.bold());
    ui->btnItalic->setChecked(font.italic());

    foreach(QListWidgetItem* item, selected) {
        // If any style differs from first, clear fields
        Style* style = m_script->style(item->text());
        if (style->font().family() != first->font().family())
            ui->fontName->setCurrentIndex(-1);
        if (style->font().pixelSize() != first->font().pixelSize())
            ui->fontSize->clear();
        if (style->primaryColour() != first->primaryColour())
            m_colour = Qt::transparent;
    }
    fillButtonColour();

    ui->fontName->blockSignals(false);
    ui->fontSize->blockSignals(false);
}

void StyleEditor::save()
{
    QSettings settings;
    settings.beginGroup(QString("Styles-%1").arg(m_preset));
    // Save overidden styles into settings
    QString line;
    foreach(Style* style, m_overidden) {
        line = QString("%1/%2/%3/%4/%5/%7/%8/%9/%10/%11")
                .arg(style->font().family())
                .arg(style->font().pixelSize())
                .arg(style->primaryColour().name())
                .arg(style->lineSpacing())
                .arg(int(style->alignment()))
                .arg(style->marginL())
                .arg(style->marginR())
                .arg(style->marginV())
                .arg(style->font().bold() ? "bold": "")
                .arg(style->font().italic() ? "italic": "");
        settings.setValue(style->name(), line);
    }
    settings.endGroup();
}

void StyleEditor::reset()
{
    // Reload styles from backup and apply overidden styles from settings
    ui->stylesNames->clearSelection();
    ui->stylesNames->setCurrentRow(-1);

    m_overidden.clear();

    QSettings settings;
    settings.beginGroup(QString("Styles-%1").arg(m_preset));
    for(int i = 0; i < m_backup.size(); i++) {
        Style* original = m_backup.at(i);
        Style* style = m_script->style(original->name());

        QStringList overriden = settings.value(style->name(), "").toString().split("/");
        if (overriden.size() >= 8) {
            QFont f(overriden[0]);
            f.setPixelSize(overriden.at(1).toInt());

            if (overriden.size() >= 10) {
                f.setBold(overriden.at(8) == "bold");
                f.setItalic(overriden.at(9) == "italic");
            }

            style->setFont(f);
            style->setPrimaryColour(QColor(overriden[2]));
            style->setLineSpacing(overriden.at(3).toDouble());
            style->setAlignment(QFlag(overriden.at(4).toInt()));
            style->setMargins(overriden.at(5).toInt(),
                              overriden.at(6).toInt(),
                              overriden.at(7).toInt());
            m_overidden.append(style);
            setStyleNameBold(i, true);
        }
        else {
            style->setFont(original->font());
            style->setPrimaryColour(original->primaryColour());
            setStyleNameBold(i, false);
        }
    }
    settings.endGroup();

    emit styleOverriden(m_overidden.size() > 0);

    if (ui->stylesNames->count() > 0)
        ui->stylesNames->setCurrentRow(0);

    emit styleChanged();
}

void StyleEditor::apply()
{
    if (!m_script)
        return;

    QList<QListWidgetItem*> selected = ui->stylesNames->selectedItems();
    int nbselected = selected.size();
    if (nbselected == 0)
        return;

    foreach(QListWidgetItem* item, selected) {
        Style* style = m_script->style(item->text());

        int fontSize = style->font().pixelSize();
        QFont font = style->font();
        if (!ui->fontSize->text().isEmpty()) {
            fontSize = ui->fontSize->value();
            font.setPixelSize(ui->fontSize->value());
        }
        if (ui->fontName->currentIndex() >= 0) {
            font = ui->fontName->currentFont();
            font.setPixelSize(fontSize);
        }
        font.setBold(ui->btnBold->isChecked());
        font.setItalic(ui->btnItalic->isChecked());
        style->setFont(font);

        if (m_colour != Qt::transparent) {
            style->setPrimaryColour(m_colour);
        }

        m_overidden.append(style);
        setStyleNameBold(item, true);
        emit styleOverriden(true);
    }

    styleSelected();

    emit styleChanged();
}

void StyleEditor::restore()
{
    // Remove all styles from settings
    QSettings settings;
    settings.beginGroup(QString("Styles-%1").arg(m_preset));
    foreach(QString key, settings.allKeys()) {
        settings.remove(key);
    }
    settings.endGroup();
    m_overidden.clear();
    reset();
}

void StyleEditor::chooseColour()
{
    // Show color chooser
    QColor init = m_colour;
    if (init == Qt::transparent)
        init = Qt::white;
    QColor chosen = QColorDialog::getColor(init, this
    #if (QT_VERSION >= QT_VERSION_CHECK(4, 5, 0))
        , tr("Select Color"), QColorDialog::ShowAlphaChannel
    #endif
        );
    if (chosen.isValid()) {
        m_colour = chosen;
        fillButtonColour();
        apply();
    }
}

void StyleEditor::fillButtonColour()
{
    QPixmap pm(24, 24);
    QPainter p(&pm);
    if (m_colour == Qt::transparent) {
        ui->btnColor->setIcon(QIcon());
    }
    else {
        p.setPen(m_colour);
        p.setBrush(p.pen().color());
        p.drawRect(0, 0, 24, 24);
        ui->btnColor->setIcon(pm);
    }
}

void StyleEditor::setStyleNameBold(int row, bool bold)
{
    setStyleNameBold(ui->stylesNames->item(row), bold);
}

void StyleEditor::setStyleNameBold(QListWidgetItem *item, bool bold)
{
    QFont f = item->font();
    f.setBold(bold);
    item->setFont(f);
}

void StyleEditor::initComponents()
{
    ui->groupStyles->setEnabled(m_script);
    ui->groupFont->setEnabled(false);
    if (!m_script)
        return;

    ui->stylesNames->clear();
    foreach(Style* style, m_script->styles()) {
        // Add to the list
        QListWidgetItem *item = new QListWidgetItem(style->name());
        ui->stylesNames->addItem(item);
    }
}
