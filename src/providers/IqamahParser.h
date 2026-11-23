#pragma once

#include "core/Types.h"

#include <QByteArray>
#include <QString>
#include <QVector>

namespace islamic {

// Parsing only: bytes in, value types out. No network, no cache, no QML. That
// is what lets the whole provider layer be tested against captured payloads.
class IqamahParser
{
public:
    // GET /v1/mosque/{id}.json — one day, with iqama and jumuah.
    static bool parseMosqueDay(const QByteArray &json, PrayerDay *day, QString *error = nullptr);

    // GET /v1/times/{lat},{lon}/{year}.json — a year of calculated times and
    // the Hijri day for each of them.
    static bool parseYearTimes(const QByteArray &json,
                               QVector<PrayerDay> *days,
                               MethodPreset *method = nullptr,
                               QString *error = nullptr);
};

} // namespace islamic
