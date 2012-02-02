#include "styleeditor.h"
#include "ui_styleeditor.h"

#include <QListWidgetItem>
#include <QColorDialog>
#include <QPainter>
#include <QSettings>

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
    foreach(Style* style, m_backup)
        delete style;
    m_backup.clear();
    foreach(Style* style, m_script->styles()) {
        m_backup.append(new Style(*style, style->font()));
    }
    initComponents();
    reset();
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

void StyleEditor::save()
{
    QSettings settings;
    settings.beginGroup("Styles");
    // Save overidden styles into settings
    QString line;
    foreach(Style* style, m_overidden) {
        line = QString("%1/%2/%3").arg(style->font().family())
                                  .arg(style->font().pointSize())
                                  .arg(style->primaryColour().name());
        settings.setValue(style->name(), line);
    }
    settings.endGroup();
}

void StyleEditor::reset()
{
    // Reload styles from backup and apply overidden styles from settings
    m_overidden.clear();
    QSettings settings;
    settings.beginGroup("Styles");
    for(int i = 0; i < m_backup.size(); i++) {
        Style* original = m_backup.at(i);
        Style* style = m_script->style(original->name());

        QStringList overriden = settings.value(style->name(), "").toString().split("/");
        if (overriden.size() == 3) {
            QFont f(overriden[0]);
            f.setPointSize(overriden.at(1).toInt());
            style->setFont(f);
            style->setPrimaryColour(QColor(overriden[2]));
            m_overidden.append(style);
            ui->stylesNames->item(i)->setForeground(qApp->palette().highlight());
        }
        else {
            style->setFont(original->font());
            style->setPrimaryColour(original->primaryColour());
            ui->stylesNames->item(i)->setForeground(QBrush());
        }
    }
    settings.endGroup();
    emit styleChanged();
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

    // Distinct apparence of overidden item in list
    ui->stylesNames->item(row)->setForeground(qApp->palette().highlight());
    m_overidden.append(style);
}

void StyleEditor::restore()
{
    // Remove all styles from settings
    QSettings settings;
    settings.beginGroup("Styles");
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

