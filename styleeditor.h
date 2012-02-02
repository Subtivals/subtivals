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
    explicit StyleEditor(Script* script = 0, QWidget *parent = 0);
    ~StyleEditor();
    void setScript(Script*);
signals:
    void styleChanged();
public slots:
    void restore();
    void save();
    void apply();
    void reset();
    void styleSelected(int);
    void chooseColour();

protected:
    void initComponents();
    void fillButtonColour();
    void setStyleNameBold(int, bool);

private:
    Script* m_script;
    QList<Style*> m_backup;
    QList<Style*> m_overidden;
    QColor m_colour;
    Ui::StyleEditor *ui;
};

#endif // STYLEEDITOR_H
