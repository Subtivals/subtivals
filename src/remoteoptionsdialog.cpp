#include <QClipboard>
#include <QGuiApplication>
#include <QMessageBox>
#include <QPainter>

#include "qrcodegen.hpp"
#include "remoteoptionsdialog.h"
#include "ui_remoteoptionsdialog.h"

using qrcodegen::QrCode;

RemoteOptionsDialog::RemoteOptionsDialog(QWidget *parent) : QDialog(parent) {
  m_ui = new Ui::RemoteOptionsDialog;
  m_ui->setupUi(this);

  connect(m_ui->checkBoxRemoteScreensEnabled, &QCheckBox::toggled, this,
          &RemoteOptionsDialog::onRemoteScreensToggled);
  connect(m_ui->pushButtonCopyUrlViewers, &QPushButton::clicked, this,
          &RemoteOptionsDialog::onCopyViewersUrl);
  connect(m_ui->pushButtonCopyUrlControl, &QPushButton::clicked, this,
          &RemoteOptionsDialog::onCopyControlUrl);
  connect(m_ui->lineEditPassphrase, &QLineEdit::textEdited, this,
          &RemoteOptionsDialog::setPassphrase);

  refreshLabelsAndQr(); // will be empty until service emits signals
}

RemoteOptionsDialog::~RemoteOptionsDialog() { delete m_ui; }

void RemoteOptionsDialog::onSettingsLoaded(const bool enabled, quint16 httpPort,
                                           quint16 webSocketPort,
                                           const QString &passphrase) {
  m_ui->spinBoxHttpPort->blockSignals(true);
  m_ui->spinBoxHttpPort->setValue(httpPort);
  m_ui->spinBoxHttpPort->blockSignals(false);

  m_ui->spinBoxWebSocketPort->blockSignals(true);
  m_ui->spinBoxWebSocketPort->setValue(webSocketPort);
  m_ui->spinBoxWebSocketPort->blockSignals(false);

  m_ui->checkBoxRemoteScreensEnabled->blockSignals(true);
  m_ui->checkBoxRemoteScreensEnabled->setChecked(enabled);
  m_ui->checkBoxRemoteScreensEnabled->blockSignals(false);

  m_ui->lineEditPassphrase->blockSignals(true);
  m_ui->lineEditPassphrase->setText(passphrase);
  m_ui->lineEditPassphrase->blockSignals(false);

  setControlsEnabled(enabled);
  refreshLabelsAndQr();
}

void RemoteOptionsDialog::onServiceStarted(const QString &urlViewers,
                                           const QString &urlControl) {
  m_urlViewers = urlViewers;
  m_urlControl = urlControl;
  m_ui->checkBoxRemoteScreensEnabled->blockSignals(true);
  m_ui->checkBoxRemoteScreensEnabled->setEnabled(true);
  m_ui->checkBoxRemoteScreensEnabled->blockSignals(false);
  setControlsEnabled(true);
  refreshLabelsAndQr();
}

void RemoteOptionsDialog::onServiceStopped() {
  setControlsEnabled(false);
  refreshLabelsAndQr();
}

void RemoteOptionsDialog::onClientsConnected(quint16 countViewers,
                                             quint16 countControl) {
  m_viewersConnected = countViewers;
  m_controlConnected = countControl;
  refreshLabelsAndQr();
}

void RemoteOptionsDialog::onServiceError(const QString &message) {
  QMessageBox::critical(this, tr("Remote Screens Error"), message);
  if (m_ui->checkBoxRemoteScreensEnabled->isChecked()) {
    m_ui->checkBoxRemoteScreensEnabled->blockSignals(true);
    m_ui->checkBoxRemoteScreensEnabled->setChecked(false);
    m_ui->checkBoxRemoteScreensEnabled->blockSignals(false);
  }
  onServiceStopped();
}

