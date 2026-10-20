#include "CameraPanel.h"

#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QStackedLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QTimer>
#include <QDateTime>
#include <QImage>

#include <QMediaDevices>
#include <QCamera>
#include <QMediaCaptureSession>
#include <QMediaRecorder>
#include <QVideoWidget>
#include <QVideoSink>

// ── Ctor / Dtor ───────────────────────────────────────────────────────────────

CameraPanel::CameraPanel(QWidget *parent)
    : QWidget(parent)
{
    buildUi();

    m_mediaDevices = new QMediaDevices(this);
    connect(m_mediaDevices, &QMediaDevices::videoInputsChanged,
            this, &CameraPanel::populateDevices);

    m_session = new QMediaCaptureSession(this);
    m_session->setVideoOutput(m_videoWidget);

    m_recorder = new QMediaRecorder(this);
    m_session->setRecorder(m_recorder);

    connect(m_videoWidget->videoSink(), &QVideoSink::videoFrameChanged,
            this, &CameraPanel::onVideoFrameChanged);

    m_fpsTimer = new QTimer(this);
    m_fpsTimer->setInterval(1000);
    connect(m_fpsTimer, &QTimer::timeout, this, &CameraPanel::onFpsTick);
    m_fpsTimer->start();

    populateDevices();
}

CameraPanel::~CameraPanel()
{
    stopCamera();
}

// ── UI ────────────────────────────────────────────────────────────────────────

void CameraPanel::buildUi()
{
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(6);

    // ── Controls row ─────────────────────────────────────────────────────────
    auto *group = new QGroupBox("Camera Feed");
    auto *gl = new QHBoxLayout(group);
    gl->setSpacing(6);

    gl->addWidget(new QLabel("Device:"));
    m_deviceCombo = new QComboBox;
    m_deviceCombo->setMinimumWidth(160);
    gl->addWidget(m_deviceCombo);

    m_refreshBtn = new QPushButton("Refresh");
    connect(m_refreshBtn, &QPushButton::clicked, this, &CameraPanel::onRefreshClicked);
    gl->addWidget(m_refreshBtn);

    m_startStopBtn = new QPushButton("Start");
    connect(m_startStopBtn, &QPushButton::clicked, this, &CameraPanel::onStartStopClicked);
    gl->addWidget(m_startStopBtn);

    m_snapshotBtn = new QPushButton("Snapshot");
    m_snapshotBtn->setEnabled(false);
    connect(m_snapshotBtn, &QPushButton::clicked, this, &CameraPanel::onSnapshotClicked);
    gl->addWidget(m_snapshotBtn);

    m_recordBtn = new QPushButton("Record");
    m_recordBtn->setEnabled(false);
    connect(m_recordBtn, &QPushButton::clicked, this, &CameraPanel::onRecordClicked);
    gl->addWidget(m_recordBtn);

    gl->addStretch();

    m_fpsLabel = new QLabel("FPS: --");
    m_fpsLabel->setStyleSheet("color: #AAAAAA; font-family: Consolas;");
    gl->addWidget(m_fpsLabel);

    m_statusLabel = new QLabel("No camera selected");
    m_statusLabel->setStyleSheet("color: #AAAAAA;");
    gl->addWidget(m_statusLabel);

    outer->addWidget(group);

    // ── Video area ───────────────────────────────────────────────────────────
    auto *videoContainer = new QWidget;
    videoContainer->setMinimumHeight(220);
    m_stack = new QStackedLayout(videoContainer);
    m_stack->setContentsMargins(0, 0, 0, 0);

    m_placeholder = new QLabel("[ NO LIVE FEED ]");
    m_placeholder->setAlignment(Qt::AlignCenter);
    m_placeholder->setStyleSheet(
        "background-color: #101010; color: #555555;"
        "font-family: Consolas; font-size: 11pt; border: 1px solid #2A2A2A;"
    );

    m_videoWidget = new QVideoWidget;
    m_videoWidget->setStyleSheet("background-color: #000000;");

    m_stack->addWidget(m_placeholder);
    m_stack->addWidget(m_videoWidget);
    m_stack->setCurrentWidget(m_placeholder);

    outer->addWidget(videoContainer, 1);
}

// ── Device handling ──────────────────────────────────────────────────────────

void CameraPanel::populateDevices()
{
    const QString previouslySelected = m_deviceCombo->currentData().toString();

    m_devices = QMediaDevices::videoInputs();
    m_deviceCombo->clear();

    if (m_devices.isEmpty()) {
        m_deviceCombo->addItem("NO CAMERA DETECTED");
        m_startStopBtn->setEnabled(false);
        setStatus("No capture devices found", "#E74C3C");
        return;
    }

    for (const QCameraDevice &dev : m_devices)
        m_deviceCombo->addItem(dev.description(), QString::fromUtf8(dev.id()));

    m_startStopBtn->setEnabled(true);

    const int idx = m_deviceCombo->findData(previouslySelected);
    if (idx >= 0) m_deviceCombo->setCurrentIndex(idx);

    if (!m_camera)
        setStatus(QString("%1 device(s) available").arg(m_devices.size()), "#AAAAAA");
}

