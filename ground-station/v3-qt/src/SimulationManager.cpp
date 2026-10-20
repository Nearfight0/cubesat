#include "SimulationManager.h"
#include "TelemetryParser.h"

#include <QFile>
#include <QTextStream>
#include <QDebug>

SimulationManager::SimulationManager(QObject *parent)
    : QObject(parent)
{
    m_timer = new QTimer(this);
    m_timer->setSingleShot(false);
    connect(m_timer, &QTimer::timeout, this, &SimulationManager::emitNextPacket);
}

SimulationManager::~SimulationManager() = default;

// ── Queries ───────────────────────────────────────────────────────────────────

int SimulationManager::flightDurationSeconds() const
{
    if (m_packets.size() < 2) return 0;

    const QDateTime &t0 = m_packets.first().timestamp;
    const QDateTime &t1 = m_packets.last().timestamp;

    if (!t0.isValid() || !t1.isValid()) {
        // Fall back to packet count at 1 Hz
        return m_packets.size();
    }
    return static_cast<int>(t0.secsTo(t1));
}

// ── Public slots ──────────────────────────────────────────────────────────────

void SimulationManager::loadFile(const QString &filePath)
{
    stop();
    m_packets.clear();
    m_currentIndex = 0;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit fileLoadError(
            QString("Cannot open file: %1").arg(file.errorString())
        );
        return;
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);

    int lineNumber = 0;
    int skipped = 0;
    bool headerSkipped = false;

    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        ++lineNumber;

        if (line.isEmpty()) continue;

        // Skip the header row (first non-empty line if it starts with TEAM_ID)
        if (!headerSkipped && line.startsWith("TEAM_ID", Qt::CaseInsensitive)) {
            headerSkipped = true;
            continue;
        }

        TelemetryPacket packet;
        QString error;

        if (TelemetryParser::parse(line, packet, error)) {
            m_packets.append(packet);
        } else {
            ++skipped;
            qDebug() << "[SimulationManager] Skipped line" << lineNumber
                     << ":" << error;
        }
    }

    if (m_packets.isEmpty()) {
        emit fileLoadError(
            "No valid telemetry packets found in file.\n"
            "Expected 15 comma-separated columns per row."
        );
        return;
    }

    setState(State::Loaded);
    emit fileLoaded(m_packets.size(), skipped, flightDurationSeconds());
    emit progressChanged(0, m_packets.size());
}

void SimulationManager::play()
{
    if (m_state == State::Idle) return;
    if (m_state == State::Playing) return;

    if (m_currentIndex >= m_packets.size()) {
        // Already at end — reset first
        reset();
        if (m_packets.isEmpty()) return;
    }

    setState(State::Playing);
    m_timer->start(speedToInterval(m_speed));
}

void SimulationManager::pause()
{
    if (m_state != State::Playing) return;
    m_timer->stop();
    setState(State::Paused);
}

void SimulationManager::stop()
{
    m_timer->stop();
    m_currentIndex = 0;

    if (m_packets.isEmpty())
        setState(State::Idle);
    else
        setState(State::Loaded);

    emit progressChanged(0, m_packets.size());
}

void SimulationManager::reset()
{
    stop();
}

void SimulationManager::setSpeed(double multiplier)
{
    m_speed = multiplier;
    if (m_timer->isActive())
        m_timer->setInterval(speedToInterval(multiplier));
}

// ── Private slots ─────────────────────────────────────────────────────────────

void SimulationManager::emitNextPacket()
{
    if (m_currentIndex >= m_packets.size()) {
        m_timer->stop();
        setState(State::Loaded);
        emit progressChanged(m_packets.size(), m_packets.size());
        emit simulationComplete();
        return;
    }

    emit packetReceived(m_packets[m_currentIndex]);
    emit progressChanged(m_currentIndex + 1, m_packets.size());

    ++m_currentIndex;
}

// ── Private helpers ───────────────────────────────────────────────────────────

void SimulationManager::setState(State s)
{
    if (m_state == s) return;
    m_state = s;
    emit stateChanged(s);
}

int SimulationManager::speedToInterval(double multiplier) const
{
    // 1x = 1000 ms (matches 1 Hz telemetry)
    if (multiplier <= 0.5)  return 2000;
    if (multiplier <= 1.0)  return 1000;
    if (multiplier <= 2.0)  return  500;
    if (multiplier <= 5.0)  return  200;
    return 100; // 10x
}
