#pragma once

#include "TelemetryPacket.h"

#include <QObject>
#include <QString>
#include <QFile>
#include <QTextStream>

/**
 * CsvLogger
 * ---------
 * Logs TelemetryPacket data to a CSV file named <TEAM_ID>.csv.
 *
 * Rules:
 *  - File is created on the first valid packet received.
 *  - The header row is written once at creation time.
 *  - Every write is immediately flushed so data survives a crash.
 *  - Logging is automatically disabled when simulation mode is active.
 *  - The log file lives in the application working directory.
 */
class CsvLogger : public QObject
{
    Q_OBJECT

public:
    explicit CsvLogger(QObject *parent = nullptr);
    ~CsvLogger() override;

    bool isLogging() const { return m_file.isOpen() && m_enabled; }
    bool isEnabled() const { return m_enabled; }
    QString filePath() const { return m_file.fileName(); }
    int rowCount() const { return m_rowCount; }

public slots:
    /**
     * Log a single packet. Creates the file on first call if not already open.
     * No-op if logging is disabled or the packet is invalid.
     */
    void logPacket(const TelemetryPacket &packet);

    /**
     * Enable or disable logging.
     * Pass false during simulation playback; true for live serial mode.
     */
    void setEnabled(bool enabled);

    /** Close the current log file (e.g., on disconnect). */
    void closeFile();

signals:
    /** Emitted whenever the file path or row count changes. */
    void logStatusChanged(const QString &filePath, int rowCount);

    /** Emitted on file I/O errors. */
    void logError(const QString &message);

private:
    bool openFile(const QString &teamId);

    QFile        m_file;
    QTextStream  m_stream;
    int          m_rowCount{0};
    bool         m_enabled{true};
    bool         m_headerWritten{false};
};
