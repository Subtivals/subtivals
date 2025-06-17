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
#include <QColorDialog>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QScreen>
#include <QtCore/QSettings>

#include "configeditor.h"
#include "ui_configeditor.h"

#include "script.h"
#include "styleeditor.h"

ConfigEditor::ConfigEditor(QWidget *parent)
    : QWidget(parent), ui(new Ui::ConfigEditor),
      m_styleEditor(new StyleEditor(this)), m_preset(-1),
      m_parentWidget(parent) {
  ui->setupUi(this);
  ui->tabStyles->setLayout(m_styleEditor->layout());
  connect(m_styleEditor, SIGNAL(styleChanged()), this, SIGNAL(styleChanged()));
  connect(m_styleEditor, SIGNAL(styleOverriden(bool)), this,
          SIGNAL(styleOverriden(bool)));
  connect(m_styleEditor, SIGNAL(styleChanged()), this, SLOT(enableButtonBox()));
  connect(ui->enableWeblive, SIGNAL(toggled(bool)), this,
          SIGNAL(webliveEnabled(bool)));

  adjustSize();
  setMaximumSize(size());

  connect(qApp, SIGNAL(screenAdded(QScreen *)), this,
          SLOT(refreshScreensList()));
  connect(qApp, SIGNAL(screenRemoved(QScreen *)), this,
          SLOT(refreshScreensList()));
  refreshScreensList();

  // Load known presets.
  QSettings settings;
  for (int i = 0; i < NB_PRESETS; i++) {
    settings.beginGroup(QString("ScreenGeometry-%1").arg(i));
    QString name =
        settings.value("name", QString(tr("Preset %1")).arg(i + 1)).toString();
    ui->presets->addItem(name);
    settings.endGroup();
  }
  // Use last preset, this will trigger presetChanged()
  ui->presets->setCurrentIndex(settings.value("preset", 0).toInt());

  ui->presets->installEventFilter(this);
}

ConfigEditor::~ConfigEditor() {
  // Save last preset
  QSettings settings;
  settings.setValue("preset", m_preset);
  delete m_styleEditor;
  delete ui;
}

void ConfigEditor::refreshScreensList() {
  QList<QScreen *> screens = QGuiApplication::screens();
  ui->screens->clear();
  for (int i = 0; i < screens.size(); i++) {
    QString screenName =
        screens.at(i)->name().replace("\\", "").replace(".", " ").replace("  ",
                                                                          " ");
    ui->screens->addItem(QString(tr("Monitor %1")).arg(screenName));
  }
}

void ConfigEditor::setScript(Script *script) {
  m_styleEditor->setScript(script);
  // Refresh the list of styles from script
  m_styleEditor->reset();
  // Emit events as if current config was changed
  apply();
}

void ConfigEditor::mouseReleaseEvent(QMouseEvent *event) {
  // Exit editable mode of preset when clicking elsewhere.
  QWidget *clicked = this->childAt(event->pos());
  if (clicked != ui->presetsGroupBox) {
    ui->presets->setEditable(false);
  }
}

bool ConfigEditor::eventFilter(QObject *object, QEvent *event) {
  if (object == ui->presets) {
    // Exit editable mode of preset when pressing Enter/Return.
    if (event->type() == QEvent::KeyPress) {
      QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
      if (keyEvent->key() == Qt::Key_Enter ||
          keyEvent->key() == Qt::Key_Return) {
        ui->presets->setEditable(false);
      }
    }
    // Enter editable mode of presets on right click or double click.
    if (!ui->presets->isEditable()) {
      bool toggleEdit = false;
      if (event->type() == QEvent::MouseButtonRelease) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::RightButton) {
          toggleEdit = true;
        }
      }
      if (event->type() == QEvent::MouseButtonDblClick) {
        toggleEdit = true;
      }
      if (toggleEdit) {
        ui->presets->setEditable(toggleEdit);
      }
    }
  }
  return QWidget::eventFilter(object, event);
}

