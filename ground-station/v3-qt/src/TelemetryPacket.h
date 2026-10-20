#pragma once

#include <QString>
#include <QDateTime>

/**
 * TelemetryPacket
 * ---------------
 * Plain data struct representing one parsed telemetry frame received from
 * the CubeSat over XBee serial link (or replayed from a CSV file).
 *
 * Wire format (1 Hz, comma-separated):
 *   TEAM_ID, PACKET_NO, TIME, PRESSURE, REL_ALTITUDE, DESCENT_SPEED,
 *   VOLTAGE, GPS_LAT, GPS_LNG, GPS_ALT, PITCH, ROLL, YAW,
 *   SEPARATION, CRYPTO_MSG
 *
 * All fields are stored in their native C++ type after parsing.
 * The struct is a value type — cheap to copy across signal/slot boundaries.
 */
struct TelemetryPacket
{
    // ── Identity & sequencing ─────────────────────────────────────────────
    QString  teamId;        ///< 5-digit team identifier (string)
    int      packetNo{0};   ///< Sequential packet counter (survives reboots)

    // ── Timestamp ─────────────────────────────────────────────────────────
    QString  timeRaw;       ///< Raw string from wire: "DD/MM/YYYY HH:MM:SS"
    QDateTime timestamp;    ///< Parsed QDateTime (local, no timezone assumed)

    // ── Atmospheric / barometric ──────────────────────────────────────────
    float    pressure{0.0f};       ///< Atmospheric pressure [Pa]
    float    relAltitude{0.0f};    ///< Altitude above launch point [m]
    float    descentSpeed{0.0f};   ///< Descent speed [m/s]

    // ── Electrical ────────────────────────────────────────────────────────
    float    voltage{0.0f};        ///< Battery voltage [V]

    // ── GPS ───────────────────────────────────────────────────────────────
    double   gpsLat{0.0};          ///< Latitude  [decimal degrees]
    double   gpsLng{0.0};          ///< Longitude [decimal degrees]
    double   gpsAlt{0.0};          ///< GPS altitude [m MSL]

    // ── Orientation ───────────────────────────────────────────────────────
    float    pitch{0.0f};   ///< Pitch angle [degrees]
    float    roll{0.0f};    ///< Roll angle  [degrees]
    float    yaw{0.0f};     ///< Yaw angle   [degrees]

    // ── Mission state ─────────────────────────────────────────────────────
    int      separation{0};      ///< 0 = not separated, 1 = separated
    QString  cryptoMsg;          ///< Encrypted message received from ground

    // ── Helpers ───────────────────────────────────────────────────────────

    /** Returns true if the packet appears to have valid identity fields. */
    bool isValid() const
    {
        return !teamId.isEmpty() && packetNo >= 0;
    }

    /**
     * Returns a CSV row string matching the telemetry wire format order.
     * Used by CsvLogger to write one row per packet.
     */
    QString toCsvRow() const
    {
        return QString("%1,%2,%3,%4,%5,%6,%7,%8,%9,%10,%11,%12,%13,%14,%15")
            .arg(teamId)
            .arg(packetNo)
            .arg(timeRaw)
            .arg(pressure,        0, 'f', 4)
            .arg(relAltitude,     0, 'f', 4)
            .arg(descentSpeed,    0, 'f', 4)
            .arg(voltage,         0, 'f', 4)
            .arg(gpsLat,          0, 'f', 8)
            .arg(gpsLng,          0, 'f', 8)
            .arg(gpsAlt,          0, 'f', 4)
            .arg(pitch,           0, 'f', 4)
            .arg(roll,            0, 'f', 4)
            .arg(yaw,             0, 'f', 4)
            .arg(separation)
            .arg(cryptoMsg);
    }

    /** CSV header row — must match toCsvRow() column order exactly. */
    static QString csvHeader()
    {
        return QStringLiteral(
            "TEAM_ID,PACKET_NO,TIME,PRESSURE,REL_ALTITUDE,"
            "DESCENT_SPEED,VOLTAGE,GPS_LAT,GPS_LNG,GPS_ALT,"
            "PITCH,ROLL,YAW,SEPARATION,CRYPTO_MSG"
        );
    }

    /** Expected number of comma-separated fields in one packet. */
    static constexpr int FIELD_COUNT = 15;
};