void CameraPanel::onRefreshClicked()
{
    populateDevices();
}

// ── Start / stop ─────────────────────────────────────────────────────────────

void CameraPanel::onStartStopClicked()
{
    if (m_camera && m_camera->isActive())
        stopCamera();
    else
        startCamera();
}

void CameraPanel::startCamera()
{
    const int idx = m_deviceCombo->currentIndex();
    if (idx < 0 || idx >= m_devices.size()) {
        setStatus("No valid camera selected", "#E74C3C");
        return;
    }

    if (m_camera) {
        m_camera->deleteLater();
        m_camera = nullptr;
    }

    m_camera = new QCamera(m_devices[idx], this);
    connect(m_camera, &QCamera::activeChanged, this, &CameraPanel::onCameraActiveChanged);
    connect(m_camera, &QCamera::errorOccurred, this, &CameraPanel::onCameraErrorOccurred);

    m_session->setCamera(m_camera);
    m_gotFirstFrame = false;
    m_frameCount = 0;
    m_camera->setActive(true);

    m_deviceCombo->setEnabled(false);
    setStatus(QString("Connecting to %1…").arg(m_deviceCombo->currentText()), "#F39C12");
}

void CameraPanel::stopCamera()
{
    if (m_recording)
        onRecordClicked(); // stop + release recorder cleanly first

    if (m_camera) {
        m_camera->setActive(false);
        m_session->setCamera(nullptr);
        m_camera->deleteLater();
        m_camera = nullptr;
    }

    m_gotFirstFrame = false;
    m_lastFrame = QVideoFrame();
    m_stack->setCurrentWidget(m_placeholder);

    m_startStopBtn->setText("Start");
    m_snapshotBtn->setEnabled(false);
    m_recordBtn->setEnabled(false);
    m_deviceCombo->setEnabled(true);
    m_fpsLabel->setText("FPS: --");
    setStatus("Stopped", "#AAAAAA");
}

// ── Camera signals ───────────────────────────────────────────────────────────

void CameraPanel::onCameraActiveChanged(bool active)
{
    if (active) {
        m_startStopBtn->setText("Stop");
        m_snapshotBtn->setEnabled(true);
        m_recordBtn->setEnabled(true);
        setStatus(QString("LIVE — %1").arg(m_deviceCombo->currentText()), "#27AE60");
    }
}

void CameraPanel::onCameraErrorOccurred()
{
    if (!m_camera) return;
    setStatus("Camera error: " + m_camera->errorString(), "#E74C3C");
    stopCamera();
}

void CameraPanel::onVideoFrameChanged(const QVideoFrame &frame)
{
    if (!frame.isValid()) return;

    m_lastFrame = frame;
    ++m_frameCount;

    if (!m_gotFirstFrame) {
        m_gotFirstFrame = true;
        m_stack->setCurrentWidget(m_videoWidget);
    }
}

void CameraPanel::onFpsTick()
{
    if (m_camera && m_camera->isActive())
        m_fpsLabel->setText(QString("FPS: %1").arg(m_frameCount));
    m_frameCount = 0;
}

// ── Snapshot / record ────────────────────────────────────────────────────────

void CameraPanel::onSnapshotClicked()
{
    if (!m_lastFrame.isValid()) {
        setStatus("No frame available yet", "#F39C12");
        return;
    }

    const QImage img = m_lastFrame.toImage();
    if (img.isNull()) {
        setStatus("Snapshot failed — empty frame", "#E74C3C");
        return;
    }

    const QString filename = QString("CubeSat_SNAP_%1.png")
        .arg(QDateTime::currentSecsSinceEpoch());

    if (img.save(filename))
        setStatus("Snapshot saved: " + filename, "#27AE60");
    else
        setStatus("Snapshot failed to save: " + filename, "#E74C3C");
}

void CameraPanel::onRecordClicked()
{
    if (!m_recording) {
        const QString filename = QString("CubeSat_REC_%1.mp4")
            .arg(QDateTime::currentSecsSinceEpoch());
        m_recorder->setOutputLocation(QUrl::fromLocalFile(filename));
        m_recorder->record();
        m_recording = true;
        m_recordBtn->setText("Stop Rec");
        setStatus("Recording to " + filename, "#E74C3C");
    } else {
        m_recorder->stop();
        m_recording = false;
        m_recordBtn->setText("Record");
        setStatus("Recording stopped", "#AAAAAA");
    }
}

// ── Helpers ───────────────────────────────────────────────────────────────────

void CameraPanel::setStatus(const QString &text, const QString &color)
{
    m_statusLabel->setText(text);
    m_statusLabel->setStyleSheet(QString("color: %1;").arg(color));
}