void ConfigEditor::presetChanged(int p_preset) {
  if (m_preset >= 0) // Do not save while constructing
    save();
  m_styleEditor->setPreset(p_preset);
  m_preset = p_preset;
  reset();
}

void ConfigEditor::presetRenamed(QString text) {
  ui->presets->setItemText(m_preset, text);
  save();
}

void ConfigEditor::projectionWindowChanged(int monitor, const QRect &r) {
  // Show values in form
  ui->screens->blockSignals(true);
  ui->screens->setCurrentIndex(monitor);
  ui->screens->blockSignals(false);
  ui->x->blockSignals(true);
  ui->x->setValue(r.x());
  ui->x->blockSignals(false);
  ui->y->blockSignals(true);
  ui->y->setValue(r.y());
  ui->y->blockSignals(false);
  ui->w->blockSignals(true);
  ui->w->setValue(r.width());
  ui->w->blockSignals(false);
  ui->h->blockSignals(true);
  ui->h->setValue(r.height());
  ui->h->blockSignals(false);
  // We blocked signals to avoid apply() to be called. Update UI accordinly
  // then:
  enableButtonBox(true, true, true);
}

void ConfigEditor::chooseColor() {
  QPushButton *sender = static_cast<QPushButton *>(QObject::sender());
  // Show color chooser
  QColor chosen =
      QColorDialog::getColor(m_color, this
#if (QT_VERSION >= QT_VERSION_CHECK(4, 5, 0))
                             ,
                             tr("Select Color"), QColorDialog::ShowAlphaChannel
#endif
      );
  if (chosen.isValid()) {
    setColor(sender, chosen);
    if (sender == ui->btnColor)
      m_color = chosen;
    else if (sender == ui->btnOutlineColor)
      m_outlineColor = chosen;
    apply();
  }
}

void ConfigEditor::setColor(QPushButton *button, const QColor &c) {
  QPixmap pm(24, 24);
  QPainter p(&pm);
  p.setPen(c);
  p.setBrush(c);
  p.drawRect(0, 0, 24, 24);
  button->setIcon(pm);
}

void ConfigEditor::enableWeblive(bool p_state) {
  ui->groupWebLive->setVisible(p_state);
  ui->iconWeblive->setVisible(p_state);
  ui->urlWeblive->setVisible(p_state);
  ui->enableWeblive->setEnabled(p_state);
}

void ConfigEditor::webliveConnected(bool p_state, QString p_url) {
  QString message = QString("<a href=\"%1\">%1</a>");
  if (!p_state) {
    message = QString("<span style=\"color: red\">%1</span>");
  }
  ui->urlWeblive->setText(message.arg(p_url));
  ui->iconWeblive->setPixmap(
      QPixmap(p_state ? ":/icons/on.svg" : ":/icons/off.svg"));
}

void ConfigEditor::restore() {
  // Apply default screen size
  if (ui->tabs->currentWidget() == ui->tabScreen) {
    QSettings settings;
    settings.remove(QString("ScreenGeometry-%1").arg(m_preset));
  } else {
    m_styleEditor->restore();
  }
  reset();
  enableButtonBox(false, false, false);
}

