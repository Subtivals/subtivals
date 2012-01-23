#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include <QDockWidget>
#include <QAbstractButton>

namespace Ui {
    class ConfigDialog;
}

class Script;
class StyleEditor;

class ConfigDialog : public QDockWidget
{
    Q_OBJECT

public:
    explicit ConfigDialog(QWidget *parent = 0);
    ~ConfigDialog();
    void setScript(Script* script);
signals:
    void changeScreen(int, QRect);
public slots:
    void screenChanged(const QRect& r);
    void restore();
    void reset();
    void apply();
    void save();
    void onClicked(QAbstractButton*);
private:
    Ui::ConfigDialog *ui;
    StyleEditor* m_styleEditor;
};

#endif // CONFIGDIALOG_H