void RemoteOptionsDialog::onRemoteScreensToggled(bool enabled) {
  if (!enabled) {
    if (m_viewersConnected > 0 || m_controlConnected > 0) {
      const auto ret = QMessageBox::warning(
          this, tr("Disable remote screens"),
          tr("This will close all remote screens and stop the "
             "servers.\nContinue?"),
          QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

      if (ret != QMessageBox::Yes) {
        m_ui->checkBoxRemoteScreensEnabled->blockSignals(true);
        m_ui->checkBoxRemoteScreensEnabled->setChecked(true);
        m_ui->checkBoxRemoteScreensEnabled->blockSignals(false);
        return;
      }
    }
    m_urlViewers.clear();
    m_urlControl.clear();
    emit disableRequested();
  } else {
    emit startRequested(m_ui->spinBoxHttpPort->value(),
                        m_ui->spinBoxWebSocketPort->value());
  }

  refreshLabelsAndQr();
}

void RemoteOptionsDialog::onCopyViewersUrl() {
  if (!m_urlViewers.isEmpty())
    QGuiApplication::clipboard()->setText(m_urlViewers);
}

void RemoteOptionsDialog::onCopyControlUrl() {
  if (!m_urlControl.isEmpty())
    QGuiApplication::clipboard()->setText(m_urlControl);
}

void RemoteOptionsDialog::setControlsEnabled(bool running) {
  m_ui->spinBoxHttpPort->setEnabled(!running);
  m_ui->spinBoxWebSocketPort->setEnabled(!running);
  m_ui->pushButtonCopyUrlViewers->setEnabled(running);
  m_ui->pushButtonCopyUrlControl->setEnabled(running);
}

void RemoteOptionsDialog::refreshLabelsAndQr() {
  if (m_urlViewers.isEmpty() || m_urlControl.isEmpty()) {
    m_ui->labelUrlViewers->clear();
    m_ui->labelQrCodeViewers->clear();
    m_ui->labelConnectedCountViewers->clear();

    m_ui->labelUrlControl->clear();
    m_ui->labelQrCodeControl->clear();
    m_ui->labelConnectedCountControl->clear();
    return;
  }
  m_ui->labelUrlViewers->setText(
      QString("URL: <a href=\"%1\">%1</a>").arg(m_urlViewers));
  m_ui->labelConnectedCountViewers->setText(
      QString("Clients connected: %1").arg(m_viewersConnected));

  m_ui->labelUrlControl->setText(
      QString("URL: <a href=\"%1\">%1</a>").arg(m_urlControl));
  m_ui->labelConnectedCountControl->setText(
      QString("Clients connected: %1").arg(m_controlConnected));

  // Generate at the exact display size
  // Figure out target points size (account for Retina DPR)
  m_ui->labelQrCodeViewers->setScaledContents(false);
  const QRect rV = m_ui->labelQrCodeViewers->contentsRect();
  QImage imgV = makeQrImage(m_urlViewers, rV);
  m_ui->labelQrCodeViewers->setPixmap(QPixmap::fromImage(imgV));

  m_ui->labelQrCodeControl->setScaledContents(false);
  const QRect rC = m_ui->labelQrCodeControl->contentsRect();
  QImage imgC = makeQrImage(m_urlControl, rC);
  m_ui->labelQrCodeControl->setPixmap(QPixmap::fromImage(imgC));
}

QImage RemoteOptionsDialog::makeQrImage(const QString &text, const QRect &rect,
                                        int border) const {
  // Encode
  const auto data = text.toUtf8().toStdString();
  const QrCode qr = QrCode::encodeText(data.c_str(), QrCode::Ecc::LOW);
  const int modules = qr.getSize(); // module count per side
  const int sideLogical = qMin(rect.width(), rect.height());
  const int sideDevice = qMax(1, int(std::floor(sideLogical)));
  const int neededModules = modules + border * 2;
  int moduleScale = qMax(1, sideDevice / neededModules);
  const int sizePoints = neededModules * moduleScale;

  // reate DPR-aware image, no antialiasing, no interpolation
  QImage img(sizePoints, sizePoints, QImage::Format_ARGB32_Premultiplied);
  img.fill(Qt::white);

  QPainter p(&img);
  p.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing, false);
  p.setPen(Qt::NoPen);
  p.setBrush(Qt::black);

  // 5) Paint each “on” module as an aligned rect
  for (int y = 0; y < modules; ++y) {
    for (int x = 0; x < modules; ++x) {
      if (qr.getModule(x, y)) {
        const int px = (x + border) * moduleScale;
        const int py = (y + border) * moduleScale;
        p.drawRect(px, py, moduleScale, moduleScale);
      }
    }
  }
  p.end();
  return img;
}
