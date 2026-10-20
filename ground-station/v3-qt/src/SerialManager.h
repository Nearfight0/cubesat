#pragma once

#include "TelemetryPacket.h"

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QString>
#include <QStringList>

QT_BEGIN_NAMESPACE
class QSerialPort;
QT_END_NAMESPACE

// ── Worker (lives on its own QThread) ─────────────────────────────────────────

/**
 * SerialWorker
 * ------------
 * Runs on a dedicated QThread. Opens the serial port, reads lines, and
 * emits them back to the main thread. All blocking serial I/O happens here.
 * MainWindow / SerialManager never touch QSerialPort directly.
 */
class SerialWorker : public QObject
{
    Q_OBJECT

public:
    explicit SerialWorker(QObject *parent = nullptr);
    ~SerialWorker() override;

public slots:
    void openPort(const QString &portName, int baudRate);
    void closePort();

signals:
    void lineReceived(const QString &line);   ///< Raw text line from serial
    void errorOccurred(const QString &msg);   ///< Human-readable error
    void portOpened();                         ///< Emitted on successful open
    void portClosed();                         ///< Emitted after close

private slots:
    void onReadyRead();

private:
    QSerialPort *m_serial{nullptr};
};

// ── Manager (lives on the main thread) ───────────────────────────────────────

/**
 * SerialManager
 * -------------
 * Owned by MainWindow. Manages the lifecycle of SerialWorker + its thread.
 * Parses raw lines into TelemetryPacket and emits packetReceived().
 * Implements auto-reconnect via a QTimer.
 *
 * Usage:
 *   1. Call refreshPorts() to populate port list.
 *   2. Call connectToPort(name, baud) to open.
 *   3. Listen to packetReceived(), rawLineReceived(), statusChanged().
 *   4. Call disconnectPort() to close.
 */
class SerialManager : public QObject
{
    Q_OBJECT

public:
    explicit SerialManager(QObject *parent = nullptr);
    ~SerialManager() override;

    /** Returns currently available COM port names. */
    static QStringList availablePorts();

    bool isConnected() const { return m_connected; }
    QString currentPort() const { return m_portName; }
    int currentBaudRate() const { return m_baudRate; }

public slots:
    void connectToPort(const QString &portName, int baudRate);
    void disconnectPort();
    void refreshPorts(); ///< Re-scans system COM ports (emits portsUpdated)

signals:
    /** Emitted for every successfully parsed telemetry frame. */
    void packetReceived(const TelemetryPacket &packet);

    /** Raw text line — shown in the Raw Data Log tab. */
    void rawLineReceived(const QString &line);

    /** "Connected" / "Disconnected" / error text for status bar. */
    void statusChanged(const QString &status, bool connected);

    /** Updated list of COM ports after refreshPorts(). */
    void portsUpdated(const QStringList &ports);

    /** Emitted when a parse error occurs (for the raw log). */
    void parseError(const QString &error);

private slots:
    void onLineReceived(const QString &line);
    void onWorkerError(const QString &msg);
    void onPortOpened();
    void onPortClosed();
    void attemptReconnect();

private:
    void startWorker();
    void stopWorker();
    void scheduleReconnect();

    QThread       *m_thread{nullptr};
    SerialWorker  *m_worker{nullptr};
    QTimer        *m_reconnectTimer{nullptr};

    QString  m_portName;
    int      m_baudRate{9600};
    bool     m_connected{false};
    bool     m_intentionalDisconnect{false};
    bool     m_scanInProgress{false}; ///< Prevent overlapping async port scans

    static constexpr int RECONNECT_INTERVAL_MS = 3000;
};
