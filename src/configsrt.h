#ifndef CONFIGSRT_H
#define CONFIGSRT_H

#include <QDialog>

class Style;

namespace Ui {
class ConfigSrt;
}

class ConfigSrt : public QDialog
{
    Q_OBJECT
    
public:
    explicit ConfigSrt(Style* p_style, QWidget *parent = 0);
    ~ConfigSrt();
protected slots:
    void accept();
private:
    Ui::ConfigSrt *ui;
    Style* m_style;
};

#endif // CONFIGSRT_H
