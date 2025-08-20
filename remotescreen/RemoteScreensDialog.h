#pragma once
#include "RemoteScreensTypes.h"
#include <QDialog>
#include <QImage>

QT_BEGIN_NAMESPACE
namespace Ui { class RemoteScreensDialog; }
QT_END_NAMESPACE

class RemoteScreensDialog : public QDialog {
    Q_OBJECT
public:
    explicit RemoteScreensDialog(QWidget *parent = nullptr);
    ~RemoteScreensDialog() override;

signals:
    void startRequested(int httpPort, int webSocketPort);
    void disableRequested();

public slots:
    // Service → Dialog updates
    void onServiceStarted(const QString &url, quint16 httpPort, quint16 webSocketPort);
    void onServiceStopped();
    void onServiceError(const QString &message);
    void onSettingsLoaded(const RemoteScreensConfig &cfg);
    void clientsConnected(int count);

private slots:
    void onRemoteScreensToggled(bool enabled);
    void onCopyUrl();

private:
    void setControlsEnabled(bool running);
    void refreshLabelsAndQr();
    QImage makeQrImage(const QString &text, const QRect &rect, int border = 2) const;

private:
    QString m_currentUrl;
    int m_clientsConnected = 0;
    Ui::RemoteScreensDialog *m_ui = nullptr;
};
