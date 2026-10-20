#include "SerialManager.h"
#include "TelemetryParser.h"

#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTimer>
#include <QDebug>
#include <QtConcurrent/QtConcurrent>

// ═════════════════════════════════════════════════════════════════════════════
// SerialWorker
// ═════════════════════════════════════════════════════════════════════════════

SerialWorker::SerialWorker(QObject *parent)
    : QObject(parent)
{}

SerialWorker::~SerialWorker()
{
    closePort();
}

void SerialWorker::openPort(const QString &portName, int baudRate)
{
    // If already open, close first
    if (m_serial && m_serial->isOpen()) {
        m_serial->close();
        delete m_serial;
        m_serial = nullptr;
    }

    m_serial = new QSerialPort(this);
    m_serial->setPortName(portName);
    m_serial->setBaudRate(baudRate);
    m_serial->setDataBits(QSerialPort::Data8);
    m_serial->setParity(QSerialPort::NoParity);
    m_serial->setStopBits(QSerialPort::OneStop);
    m_serial->setFlowControl(QSerialPort::NoFlowControl);

    connect(m_serial, &QSerialPort::readyRead,
            this, &SerialWorker::onReadyRead);

    if (!m_serial->open(QIODevice::ReadWrite)) {
        emit errorOccurred(
            QString("Cannot open %1: %2")
                .arg(portName)
                .arg(m_serial->errorString())
        );
        delete m_serial;
        m_serial = nullptr;
        return;
    }

    emit portOpened();
}

void SerialWorker::closePort()
{
    if (m_serial) {
        if (m_serial->isOpen())
            m_serial->close();
        delete m_serial;
        m_serial = nullptr;
    }
    emit portClosed();
}

void SerialWorker::onReadyRead()
{
    if (!m_serial) return;

    // Read all complete lines that have arrived
    while (m_serial->canReadLine()) {
        const QByteArray raw = m_serial->readLine();
        emit lineReceived(QString::fromUtf8(raw).trimmed());
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// SerialManager
// ═════════════════════════════════════════════════════════════════════════════

SerialManager::SerialManager(QObject *parent)
    : QObject(parent)
{
    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setInterval(RECONNECT_INTERVAL_MS);
    m_reconnectTimer->setSingleShot(true);
    connect(m_reconnectTimer, &QTimer::timeout,
            this, &SerialManager::attemptReconnect);
}

SerialManager::~SerialManager()
{
    stopWorker();
}

// ── Static ────────────────────────────────────────────────────────────────────

QStringList SerialManager::availablePorts()
{
    QStringList names;
    for (const QSerialPortInfo &info : QSerialPortInfo::availablePorts())
        names << info.portName();
    return names;
}

// ── Public slots ──────────────────────────────────────────────────────────────

void SerialManager::connectToPort(const QString &portName, int baudRate)
{
    m_intentionalDisconnect = false;
    m_portName = portName;
    m_baudRate = baudRate;

    // Tear down any existing worker before starting a fresh one
    stopWorker();
    startWorker();

    // Ask the worker (on its thread) to open the port
    QMetaObject::invokeMethod(m_worker, "openPort",
                              Qt::QueuedConnection,
                              Q_ARG(QString, portName),
                              Q_ARG(int, baudRate));
}

void SerialManager::disconnectPort()
{
    m_intentionalDisconnect = true;
    m_reconnectTimer->stop();
    stopWorker();

    m_connected = false;
    emit statusChanged("Disconnected", false);
}

void SerialManager::refreshPorts()
{
    // Guard: don't queue a second scan while one is already running.
    // QSerialPortInfo::availablePorts() is a blocking Windows registry call
    // that can take 1-3 s — running it off the main thread prevents UI freezes.
    if (m_scanInProgress) return;
    m_scanInProgress = true;

    QtConcurrent::run([this]() {
        QStringList ports;
        for (const QSerialPortInfo &info : QSerialPortInfo::availablePorts())
            ports << info.portName();

        // Marshal result back to the main thread via a queued call
        QMetaObject::invokeMethod(this, [this, ports]() {
            m_scanInProgress = false;
            emit portsUpdated(ports);
        }, Qt::QueuedConnection);
    });
}

// ── Private ───────────────────────────────────────────────────────────────────

void SerialManager::startWorker()
{
    m_thread = new QThread(this);
    m_worker = new SerialWorker;         // no parent — will move to thread
    m_worker->moveToThread(m_thread);

    connect(m_worker, &SerialWorker::lineReceived,
            this, &SerialManager::onLineReceived);
    connect(m_worker, &SerialWorker::errorOccurred,
            this, &SerialManager::onWorkerError);
    connect(m_worker, &SerialWorker::portOpened,
            this, &SerialManager::onPortOpened);
    connect(m_worker, &SerialWorker::portClosed,
            this, &SerialManager::onPortClosed);

    // Clean up worker when thread finishes
    connect(m_thread, &QThread::finished, m_worker, &QObject::deleteLater);

    m_thread->start();
}

void SerialManager::stopWorker()
{
    if (!m_thread) return;

    // Post closePort to the worker queue (non-blocking), then quit the loop.
    // QueuedConnection ensures closePort runs before the thread exits because
    // quit() posts a WM_QUIT *after* closePort in the queue.
    if (m_worker)
        QMetaObject::invokeMethod(m_worker, "closePort", Qt::QueuedConnection);

    m_thread->quit();

    // Wait up to 1.5 s; force-terminate only if truly stuck (e.g. driver hung)
    if (!m_thread->wait(1500)) {
        qWarning() << "[SerialManager] Worker thread did not exit — terminating.";
        m_thread->terminate();
        m_thread->wait(300);
    }

    delete m_thread;   // deleteLater chain cleans up m_worker
    m_thread = nullptr;
    m_worker = nullptr;
}

void SerialManager::scheduleReconnect()
{
    if (!m_intentionalDisconnect && !m_portName.isEmpty()) {
        emit statusChanged(
            QString("Connection lost — retrying in %1 s...")
                .arg(RECONNECT_INTERVAL_MS / 1000),
            false
        );
        m_reconnectTimer->start();
    }
}

// ── Slots (main thread) ───────────────────────────────────────────────────────

void SerialManager::onLineReceived(const QString &line)
{
    emit rawLineReceived(line);

    TelemetryPacket packet;
    QString error;

    if (TelemetryParser::parse(line, packet, error)) {
        emit packetReceived(packet);
    } else {
        emit parseError(error);
    }
}

void SerialManager::onWorkerError(const QString &msg)
{
    qWarning() << "[SerialManager] Worker error:" << msg;
    m_connected = false;
    emit statusChanged(msg, false);
    scheduleReconnect();
}

void SerialManager::onPortOpened()
{
    m_connected = true;
    m_reconnectTimer->stop();
    emit statusChanged(
        QString("Connected — %1 @ %2 baud").arg(m_portName).arg(m_baudRate),
        true
    );
}

void SerialManager::onPortClosed()
{
    if (!m_intentionalDisconnect) {
        m_connected = false;
        scheduleReconnect();
    }
}

void SerialManager::attemptReconnect()
{
    qDebug() << "[SerialManager] Attempting reconnect on" << m_portName;
    // Restart the worker and try opening again
    stopWorker();
    startWorker();
    QMetaObject::invokeMethod(m_worker, "openPort",
                              Qt::QueuedConnection,
                              Q_ARG(QString, m_portName),
                              Q_ARG(int, m_baudRate));
}
