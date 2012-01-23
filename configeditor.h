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
public slots:
    void screenChanged(const QRect& r);
    void restore();
    void reset();
    void apply();
    void save();
    void onClicked(QAbstractButton*);
private:
    Ui::ConfigEditor *ui;
    StyleEditor* m_styleEditor;
};

#endif // CONFIGDIALOG_H
