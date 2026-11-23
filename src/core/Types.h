#pragma once

#include <QDate>
#include <QDateTime>
#include <QMap>
#include <QString>
#include <QTime>
#include <QVector>

// ---------------------------------------------------------------------------
// The value types the whole library is built on. No formatting, no QML, no
// network: a PrayerDay is the same object whether it came from the provider,
// from the cache, or from a test fixture.
// ---------------------------------------------------------------------------

namespace islamic {

struct HijriDate {
    int year = 0;
    int month = 0;
    int day = 0;

    bool isValid() const
    {
        return year > 0 && month >= 1 && month <= 12 && day >= 1 && day <= 30;
    }
    bool operator==(const HijriDate &o) const
    {
        return year == o.year && month == o.month && day == o.day;
    }
    bool operator!=(const HijriDate &o) const { return !(*this == o); }
};

inline bool hijriLess(const HijriDate &a, const HijriDate &b)
{
    if (a.year != b.year)
        return a.year < b.year;
    if (a.month != b.month)
        return a.month < b.month;
    return a.day < b.day;
}

enum class PrayerKey { Fajr, Sunrise, Dhuhr, Asr, Maghrib, Isha };

QString prayerKeyId(PrayerKey key);
PrayerKey prayerKeyFromId(const QString &id, bool *ok = nullptr);
QVector<PrayerKey> allPrayerKeys();

struct Prayer {
    PrayerKey key = PrayerKey::Fajr;

    // The provider sends two adhan times and they mean different things: what
    // the mosque calls, and what the calculation says. We keep both untouched
    // and never substitute one for the other behind the caller's back — the
    // effective `adhan` is simply the mosque's when it exists.
    QTime mosqueAdhan;   // the mosque's own call; invalid when it publishes none
    QTime methodAdhan;   // the provider's calculated time
    QTime adhan;         // effective: mosqueAdhan if valid, else methodAdhan
    QTime iqama;         // invalid = the mosque publishes none

    bool hasAdhan() const { return adhan.isValid(); }
    bool hasIqama() const { return iqama.isValid(); }
    bool hasMosqueAdhan() const { return mosqueAdhan.isValid(); }
};

// The mosque's Friday service. `khutbah` can be empty even when the service
// exists, which is why it is not merged into Prayer.
struct Jumuah {
    QString location;
    QTime khutbah;
    QTime iqama;
};

enum class TimesSource { None, IqamahMosque, IqamahCalculated };

// The provider's own word about the day it just sent. "None" is a real answer:
// it means this mosque has no published timetable, so every iqama is null and
// the times are the country calculation. It is not an error and must not be
// dressed up as one.
enum class DayStatus { Unknown, Fresh, Stale, None };

// Where a value came from, and how fresh it is. Every day carries one, so the
// UI can be honest ("calculated", "as of 17 Nov") instead of silently stale.
struct Provenance {
    TimesSource source = TimesSource::None;
    QDate publishedFor;         // the day the source published these times for
    QDateTime asOf;             // when the source says it was last updated
    DayStatus status = DayStatus::Unknown;   // fresh | stale | none | unknown
    bool stale = false;                       // status == Stale, kept for convenience
    bool predicted = false;     // we extrapolated it; never true for provider data
};

struct PrayerDay {
    QDate date;
    HijriDate hijri;
    QVector<Prayer> prayers;    // six entries, in PrayerKey order
    QVector<Jumuah> jumuah;
    Provenance provenance;

    const Prayer *prayer(PrayerKey key) const;
    Prayer *prayer(PrayerKey key);
    bool isValid() const { return date.isValid(); }
};

// Everything the provider says about *how* it computed the times. Nothing here is
// interpreted: the library stores it so a future settings popup can show and
// override it, and so a payload remains fully explainable.
struct MethodPreset {
    QString id;
    QString name;
    double fajrAngle = -1;      // degrees; -1 when the provider gives an interval
    double ishaAngle = -1;
    int fajrMinutes = -1;       // used instead of the angle when present
    int ishaMinutes = -1;
    QString asr;                // "standard" | "hanafi"
    QString highLat;            // e.g. "middle_of_the_night"
    QMap<QString, int> adjustments;   // per-prayer minutes, e.g. dhuhr +7
    QString defaultsVersion;    // the provider's own version stamp

    bool hasTimes() const { return !id.isEmpty() || fajrAngle >= 0 || ishaAngle >= 0; }
};

} // namespace islamic
