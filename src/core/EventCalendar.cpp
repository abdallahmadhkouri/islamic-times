#include "core/EventCalendar.h"

#include <algorithm>

namespace islamic {

QVector<IslamicEvent> EventCalendar::events(bool includeMinor)
{
    QVector<IslamicEvent> list = {
        { QStringLiteral("newyear"), 1, 1, false },
        { QStringLiteral("ashura"), 1, 10, false },
        { QStringLiteral("mawlid"), 3, 12, false },
        { QStringLiteral("ramadan"), 9, 1, false },
        { QStringLiteral("lastten"), 9, 21, false },
        { QStringLiteral("qadr"), 9, 27, false },
        { QStringLiteral("fitr"), 10, 1, false },
        { QStringLiteral("arafah"), 12, 9, false },
        { QStringLiteral("adha"), 12, 10, false },
    };
    if (includeMinor) {
        list.append({ QStringLiteral("isra"), 7, 27, true });
        list.append({ QStringLiteral("midshaban"), 8, 15, true });
    }
    return list;
}

CalendarSnapshot EventCalendar::around(const HijriDate &todayHijri,
                                       const QDate &today,
                                       const HijriYear &year,
                                       bool includeMinor)
{
    CalendarSnapshot snapshot;
    if (!todayHijri.isValid() || !today.isValid())
        return snapshot;

    QVector<EventOccurrence> occurrences;
    const QVector<IslamicEvent> table = events(includeMinor);
    for (int hy = todayHijri.year - 1; hy <= todayHijri.year + 1; ++hy) {
        for (const IslamicEvent &event : table) {
            const HijriDate hijri{ hy, event.month, event.day };
            const QDate date = year.dateOf(hijri);
            if (!date.isValid())
                continue;   // outside the cached year — the caller decides what to do
            occurrences.append({ event, hijri, date, static_cast<int>(today.daysTo(date)) });
        }
    }
    std::sort(occurrences.begin(), occurrences.end(),
              [](const EventOccurrence &a, const EventOccurrence &b) {
                  if (a.date != b.date)
                      return a.date < b.date;
                  return a.event.month < b.event.month;
              });

    for (const EventOccurrence &occurrence : occurrences) {
        if (occurrence.date <= today)
            snapshot.current = occurrence;
    }
    for (const EventOccurrence &occurrence : occurrences) {
        if (occurrence.date > today) {
            snapshot.next = occurrence;
            break;
        }
    }
    return snapshot;
}

} // namespace islamic
