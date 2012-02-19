#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include <QDockWidget>
#include <QAbstractButton>

namespace Ui {
    class ConfigEditor;
}

class Script;
class StyleEditor;

class ConfigEditor : public QDockWidget
{
    Q_OBJECT

public:
    explicit ConfigEditor(QWidget *parent = 0);
    ~ConfigEditor();
    void setScript(Script* script);
signals:
    void changeScreen(int, QRect);
    void rotate(double);
    void styleChanged();
public slots:
    void presetChanged(int);
    void screenChanged(const QRect& r);
    void restore();
    void reset();
    void apply();
    void save();
    void onClicked(QAbstractButton*);
protected slots:
    void enableButtonBox(bool restore = true, bool cancel = true, bool save = true);
private:
    Ui::ConfigEditor *ui;
    StyleEditor* m_styleEditor;
    int m_preset;
};

#endif // CONFIGDIALOG_H
