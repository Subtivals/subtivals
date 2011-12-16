#include "styleeditor.h"
#include "ui_styleeditor.h"

#include <QListWidgetItem>

#include "script.h"
#include "style.h"


StyleEditor::StyleEditor(Script* script, QWidget *parent) :
    QWidget(parent),
    m_script(script),
    ui(new Ui::StyleEditor)
{
    ui->setupUi(this);
}

StyleEditor::~StyleEditor()
{
    delete ui;
}

void StyleEditor::initComponents()
{
    ui->groupStyles->setEnabled(m_script);
    if (!m_script)
        return;

    ui->stylesNames->clear();
    foreach(Style* style, m_script->styles()) {
        // Backup for reset
        m_styles.append(new Style(*style, style->font()));
        // Add to the list
        QListWidgetItem *item = new QListWidgetItem(style->name());
        ui->stylesNames->addItem(item);
    }
    ui->stylesNames->setCurrentRow(-1);
    ui->groupFont->setEnabled(false);
}

void StyleEditor::styleSelected(int row)
{
    ui->groupFont->setEnabled(row >= 0);
    if (row < 0)
        return;

    // Load style from script
    QString stylename = ui->stylesNames->item(row)->text();
    Style* style = m_script->style(stylename);
    QFont font(style->font());
    font.setPointSize(12);  // fixed size in combo

    // Block signals to avoid apply() to be called.
    ui->fontName->blockSignals(true);
    ui->fontName->setCurrentFont(font);
    ui->fontName->blockSignals(false);

    ui->fontSize->blockSignals(true);
    ui->fontSize->setValue(style->font().pointSize());
    ui->fontSize->blockSignals(false);
}

void StyleEditor::apply()
{
    int row = ui->stylesNames->currentRow();
    if (row < 0)
        return;
    // Style properties were edited, store a copy
    QString stylename = ui->stylesNames->item(row)->text();
    QFont font = ui->fontName->currentFont();
    font.setPointSize(ui->fontSize->value());
    Style* style = m_script->style(stylename);
    style->setFont(font);
}

void StyleEditor::reset()
{
    // Reapply properties from backup of styles
    foreach(Style* original, m_styles) {
        Style* style = m_script->style(original->name());
        style->setFont(original->font());
        delete original;
    }
    m_styles.clear();
    // Reinit UI
    initComponents();
}
