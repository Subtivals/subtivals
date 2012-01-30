#include "styleeditor.h"
#include "ui_styleeditor.h"

#include <QListWidgetItem>
#include <QColorDialog>
#include <QPainter>

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

void StyleEditor::setScript(Script* script)
{
    m_script = script;
    initComponents();
}

void StyleEditor::initComponents()
{
    ui->groupStyles->setEnabled(m_script);
    ui->groupFont->setEnabled(false);
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
    m_colour = style->primaryColour();
    fillButtonColour();

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
    style->setPrimaryColour(m_colour);
    emit styleChanged();
}

void StyleEditor::restore()
{
    // Reapply properties from backup of styles
    foreach(Style* original, m_styles) {
        Style* style = m_script->style(original->name());
        style->setFont(original->font());
        style->setPrimaryColour(original->primaryColour());
        delete original;
    }
    emit styleChanged();
    m_styles.clear();
    // Reinit UI
    initComponents();
}

void StyleEditor::chooseColour()
{
    // Show color chooser
    QColor chosen = QColorDialog::getColor(m_colour, this
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
    p.setPen(m_colour);
    p.setBrush(m_colour);
    p.drawRect(0, 0, 24, 24);
    ui->btnColor->setIcon(pm);
}

