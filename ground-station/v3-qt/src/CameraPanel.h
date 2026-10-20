#pragma once

#include <QWidget>
#include <QCameraDevice>
#include <QVideoFrame>

QT_BEGIN_NAMESPACE
class QComboBox;
class QPushButton;
class QLabel;
class QStackedLayout;
class QCamera;
class QMediaDevices;
class QMediaCaptureSession;
class QMediaRecorder;
class QVideoWidget;
class QTimer;
QT_END_NAMESPACE

/**
 * CameraPanel
 * -----------
 * Live view of the CubeSat's onboard camera (VTX receiver / capture
 * dongle enumerates as a standard UVC webcam on Windows).
 *
 * Self-contained: only touches its own widgets. Does not know about
 * SerialManager / TelemetryParser / the telemetry pipeline at all.
 *
 * Built on QtMultimedia (QCamera + QMediaCaptureSession + QVideoWidget)
 * rather than OpenCV -- frames arrive asynchronously from the platform
 * camera backend, so there is no manual read-loop to keep off the UI
 * thread; Start/Stop just toggles QCamera::setActive().
 */
class CameraPanel : public QWidget
{
    Q_OBJECT

public:
    explicit CameraPanel(QWidget *parent = nullptr);
    ~CameraPanel() override;

private slots:
    void onRefreshClicked();
    void onStartStopClicked();
    void onSnapshotClicked();
    void onRecordClicked();
    void onCameraActiveChanged(bool active);
    void onCameraErrorOccurred();
    void onVideoFrameChanged(const QVideoFrame &frame);
    void onFpsTick();

private:
    void buildUi();
    void populateDevices();
    void startCamera();
    void stopCamera();
    void setStatus(const QString &text, const QString &color);

    QComboBox      *m_deviceCombo{nullptr};
    QPushButton    *m_refreshBtn{nullptr};
    QPushButton    *m_startStopBtn{nullptr};
    QPushButton    *m_snapshotBtn{nullptr};
    QPushButton    *m_recordBtn{nullptr};
    QLabel         *m_statusLabel{nullptr};
    QLabel         *m_fpsLabel{nullptr};
    QLabel         *m_placeholder{nullptr};
    QStackedLayout *m_stack{nullptr};

    QMediaDevices        *m_mediaDevices{nullptr};
    QCamera              *m_camera{nullptr};
    QMediaCaptureSession *m_session{nullptr};
    QMediaRecorder       *m_recorder{nullptr};
    QVideoWidget         *m_videoWidget{nullptr};
    QTimer               *m_fpsTimer{nullptr};

    QList<QCameraDevice> m_devices;
    QVideoFrame m_lastFrame;
    bool m_gotFirstFrame{false};
    bool m_recording{false};
    int  m_frameCount{0};
};
