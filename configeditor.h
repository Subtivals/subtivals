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
    void styleChanged();
public slots:
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
};

#endif // CONFIGDIALOG_H
