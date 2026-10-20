#include "TelemetryParser.h"

#include <QStringList>
#include <QDateTime>

// ── Public ────────────────────────────────────────────────────────────────────

bool TelemetryParser::parse(const QString &raw,
                             TelemetryPacket &out,
                             QString &error)
{
    // Strip any trailing CR / LF that may arrive from the serial port
    QString line = raw.trimmed();

    if (line.isEmpty()) {
        error = "Empty line";
        return false;
    }

    QStringList fields = line.split(',');

    // ── Handle firmware time field with comma separator ───────────────────────
    // The firmware sends: "DD/MM/YYYY,HH:MM:SS" which splits into 2 fields.
    // Normal packet has 15 fields, but with comma-in-time it becomes 16.
    // We detect this and merge fields[2]+","+fields[3] back into one.
    if (fields.size() == TelemetryPacket::FIELD_COUNT + 1) {
        // 16 fields → time field contained a comma, merge it back
        // fields[2] = "DD/MM/YYYY", fields[3] = "HH:MM:SS"
        fields[2] = fields[2] + "," + fields[3];
        fields.removeAt(3);  // remove the split time part, now fields has 15
    } else if (fields.size() != TelemetryPacket::FIELD_COUNT) {
        error = QString("Expected %1 fields, got %2 in: \"%3\"")
                    .arg(TelemetryPacket::FIELD_COUNT)
                    .arg(fields.size())
                    .arg(line.left(80));
        return false;
    }

    // Work into a local packet so we don't partially overwrite `out` on error
    TelemetryPacket p;
    bool ok = false;

    // Field 0 — TEAM_ID
    p.teamId = fields[0].trimmed();
    if (p.teamId.isEmpty()) {
        error = "TEAM_ID is empty";
        return false;
    }

    // Field 1 — PACKET_NO
    p.packetNo = fields[1].trimmed().toInt(&ok);
    if (!ok) {
        error = "PACKET_NO is not a valid integer: " + fields[1].trimmed();
        return false;
    }

    // Field 2 — TIME (may be "DD/MM/YYYY HH:MM:SS" or "DD/MM/YYYY,HH:MM:SS" or "NO_TIME")
    p.timeRaw = fields[2].trimmed();
    p.timestamp = parseTimestamp(p.timeRaw, ok);
    if (!ok) {
        // Non-fatal: keep the raw string, leave timestamp invalid
        // (some CubeSat implementations send "NO_TIME" before GPS lock)
        p.timestamp = QDateTime();
    }

    // Field 3 — PRESSURE
    p.pressure = fields[3].trimmed().toFloat(&ok);
    if (!ok) {
        error = "PRESSURE is not a valid float: " + fields[3].trimmed();
        return false;
    }

    // Field 4 — REL_ALTITUDE
    p.relAltitude = fields[4].trimmed().toFloat(&ok);
    if (!ok) {
        error = "REL_ALTITUDE is not a valid float: " + fields[4].trimmed();
        return false;
    }

    // Field 5 — DESCENT_SPEED
    p.descentSpeed = fields[5].trimmed().toFloat(&ok);
    if (!ok) {
        error = "DESCENT_SPEED is not a valid float: " + fields[5].trimmed();
        return false;
    }

    // Field 6 — VOLTAGE
    p.voltage = fields[6].trimmed().toFloat(&ok);
    if (!ok) {
        error = "VOLTAGE is not a valid float: " + fields[6].trimmed();
        return false;
    }

    // Field 7 — GPS_LAT (may be "NO_GPS" if no fix)
    {
        const QString s = fields[7].trimmed();
        if (s.compare("NO_GPS", Qt::CaseInsensitive) == 0) {
            p.gpsLat = 0.0;
        } else {
            p.gpsLat = s.toDouble(&ok);
            if (!ok) {
                error = "GPS_LAT is not a valid double: " + s;
                return false;
            }
        }
    }

    // Field 8 — GPS_LNG (may be "NO_GPS")
    {
        const QString s = fields[8].trimmed();
        if (s.compare("NO_GPS", Qt::CaseInsensitive) == 0) {
            p.gpsLng = 0.0;
        } else {
            p.gpsLng = s.toDouble(&ok);
            if (!ok) {
                error = "GPS_LNG is not a valid double: " + s;
                return false;
            }
        }
    }

    // Field 9 — GPS_ALT (may be "NO_GPS")
    {
        const QString s = fields[9].trimmed();
        if (s.compare("NO_GPS", Qt::CaseInsensitive) == 0) {
            p.gpsAlt = 0.0;
        } else {
            p.gpsAlt = s.toDouble(&ok);
            if (!ok) {
                error = "GPS_ALT is not a valid double: " + s;
                return false;
            }
        }
    }

    // Field 10 — PITCH
    p.pitch = fields[10].trimmed().toFloat(&ok);
    if (!ok) {
        error = "PITCH is not a valid float: " + fields[10].trimmed();
        return false;
    }

    // Field 11 — ROLL
    p.roll = fields[11].trimmed().toFloat(&ok);
    if (!ok) {
        error = "ROLL is not a valid float: " + fields[11].trimmed();
        return false;
    }

    // Field 12 — YAW
    p.yaw = fields[12].trimmed().toFloat(&ok);
    if (!ok) {
        error = "YAW is not a valid float: " + fields[12].trimmed();
        return false;
    }

    // Field 13 — SEPARATION
    p.separation = fields[13].trimmed().toInt(&ok);
    if (!ok || (p.separation != 0 && p.separation != 1)) {
        error = "SEPARATION must be 0 or 1, got: " + fields[13].trimmed();
        return false;
    }

    // Field 14 — CRYPTO_MSG (may be empty string — that is valid)
    p.cryptoMsg = fields[14].trimmed();

    out = p;
    error.clear();
    return true;
}

// ── Private ───────────────────────────────────────────────────────────────────

QDateTime TelemetryParser::parseTimestamp(const QString &s, bool &ok)
{
    // Try multiple formats to be robust:
    // 1. "DD/MM/YYYY HH:MM:SS" — standard with space
    // 2. "DD/MM/YYYY,HH:MM:SS" — firmware format with comma
    // 3. "NO_TIME" — GPS not locked

    if (s.compare("NO_TIME", Qt::CaseInsensitive) == 0) {
        ok = false;
        return QDateTime();
    }

    QDateTime dt = QDateTime::fromString(s, "dd/MM/yyyy HH:mm:ss");
    if (dt.isValid()) {
        ok = true;
        return dt;
    }

    dt = QDateTime::fromString(s, "dd/MM/yyyy,HH:mm:ss");
    ok = dt.isValid();
    return dt;
}
