/**
  *  This file is part of Subtivals.
  *
  *  Subtivals is free software: you can redistribute it and/or modify
  *  it under the terms of the GNU General Public License as published by
  *  the Free Software Foundation, either version 3 of the License, or
  *  (at your option) any later version.
  *
  *  Subtivals is distributed in the hope that it will be useful,
  *  but WITHOUT ANY WARRANTY; without even the implied warranty of
  *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  *  GNU General Public License for more details.
  *
  *  You should have received a copy of the GNU General Public License
  *  along with Subtivals.  If not, see <http://www.gnu.org/licenses/>
  **/
#ifndef STYLEEDITOR_H
#define STYLEEDITOR_H

#include <QWidget>
#include <QListWidgetItem>

namespace Ui {
    class StyleEditor;
}

class Script;
class Style;

class StyleEditor : public QWidget
{
    Q_OBJECT

public:
    explicit StyleEditor(QWidget *parent = 0);
    ~StyleEditor();
    void setScript(Script*);
signals:
    void styleChanged();
    void styleOverriden(bool);
public slots:
    void restore();
    void save();
    void apply();
    void reset();
    void styleSelected();
    void chooseColour();
    void setPreset(int);
    void advancedConfig();
protected:
    void fillButtonColour();
    void setStyleNameBold(int, bool);
    void setStyleNameBold(QListWidgetItem *item, bool bold);
    void initComponents();
private:
    Script* m_script;
    QList<Style*> m_backup;
    QList<Style*> m_overidden;
    QColor m_colour;
    int m_preset;
    Ui::StyleEditor *ui;
};

#endif // STYLEEDITOR_H
