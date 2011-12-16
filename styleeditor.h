#ifndef STYLEEDITOR_H
#define STYLEEDITOR_H

#include <QWidget>

namespace Ui {
    class StyleEditor;
}

class Script;
class Style;

class StyleEditor : public QWidget
{
    Q_OBJECT

public:
    explicit StyleEditor(Script* script, QWidget *parent = 0);
    ~StyleEditor();

public slots:
    void apply();
    void reset();
    void styleSelected(int);

protected:
    void initComponents();

private:
    Script* m_script;
    QList<Style*> m_styles;
    Ui::StyleEditor *ui;
};

#endif // STYLEEDITOR_H
