#pragma once

#include "core/Types.h"

#include <QDateTime>

namespace islamic {

enum class PrayerState {
    Idle,           // nothing to count to (no data)
    WaitingAdhan,   // counting to the next prayer's adhan
    WaitingIqama,   // adhan has been called, counting to the iqama
    InPrayer        // iqama has been called, within the in-prayer window
};

struct PrayerSnapshot {
    PrayerState state = PrayerState::Idle;
    PrayerKey key = PrayerKey::Fajr;
    QDateTime target;      // the instant being counted to (invalid for InPrayer)
    int seconds = 0;       // to target, or since the iqama for InPrayer
};

// The whole "Mawaqit" sequence, decided once, testable without a UI:
//   WaitingAdhan → WaitingIqama → InPrayer → WaitingAdhan (next prayer)
// `tomorrow` is optional so the caller can keep the countdown alive after Isha.
PrayerSnapshot evaluate(const PrayerDay &today,
                        const PrayerDay *tomorrow,
                        const QDateTime &now,
                        int inPrayerMinutes = 20);

} // namespace islamic
