#include "core/Types.h"

namespace islamic {

QString prayerKeyId(PrayerKey key)
{
    switch (key) {
    case PrayerKey::Fajr: return QStringLiteral("fajr");
    case PrayerKey::Sunrise: return QStringLiteral("sunrise");
    case PrayerKey::Dhuhr: return QStringLiteral("dhuhr");
    case PrayerKey::Asr: return QStringLiteral("asr");
    case PrayerKey::Maghrib: return QStringLiteral("maghrib");
    case PrayerKey::Isha: return QStringLiteral("isha");
    }
    return QString();
}

PrayerKey prayerKeyFromId(const QString &id, bool *ok)
{
    const QString key = id.trimmed().toLower();
    if (key == QLatin1String("fajr")) { if (ok) *ok = true; return PrayerKey::Fajr; }
    if (key == QLatin1String("sunrise") || key == QLatin1String("shuruq")) { if (ok) *ok = true; return PrayerKey::Sunrise; }
    if (key == QLatin1String("dhuhr") || key == QLatin1String("zuhr")) { if (ok) *ok = true; return PrayerKey::Dhuhr; }
    if (key == QLatin1String("asr")) { if (ok) *ok = true; return PrayerKey::Asr; }
    if (key == QLatin1String("maghrib")) { if (ok) *ok = true; return PrayerKey::Maghrib; }
    if (key == QLatin1String("isha")) { if (ok) *ok = true; return PrayerKey::Isha; }
    if (ok) *ok = false;
    return PrayerKey::Fajr;
}

QVector<PrayerKey> allPrayerKeys()
{
    return { PrayerKey::Fajr, PrayerKey::Sunrise, PrayerKey::Dhuhr,
             PrayerKey::Asr, PrayerKey::Maghrib, PrayerKey::Isha };
}

const Prayer *PrayerDay::prayer(PrayerKey key) const
{
    for (const Prayer &p : prayers) {
        if (p.key == key)
            return &p;
    }
    return nullptr;
}

Prayer *PrayerDay::prayer(PrayerKey key)
{
    for (Prayer &p : prayers) {
        if (p.key == key)
            return &p;
    }
    return nullptr;
}

} // namespace islamic
