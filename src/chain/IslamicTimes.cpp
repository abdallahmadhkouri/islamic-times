#include "chain/IslamicTimes.h"

#include "providers/IqamahParser.h"

namespace islamic {

IslamicTimes::IslamicTimes(IqamahClient *client, PayloadCache *cache, ChainOptions options)
    : m_client(client), m_cache(cache), m_options(options)
{
}

bool IslamicTimes::loadYear(double lat, double lon, int year, bool allowNetwork,
                            QVector<PrayerDay> *days, MethodPreset *method,
                            bool *fromCache, bool *ok)
{
    *ok = false;
    *fromCache = false;
    if (!days || !method)
        return false;

    const QString key = PayloadCache::keyForYear(lat, lon, year);
    CacheEntry entry;

    if (m_cache && m_cache->read(key, &entry)) {
        const int ageDays = entry.ageSeconds() / 86400;
        const bool fresh = entry.ageSeconds() >= 0 && ageDays <= m_options.yearMaxAgeDays;
        if (fresh || !allowNetwork) {
            if (IqamahParser::parseYearTimes(entry.body, days, method, nullptr) && !days->isEmpty()) {
                *fromCache = true;
                *ok = true;
                if (!fresh)
                    m_notice = QStringLiteral("offline: times from a cached year, %1 days old").arg(ageDays);
                return true;
            }
        }
    }

    if (!allowNetwork || !m_client)
        return false;

    QByteArray body;
    QString error;
    if (m_client->fetchYear(lat, lon, year, &body, &error) != ClientError::None) {
        // The network said no; an older cache is still better than nothing.
        if (m_cache && m_cache->read(key, &entry)
            && IqamahParser::parseYearTimes(entry.body, days, method, nullptr)) {
            *fromCache = true;
            *ok = true;
            m_notice = QStringLiteral("offline: %1").arg(error);
            return true;
        }
        m_notice = error;
        return false;
    }

    if (m_cache)
        m_cache->write(key, body);
    *ok = IqamahParser::parseYearTimes(body, days, method, nullptr);
    return *ok;
}

MosqueInfo IslamicTimes::findMosque(double lat, double lon, bool allowNetwork, bool *fromCache)
{
    *fromCache = false;
    if (!m_cache)
        return {};
    if (!IqamahClient::isValidRadius(m_options.radiusKm))
        return {};

    const QString key = PayloadCache::keyForNearby(lat, lon, m_options.radiusKm);
    CacheEntry entry;
    QVector<MosqueInfo> mosques;

    // An empty answer is an answer: "no mosque in this town" is cacheable, and
    // refetching it on every launch would hammer the provider for nothing.
    bool haveUsableCache = false;
    if (m_cache->read(key, &entry)) {
        const bool fresh = entry.ageSeconds() >= 0
                           && entry.ageSeconds() / 86400 <= m_options.nearbyMaxAgeDays;
        if (fresh || !allowNetwork) {
            mosques = IqamahClient::parseNearby(entry.body);
            haveUsableCache = fresh;
            *fromCache = true;
        }
    }

    if (!haveUsableCache && allowNetwork && m_client) {
        QString error;
        QByteArray body;
        if (m_client->fetchNearby(lat, lon, m_options.radiusKm, &mosques, &body, &error) == ClientError::None) {
            m_cache->write(key, body);
            *fromCache = false;
        }
    }

    // Only a mosque that publishes a timetable is worth adopting: anything else
    // would show a name next to times it never announced.
    for (const MosqueInfo &mosque : mosques) {
        if (mosque.hasSource)
            return mosque;
    }
    return {};
}

DayResult IslamicTimes::dayFor(double lat, double lon, const QDate &date, bool allowNetwork)
{
    m_notice.clear();
    DayResult result;
    if (!date.isValid())
        return result;

    QVector<PrayerDay> days;
    MethodPreset method;
    bool yearCached = false;
    bool ok = false;
    loadYear(lat, lon, date.year(), allowNetwork, &days, &method, &yearCached, &ok);
    if (!ok) {
        if (result.notice.isEmpty())
            result.notice = m_notice.isEmpty() ? QStringLiteral("no times available") : m_notice;
        return result;
    }

    const PrayerDay *picked = nullptr;
    for (const PrayerDay &day : days) {
        if (day.date == date) {
            picked = &day;
            break;
        }
    }
    if (!picked) {
        result.notice = QStringLiteral("the year payload does not cover this day");
        return result;
    }

    result.day = *picked;
    result.method = method;
    result.yearFromCache = yearCached;
    if (!m_notice.isEmpty())
        result.notice = m_notice;

    bool nearbyCached = false;
    result.mosque = findMosque(lat, lon, allowNetwork, &nearbyCached);
    if (!result.mosque.isValid())
        return result;

    const QString key = PayloadCache::keyForMosqueDay(result.mosque.id, date);
    CacheEntry entry;
    QByteArray body;
    bool haveBody = false;

    if (m_cache && m_cache->read(key, &entry)) {
        const int ageMinutes = entry.ageSeconds() / 60;
        const bool fresh = entry.ageSeconds() >= 0 && ageMinutes <= m_options.mosqueDayMaxAgeMinutes;
        if (fresh || !allowNetwork) {
            body = entry.body;
            haveBody = true;
            result.mosqueFromCache = true;
            if (!fresh)
                result.notice = QStringLiteral("mosque times are %1 hours old").arg(ageMinutes / 60);
        }
    }

    if (!haveBody && allowNetwork && m_client) {
        QString error;
        if (m_client->fetchMosqueDay(result.mosque.id, &body, &error) != ClientError::None) {
            result.notice = error;
            return result;
        }
        if (m_cache)
            m_cache->write(key, body);
        haveBody = true;
    }

    if (!haveBody)
        return result;

    PrayerDay mosqueDay;
    if (!IqamahParser::parseMosqueDay(body, &mosqueDay, nullptr)) {
        result.notice = QStringLiteral("the mosque answer could not be read");
        return result;
    }
    if (mosqueDay.date != date) {
        result.notice = QStringLiteral("the mosque published a different day");
        return result;
    }

    if (mosqueDay.provenance.status == DayStatus::None) {
        result.notice = QStringLiteral("%1 publishes no timetable — calculated times").arg(result.mosque.name);
        return result;
    }

    // Overlay only what the mosque actually publishes; everything else stays the
    // calculated time, and the provenance becomes the mosque's.
    for (const Prayer &from : mosqueDay.prayers) {
        Prayer *to = result.day.prayer(from.key);
        if (!to)
            continue;
        if (from.hasMosqueAdhan())
            to->mosqueAdhan = from.mosqueAdhan;
        if (from.adhan.isValid())
            to->adhan = from.adhan;
        if (from.hasIqama())
            to->iqama = from.iqama;
    }
    result.day.jumuah = mosqueDay.jumuah;
    result.day.provenance = mosqueDay.provenance;
    return result;
}

} // namespace islamic
