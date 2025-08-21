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
          &RemoteOptionsDialog::onCopyUrl);

  refreshLabelsAndQr(); // will be empty until service emits signals
}

RemoteOptionsDialog::~RemoteOptionsDialog() { delete m_ui; }

void RemoteOptionsDialog::onSettingsLoaded(const bool enabled, quint16 httpPort,
                                           quint16 webSocketPort) {
  m_ui->spinBoxHttpPort->blockSignals(true);
  m_ui->spinBoxHttpPort->setValue(httpPort);
  m_ui->spinBoxHttpPort->blockSignals(false);

  m_ui->spinBoxWebSocketPort->blockSignals(true);
  m_ui->spinBoxWebSocketPort->setValue(webSocketPort);
  m_ui->spinBoxWebSocketPort->blockSignals(false);

  m_ui->checkBoxRemoteScreensEnabled->blockSignals(true);
  m_ui->checkBoxRemoteScreensEnabled->setChecked(enabled);
  m_ui->checkBoxRemoteScreensEnabled->blockSignals(false);

  setControlsEnabled(enabled);
  refreshLabelsAndQr();
}

void RemoteOptionsDialog::onServiceStarted(const QString &url) {
  m_currentUrl = url;
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

void RemoteOptionsDialog::onClientsConnected(quint16 count) {
  m_clientsConnected = count;
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
    if (m_clientsConnected > 0) {
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
    m_currentUrl.clear();
    emit disableRequested();
  } else {

    emit startRequested(m_ui->spinBoxHttpPort->value(),
                        m_ui->spinBoxWebSocketPort->value());
  }

  refreshLabelsAndQr();
}

void RemoteOptionsDialog::onCopyUrl() {
  if (!m_currentUrl.isEmpty())
    QGuiApplication::clipboard()->setText(m_currentUrl);
}

void RemoteOptionsDialog::setControlsEnabled(bool running) {
  m_ui->spinBoxHttpPort->setEnabled(!running);
  m_ui->spinBoxWebSocketPort->setEnabled(!running);
  m_ui->pushButtonCopyUrlViewers->setEnabled(running);
}

void RemoteOptionsDialog::refreshLabelsAndQr() {
  if (m_currentUrl.isEmpty()) {
    m_ui->labelUrlViewers->clear();
    m_ui->labelQrCodeViewers->clear();
    m_ui->labelConnectedCountViewers->clear();
    return;
  }
  m_ui->labelUrlViewers->setText(
      QString("URL: <a href=\"%1\">%1</a>").arg(m_currentUrl));

  m_ui->labelConnectedCountViewers->setText(
      QString("Clients connected: %1").arg(m_clientsConnected));

  // Generate at the exact display size
  m_ui->labelQrCodeViewers->setScaledContents(false);

  // Figure out target points size (account for Retina DPR)
  const QRect r = m_ui->labelQrCodeViewers->contentsRect();

  QImage img = makeQrImage(m_currentUrl, r);
  m_ui->labelQrCodeViewers->setPixmap(QPixmap::fromImage(img));
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
