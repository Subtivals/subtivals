#pragma once
#include <QDialog>
#include <QImage>

QT_BEGIN_NAMESPACE
namespace Ui {
  class RemoteOptionsDialog;
}
QT_END_NAMESPACE

class RemoteOptionsDialog : public QDialog {
  Q_OBJECT
public:
  explicit RemoteOptionsDialog(QWidget *parent = nullptr);
  ~RemoteOptionsDialog() override;

signals:
  void startRequested(quint16 httpPort, quint16 webSocketPort);
  void disableRequested();

public slots:
  void onServiceStarted(const QString &url);
  void onServiceStopped();
  void onServiceError(const QString &message);
  void onSettingsLoaded(const bool enabled, quint16 httpPort,
                        quint16 webSocketPort);
  void onClientsConnected(quint16 count);

private slots:
  void onRemoteScreensToggled(bool enabled);
  void onCopyUrl();

private:
  void setControlsEnabled(bool running);
  void refreshLabelsAndQr();
  QImage makeQrImage(const QString &text, const QRect &rect,
                     int border = 2) const;

private:
  QString m_currentUrl;
  int m_clientsConnected = 0;
  Ui::RemoteOptionsDialog *m_ui = nullptr;
};
