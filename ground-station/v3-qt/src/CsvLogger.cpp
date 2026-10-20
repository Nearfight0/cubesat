#include "CsvLogger.h"

#include <QDir>
#include <QDebug>

CsvLogger::CsvLogger(QObject *parent)
    : QObject(parent)
{}

CsvLogger::~CsvLogger()
{
    closeFile();
}

// ── Public slots ──────────────────────────────────────────────────────────────

void CsvLogger::logPacket(const TelemetryPacket &packet)
{
    if (!m_enabled) return;
    if (!packet.isValid()) return;

    // Open file on first valid packet
    if (!m_file.isOpen()) {
        if (!openFile(packet.teamId))
            return;
    }

    m_stream << packet.toCsvRow() << '\n';

    // Flush every write so data survives a crash
    m_stream.flush();

    ++m_rowCount;
    emit logStatusChanged(m_file.fileName(), m_rowCount);
}

void CsvLogger::setEnabled(bool enabled)
{
    m_enabled = enabled;

    if (!enabled && m_file.isOpen()) {
        m_stream.flush();
        // Keep file open — just stop writing. Caller may re-enable later.
    }

    if (!enabled)
        emit logStatusChanged(QString(), m_rowCount);
    else if (m_file.isOpen())
        emit logStatusChanged(m_file.fileName(), m_rowCount);
}

void CsvLogger::closeFile()
{
    if (m_file.isOpen()) {
        m_stream.flush();
        m_file.close();
        m_headerWritten = false;
        m_rowCount = 0;
        emit logStatusChanged(QString(), 0);
    }
}

// ── Private ───────────────────────────────────────────────────────────────────

bool CsvLogger::openFile(const QString &teamId)
{
    // File goes in the current working directory as <TEAM_ID>.csv
    const QString fileName = teamId + ".csv";
    m_file.setFileName(fileName);

    // Append if file already exists (i.e., continuing a session after reboot)
    if (!m_file.open(QIODevice::Append | QIODevice::Text)) {
        const QString msg = QString("Cannot open log file %1: %2")
                                .arg(fileName)
                                .arg(m_file.errorString());
        qWarning() << "[CsvLogger]" << msg;
        emit logError(msg);
        return false;
    }

    m_stream.setDevice(&m_file);
    m_stream.setEncoding(QStringConverter::Utf8);

    // Write header only if the file is new (size == 0)
    if (m_file.size() == 0 || !m_headerWritten) {
        m_stream << TelemetryPacket::csvHeader() << '\n';
        m_stream.flush();
        m_headerWritten = true;
    }

    qDebug() << "[CsvLogger] Logging to" << QDir::current().absoluteFilePath(fileName);
    emit logStatusChanged(m_file.fileName(), m_rowCount);
    return true;
}
