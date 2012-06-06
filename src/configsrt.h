#ifndef CONFIGSRT_H
#define CONFIGSRT_H

#include <QDialog>

class Script;

namespace Ui {
class ConfigSrt;
}

class ConfigSrt : public QDialog
{
    Q_OBJECT
    
public:
    explicit ConfigSrt(Script* p_script, QWidget *parent = 0);
    ~ConfigSrt();
protected slots:
    void accept();
private:
    Ui::ConfigSrt *ui;
    Script* m_script;
};

#endif // CONFIGSRT_H
