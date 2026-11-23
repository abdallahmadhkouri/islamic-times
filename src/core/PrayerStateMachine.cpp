#include "core/PrayerStateMachine.h"

namespace islamic {

namespace {

QDateTime at(const QDate &date, const QTime &time)
{
    if (!date.isValid() || !time.isValid())
        return {};
    return QDateTime(date, time);
}

PrayerSnapshot waiting(PrayerState state, PrayerKey key, const QDateTime &target, const QDateTime &now)
{
    PrayerSnapshot snapshot;
    snapshot.state = state;
    snapshot.key = key;
    snapshot.target = target;
    snapshot.seconds = static_cast<int>(now.secsTo(target));
    return snapshot;
}

} // namespace

PrayerSnapshot evaluate(const PrayerDay &today,
                        const PrayerDay *tomorrow,
                        const QDateTime &now,
                        int inPrayerMinutes)
{
    PrayerSnapshot idle;

    if (today.isValid() && now.isValid()) {
        const int windowSeconds = inPrayerMinutes * 60;
        for (const Prayer &prayer : today.prayers) {
            // Sunrise is an astronomical event, not a prayer: nobody counts down
            // to it, and counting to it would hide the next real prayer for an hour.
            if (prayer.key == PrayerKey::Sunrise)
                continue;
            const QDateTime adhan = at(today.date.addDays(prayer.adhanDayOffset), prayer.adhan);
            if (!adhan.isValid())
                continue;

            if (now < adhan)
                return waiting(PrayerState::WaitingAdhan, prayer.key, adhan, now);

            // The adhan has been called. If the mosque publishes an iqama, it is
            // the next boundary; otherwise the prayer window starts now.
            if (prayer.hasIqama()) {
                const QDateTime iqama = at(today.date.addDays(prayer.iqamaDayOffset), prayer.iqama);
                if (iqama.isValid()) {
                    if (now < iqama)
                        return waiting(PrayerState::WaitingIqama, prayer.key, iqama, now);
                    if (now.secsTo(iqama) > -windowSeconds) {
                        PrayerSnapshot inPrayer;
                        inPrayer.state = PrayerState::InPrayer;
                        inPrayer.key = prayer.key;
                        inPrayer.target = iqama;
                        inPrayer.seconds = static_cast<int>(iqama.secsTo(now));
                        return inPrayer;
                    }
                }
            } else if (now.secsTo(adhan) > -windowSeconds) {
                PrayerSnapshot inPrayer;
                inPrayer.state = PrayerState::InPrayer;
                inPrayer.key = prayer.key;
                inPrayer.target = adhan;
                inPrayer.seconds = static_cast<int>(adhan.secsTo(now));
                return inPrayer;
            }
        }
    }

    // Nothing left today: count to tomorrow's first adhan if we have it.
    if (tomorrow && tomorrow->isValid()) {
        for (const Prayer &prayer : tomorrow->prayers) {
            if (prayer.key == PrayerKey::Sunrise)
                continue;
            const QDateTime adhan = at(tomorrow->date.addDays(prayer.adhanDayOffset), prayer.adhan);
            if (!adhan.isValid() || adhan <= now)
                continue;
            return waiting(PrayerState::WaitingAdhan, prayer.key, adhan, now);
        }
    }

    return idle;
}

} // namespace islamic