void ConfigEditor::reset() {
  QList<QScreen *> screens = QGuiApplication::screens();
  int nbScreens = screens.size();
  int defaultScreen = nbScreens > 1 ? 1 : 0;
  // Reload from settings
  QSettings settings;
  settings.beginGroup(QString("ScreenGeometry-%1").arg(m_preset));
  int presetScreen = settings.value("screen", defaultScreen).toInt();
  int screen = qBound(0, presetScreen, nbScreens);
  QRect screenGeom = screens.at(screen)->geometry();
  int x = settings.value("x", 0).toInt();
  int y =
      settings
          .value("y", screenGeom.height() - DEFAULT_PROJECTION_WINDOW_HEIGHT)
          .toInt();
  int w = settings.value("w", screenGeom.width()).toInt();
  int h = settings.value("h", DEFAULT_PROJECTION_WINDOW_HEIGHT).toInt();
  double rotation = settings.value("rotation", 0).toDouble();
  QColor color(settings.value("color", DEFAULT_COLOR).toString());
  QColor outlineColor(
      settings.value("outline-color", DEFAULT_OUTLINE_COLOR).toString());
  int outlineWidth =
      settings.value("outline-width", DEFAULT_OUTLINE_WIDTH).toInt();
  settings.endGroup();
  // Update the UI with the reloaded settings
  ui->screens->setCurrentIndex(screen);
  this->projectionWindowChanged(screen, QRect(x, y, w, h));
  ui->rotation->setValue(rotation);
  setColor(ui->btnColor, color);
  m_color = color;
  setColor(ui->btnOutlineColor, outlineColor);
  m_outlineColor = outlineColor;
  ui->outlineWidth->setValue(outlineWidth);
  ui->chkOutline->setChecked(outlineWidth > 0);
  apply();
  m_styleEditor->reset();
  enableButtonBox(true, false, false);
}

void ConfigEditor::save() {
  // Save settings
  QSettings settings;
  settings.beginGroup(QString("ScreenGeometry-%1").arg(m_preset));
  settings.setValue("name", ui->presets->itemText(m_preset));
  settings.setValue("screen", qMax(0, ui->screens->currentIndex()));
  settings.setValue("x", ui->x->text());
  settings.setValue("y", ui->y->text());
  settings.setValue("w", ui->w->text());
  settings.setValue("h", ui->h->text());
  settings.setValue("rotation", ui->rotation->value());
  settings.setValue("color", m_color.name());
  settings.setValue("outline-color", m_outlineColor.name());
  settings.setValue("outline-width", ui->outlineWidth->value());
  settings.endGroup();
  m_styleEditor->save();
  // Settings saved, update the UI
  enableButtonBox(true, false, false);
}

void ConfigEditor::apply() {
  int screen = ui->screens->currentIndex();
  QRect r(ui->x->text().toInt(), ui->y->text().toInt(), ui->w->text().toInt(),
          ui->h->text().toInt());
  emit changeScreen(screen, r);
  emit rotate(ui->rotation->value());
  emit color(m_color);
  emit outline(m_outlineColor,
               ui->chkOutline->isChecked() ? ui->outlineWidth->value() : 0);
  m_styleEditor->apply();
  enableButtonBox(true, true, true);
}

void ConfigEditor::onClicked(QAbstractButton *btn) {
  if (ui->buttonBox->buttonRole(btn) == ui->buttonBox->ResetRole) {
    restore(); // ResetRole == RestoreDefaults button (sic)
  } else if (ui->buttonBox->buttonRole(btn) == ui->buttonBox->AcceptRole) {
    save();
  } else if (ui->buttonBox->buttonRole(btn) == ui->buttonBox->RejectRole) {
    reset();
  }
}

void ConfigEditor::enableButtonBox(bool restore, bool cancel, bool save) {
  // Show preset as unsaved in combobox.
  for (int i = 0; i < ui->presets->count(); i++) {
    ui->presets->setItemText(i, ui->presets->itemText(i).replace("*", ""));
  }
  if (save) {
    QString current = ui->presets->currentText().append("*");
    ui->presets->setItemText(ui->presets->currentIndex(), current);
  }

  // Show current tab as unsaved.
  if (save) {
    QString current = ui->tabs->tabText(ui->tabs->currentIndex())
                          .replace("*", "")
                          .append("*");
    ui->tabs->setTabText(ui->tabs->currentIndex(), current);
  } else {
    for (int i = 0; i < ui->tabs->count(); i++) {
      ui->tabs->setTabText(i, ui->tabs->tabText(i).replace("*", ""));
    }
  }

  ui->buttonBox->button(QDialogButtonBox::RestoreDefaults)->setEnabled(restore);
  ui->buttonBox->button(QDialogButtonBox::Cancel)->setEnabled(cancel);
  ui->buttonBox->button(QDialogButtonBox::Save)->setEnabled(save);
}
