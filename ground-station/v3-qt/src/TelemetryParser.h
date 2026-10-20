#pragma once

#include "TelemetryPacket.h"
#include <QString>

/**
 * TelemetryParser
 * ---------------
 * Stateless utility class. Parses a single raw comma-separated telemetry
 * string into a TelemetryPacket. Returns false (with a human-readable
 * error string) for any malformed input — never throws or crashes.
 */
class TelemetryParser
{
public:
    TelemetryParser() = default;

    /**
     * Parse one line of telemetry data.
     *
     * @param raw      The raw comma-separated string (trailing \r\n stripped).
     * @param out      Output packet (populated on success, unchanged on failure).
     * @param error    Human-readable error description (set on failure).
     * @return         true if parsing fully succeeded, false otherwise.
     */
    static bool parse(const QString &raw,
                      TelemetryPacket &out,
                      QString &error);

private:
    /** Parse "DD/MM/YYYY HH:MM:SS" into a QDateTime. */
    static QDateTime parseTimestamp(const QString &s, bool &ok);
};
