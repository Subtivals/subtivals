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
  *  along with Foobar.  If not, see <http://www.gnu.org/licenses/>
  **/
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
