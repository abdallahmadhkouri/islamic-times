#pragma once

#include "core/Types.h"
#include "providers/IqamahClient.h"
#include "store/PayloadCache.h"

#include <QDate>
#include <QString>

namespace islamic {

struct ChainOptions {
    int radiusKm = 10;                 // a town, not a region: see DECISIONS 13
    int yearMaxAgeDays = 45;           // beyond this the year is refetched when possible
    int mosqueDayMaxAgeMinutes = 360;  // six hours: mosques change times
    int nearbyMaxAgeDays = 30;         // a mosque does not move
};

struct DayResult {
    PrayerDay day;
    MethodPreset method;
    MosqueInfo mosque;                 // isValid() == false when none was found
    bool yearFromCache = false;
    bool mosqueFromCache = false;
    QString notice;                    // plain-language note for the UI, may be empty

    bool ok() const { return day.isValid(); }
};

// Assembles one day out of the provider: calculated times for the year, the
// mosque's own times when one is found in town, and an honest note about what came
// from where. It owns policy — freshness, radius, fallback — and nothing else.
class IslamicTimes
{
public:
    IslamicTimes(IqamahClient *client, PayloadCache *cache, ChainOptions options = ChainOptions());

    // allowNetwork == false means "offline": only what is already cached is used.
    DayResult dayFor(double lat, double lon, const QDate &date, bool allowNetwork = true);

    QString notice() const { return m_notice; }

private:
    bool loadYear(double lat, double lon, int year, bool allowNetwork,
                  QVector<PrayerDay> *days, MethodPreset *method, bool *fromCache, bool *ok);
    MosqueInfo findMosque(double lat, double lon, bool allowNetwork, bool *fromCache);

    IqamahClient *m_client = nullptr;
    PayloadCache *m_cache = nullptr;
    ChainOptions m_options;
    QString m_notice;
};

} // namespace islamic
