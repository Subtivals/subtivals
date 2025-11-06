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
  void setPassphrase(QString passphrase);

public slots:
  void onServiceStarted(const QString &urlViewers, const QString &urlControl);
  void onServiceStopped();
  void onServiceError(const QString &message);
  void onSettingsLoaded(const bool enabled, quint16 httpPort,
                        quint16 webSocketPort, const QString &passphrase);
  void onClientsConnected(quint16 countViewers, quint16 countControl);

private slots:
  void onRemoteScreensToggled(bool enabled);
  void onCopyViewersUrl();
  void onCopyControlUrl();

private:
  void setControlsEnabled(bool running);
  void refreshLabelsAndQr();
  QImage makeQrImage(const QString &text, const QRect &rect,
                     int border = 2) const;

private:
  QString m_urlViewers;
  QString m_urlControl;
  int m_viewersConnected = 0;
  int m_controlConnected = 0;
  Ui::RemoteOptionsDialog *m_ui = nullptr;
};
