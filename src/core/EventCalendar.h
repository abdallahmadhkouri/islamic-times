#pragma once

#include "core/HijriYear.h"
#include "core/Types.h"

#include <QDate>
#include <QVector>

#include <optional>

namespace islamic {

struct IslamicEvent {
    QString id;      // "ramadan", "qadr", "fitr", ...
    int month = 0;   // Hijri month
    int day = 0;     // Hijri day
    bool minor = false;
};

struct EventOccurrence {
    IslamicEvent event;
    HijriDate hijri;
    QDate date;
    int daysAway = 0;   // negative for a past occurrence
};

struct CalendarSnapshot {
    std::optional<EventOccurrence> current;   // the most recent event that has started
    std::optional<EventOccurrence> next;      // the first event after today
};

// One rule produces the whole sequence: an event is "current" from its start
// until the next event starts, and "next" is simply the next start. That is why
// there is no per-event state machine and no month-length arithmetic here.
class EventCalendar
{
public:
    static QVector<IslamicEvent> events(bool includeMinor = false);

    static CalendarSnapshot around(const HijriDate &todayHijri,
                                   const QDate &today,
                                   const HijriYear &year,
                                   bool includeMinor = false);
};

} // namespace islamic
