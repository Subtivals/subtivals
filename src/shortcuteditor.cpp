#include "shortcuteditor.h"
#include "ui_shortcuteditor.h"

#include <QSettings>

ShortcutEditor::ShortcutEditor(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ShortcutEditor)
{
    ui->setupUi(this);
    ui->tableActions->verticalHeader()->hide();
    ui->tableActions->horizontalHeader()->setResizeMode(COLUMN_DESCRIPTION, QHeaderView::Stretch);
    connect(ui->tableActions, SIGNAL(itemPressed(QTableWidgetItem*)), this, SLOT(recordAction(QTableWidgetItem*)));
    connect(ui->tableActions, SIGNAL(itemChanged(QTableWidgetItem*)), this, SLOT(validateAction(QTableWidgetItem*)));
    connect(ui->buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(onClicked(QAbstractButton*)));
}

ShortcutEditor::~ShortcutEditor()
{
    delete ui;
}

void ShortcutEditor::apply()
{
    for(int i=0; i<ui->tableActions->rowCount(); i++) {
        QKeySequence accel(ui->tableActions->item(i, COLUMN_SHORTCUT)->text());
        m_actions[i]->setShortcut(accel);
    }
    emit(changed());
}

void ShortcutEditor::reset()
{
    int row = 0;
    QTableWidgetItem* item;
    ui->tableActions->blockSignals(true);
    ui->tableActions->setRowCount(m_actions.size());

    QSettings settings;
    settings.beginGroup(QString("Shortcuts"));
    foreach(QAction *action, m_actions) {
        // Description column
        item = new QTableWidgetItem(action->text());
        item->setIcon(action->icon());
        item->setToolTip(action->toolTip());
        item->setFlags(item->flags() ^ Qt::ItemIsEditable);
        ui->tableActions->setItem(row, COLUMN_DESCRIPTION, item);

        // Read from settings if exists
        QString accel = settings.value(action->objectName(),
                                       m_backup[action].toString()).toString();
        item = new QTableWidgetItem(accel);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        ui->tableActions->setItem(row, COLUMN_SHORTCUT, item);
        row++;
    }
    ui->tableActions->blockSignals(false);
    apply();
}

void ShortcutEditor::restore()
{
    QSettings settings;
    settings.beginGroup(QString("Shortcuts"));
    foreach(QString key, settings.allKeys()) {
        settings.remove(key);
    }
    settings.endGroup();
    reset();
}

void ShortcutEditor::onClicked(QAbstractButton* btn)
{
    if (ui->buttonBox->buttonRole(btn) == ui->buttonBox->ResetRole) {
        restore();  // ResetRole == RestoreDefaults button (sic)
    } else if (ui->buttonBox->buttonRole(btn) == ui->buttonBox->AcceptRole) {
        apply();
    } else if (ui->buttonBox->buttonRole(btn) == ui->buttonBox->RejectRole) {
        reset();
    }
}

void ShortcutEditor::recordAction(QTableWidgetItem* item)
{
    if (m_oldAccelText.isEmpty())
        m_oldAccelText = QKeySequence(item->text()).toString();
}

void ShortcutEditor::validateAction(QTableWidgetItem* item)
{
    if (item->column() != 1)
        return;
    QKeySequence accel(item->text());
    QString accelText = QString(accel);
    if (accelText.isEmpty() && !item->text().isEmpty()) {
        item->setText(m_oldAccelText);
    } else {
        item->setText(accelText);
        m_oldAccelText = "";
    }
    apply();
}

void ShortcutEditor::registerAction(QAction *p_action)
{
    if (p_action->text().simplified().isEmpty())
        return;
    m_actions.append(p_action);
    m_backup[p_action] = p_action->shortcut();
    reset();
}
