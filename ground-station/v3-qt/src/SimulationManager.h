#pragma once

#include "TelemetryPacket.h"

#include <QObject>
#include <QTimer>
#include <QString>
#include <QVector>

/**
 * SimulationManager
 * -----------------
 * Loads a previously saved <TEAM_ID>.csv telemetry file and replays its
 * packets via a QTimer, emitting the same packetReceived() signal as
 * SerialManager so MainWindow needs no special-casing.
 *
 * State machine:
 *   Idle → Loaded → Playing ⇄ Paused → Idle (after Stop/Reset)
 */
class SimulationManager : public QObject
{
    Q_OBJECT

public:
    enum class State { Idle, Loaded, Playing, Paused };
    Q_ENUM(State)

    explicit SimulationManager(QObject *parent = nullptr);
    ~SimulationManager() override;

    State state() const { return m_state; }
    bool isPlaying() const { return m_state == State::Playing; }

    int totalPackets() const { return m_packets.size(); }
    int currentIndex() const { return m_currentIndex; }

    /** Duration of the loaded flight in seconds (last packet time – first). */
    int flightDurationSeconds() const;

public slots:
    /**
     * Load and validate a CSV file.
     * Emits fileLoaded() on success, fileLoadError() on failure.
     * Malformed rows are silently skipped; their count is included in the signal.
     */
    void loadFile(const QString &filePath);

    void play();
    void pause();
    void stop();
    void reset();

    /**
     * Set playback speed multiplier.
     * Accepted values: 0.5, 1.0, 2.0, 5.0, 10.0
     * Maps to timer intervals: 2000 / 1000 / 500 / 200 / 100 ms
     */
    void setSpeed(double multiplier);

signals:
    /** Same signal shape as SerialManager::packetReceived(). */
    void packetReceived(const TelemetryPacket &packet);

    /** Emitted after a successful file load. */
    void fileLoaded(int totalPackets, int skippedRows, int durationSeconds);

    /** Emitted when the file cannot be opened or has wrong format. */
    void fileLoadError(const QString &message);

    /** Progress: current index (0-based) and total. */
    void progressChanged(int current, int total);

    /** Emitted when all packets have been played back. */
    void simulationComplete();

    /** State has changed — update UI buttons accordingly. */
    void stateChanged(SimulationManager::State newState);

private slots:
    void emitNextPacket();

private:
    void setState(State s);
    int speedToInterval(double multiplier) const;

    QTimer              *m_timer{nullptr};
    QVector<TelemetryPacket> m_packets;
    int                  m_currentIndex{0};
    double               m_speed{1.0};
    State                m_state{State::Idle};
};
