#include "styleeditor.h"
#include "ui_styleeditor.h"

#include <QListWidgetItem>

#include "script.h"


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
        QListWidgetItem *item = new QListWidgetItem(style->name());
        ui->stylesNames->addItem(item);
    }
    ui->stylesNames->setCurrentRow(-1);
    ui->groupFont->setEnabled(false);
}

void StyleEditor::saveStyle()
{
    // Style properties were edited, store a copy
    QString stylename = ui->stylesNames->currentItem()->text();
    QFont font = ui->fontName->currentFont();
    font.setPointSize(ui->fontSize->value());
    m_styles[stylename] = new Style(*m_script->style(stylename), font);
}

void StyleEditor::styleSelected(int row)
{
    ui->groupFont->setEnabled(row >= 0);
    if (row < 0)
        return;
    // Load style from script or locally if already edited
    QString stylename = ui->stylesNames->item(row)->text();
    Style* style = m_script->style(stylename);
    if (m_styles.contains(stylename))
        style = m_styles[stylename];
    // Show fonts with fixed size in combo
    QFont font(style->font());
    font.setPointSize(12);
    ui->fontName->setCurrentFont(font);
    ui->fontSize->setValue(style->font().pointSize());
}

void StyleEditor::apply()
{
    // Apply properties of copies to real ones
    foreach(QString stylename, m_styles.keys()) {
        Style* style = m_script->style(stylename);
        style->setFont(m_styles[stylename]->font());
    }
}

void StyleEditor::reset()
{
    // Clear copies and reinitialize UI
    foreach(QString key, m_styles.keys()) {
        delete m_styles[key];
    }
    m_styles.clear();
    initComponents();
}
