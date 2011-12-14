#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include <QDialog>
#include <QAbstractButton>

namespace Ui {
    class ConfigDialog;
}

class Script;
class StyleEditor;

class ConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConfigDialog(Script* script, QWidget *parent = 0);
    ~ConfigDialog();
signals:
    void configChanged();
private slots:
    void saveConfig();
    void resetConfig();
    void buttonClicked(QAbstractButton*);
private:
    Ui::ConfigDialog *ui;
    int m_screen;
    QRect m_rect;
    StyleEditor* m_styleEditor;
};

#endif // CONFIGDIALOG_H
