#pragma once

#include "core/Types.h"

namespace islamic {

namespace DayRules {

bool isFriday(const QDate &date);

// On a Friday the mosque's khutbah replaces the Dhuhr adhan, and the jumuah
// iqama replaces the Dhuhr iqama. Applied to a copy of the day by the provider
// layer, so every consumer sees one consistent "Dhuhr" entry and no caller has
// to know about Fridays.
void applyJumuah(PrayerDay &day);

} // namespace DayRules

} // namespace islamic
