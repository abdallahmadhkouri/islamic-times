#include "core/DayRules.h"

namespace islamic {
namespace DayRules {

bool isFriday(const QDate &date)
{
    return date.isValid() && date.dayOfWeek() == Qt::Friday;
}

void applyJumuah(PrayerDay &day)
{
    if (day.jumuah.isEmpty())
        return;

    // Prefer a service that publishes a khutbah time; fall back to the first
    // one that publishes anything at all.
    const Jumuah *chosen = nullptr;
    for (const Jumuah &service : day.jumuah) {
        if (service.khutbah.isValid()) {
            if (!chosen || service.khutbah < chosen->khutbah)
                chosen = &service;
        }
    }
    if (!chosen) {
        for (const Jumuah &service : day.jumuah) {
            if (service.iqama.isValid()) {
                chosen = &service;
                break;
            }
        }
    }
    if (!chosen)
        return;

    if (Prayer *dhuhr = day.prayer(PrayerKey::Dhuhr)) {
        if (chosen->khutbah.isValid())
            dhuhr->adhan = chosen->khutbah;
        if (chosen->iqama.isValid())
            dhuhr->iqama = chosen->iqama;
    }
}

} // namespace DayRules
} // namespace islamic
