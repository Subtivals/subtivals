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
#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include <QDockWidget>
#include <QAbstractButton>
#include <QColor>


#define NB_PRESETS 6
#define DEFAULT_HEIGHT 200
#define DEFAULT_COLOR "#000000"

#define DEFAULT_OUTLINE_COLOR "#000000"
#define DEFAULT_OUTLINE_WIDTH 0


namespace Ui {
    class ConfigEditor;
}

class Script;
class StyleEditor;
class QPushButton;

class ConfigEditor : public QWidget
{
    Q_OBJECT

public:
    explicit ConfigEditor(QWidget *parent = 0);
    ~ConfigEditor();
    void setScript(Script* script);
signals:
    // Live options
    void hideDesktop(bool);
    void webliveEnabled(bool);

    // Presets
    void changeScreen(int, QRect);
    void rotate(double);
    void color(QColor);
    void outline(QColor, int);
    // From styleeditor
    void styleChanged();
    void styleOverriden(bool);
public slots:
    void presetChanged(int);
    void screenChanged(const QRect& r);
    void restore();
    void reset();
    void apply();
    void save();
    void onClicked(QAbstractButton*);
    void chooseColor();

    void enableWeblive(bool);
    void webliveConnected(bool p_state, QString p_url);
protected slots:
    void enableButtonBox(bool restore = true, bool cancel = true, bool save = true);
    void setColor(QPushButton *button, const QColor &c);
private:
    Ui::ConfigEditor *ui;
    StyleEditor* m_styleEditor;
    int m_preset;
    QColor m_color;
    QColor m_outlineColor;
    QWidget* m_parentWidget;
};

#endif // CONFIGDIALOG_H
