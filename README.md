# islamic-times

A C++/Qt wrapper around the [iqamah](https://iqamah.org) API: prayer times, the local
mosque's jamaa times, Hijri dates, Islamic events, and a countdown state machine —
with a cache that survives a network outage and a model that survives the poles.

It is a library. It has no UI, no Arabic strings, no fonts and no opinion about what
you draw with it. Any Qt project — a launcher, a kiosk, a desktop app, a service, a
car HMI — links it and asks for a day.

**Status:** complete and tested (ten suites, 364 assertions). See `ARCHITECTURE.md`,
`FEATURES.md`, `DECISIONS.md` and `PLAN.md`.

## What it answers

```cpp
PayloadCache cache;                 // app-private data dir, /iqamah
QNetworkTransport transport;        // https only, manual redirects
IqamahClient client(&transport);    // keyless, one retry
IslamicTimes times(&client, &cache);

const DayResult day = times.dayFor(36.80, 10.18, QDate(2027, 2, 8));

day.day.hijri                                  // 1448-09-01
day.day.prayer(PrayerKey::Fajr)->adhan         // 05:48  effective
day.day.prayer(PrayerKey::Fajr)->mosqueAdhan   // invalid: the mosque publishes none
day.method.id                                  // "tn_ministry"
day.mosque.isValid()                           // false: no mosque within 10 km
day.notice                                     // why, in one sentence
```

and for the countdown:

```cpp
const PrayerSnapshot now = evaluate(day.day, &tomorrow,
                                   QDateTime(QDate(2027, 2, 8), QTime(5, 20)));
now.state      // WaitingAdhan -> WaitingIqama -> InPrayer -> next
now.key        // Fajr
now.seconds    // 1680
```

## Three rules the library keeps

1. **The provider owns its data.** Nothing is repaired, filled in or corrected. A null
   stays null; a mosque without a timetable says so; the mosque's own adhan and the
   calculated one are kept apart so a consumer can tell the difference.
2. **No local calculation.** If there is no payload and no cache, the answer is "no
   times", not a number this library invented.
3. **The cache stores bytes.** What is on disk is what iqamah sent, so the model can
   change without a migration and a stale payload can be audited.

## Build

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.12.0/gcc_64
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Qt 6.5 or newer. `islamic-times` links Qt Core only; `islamic-times-net` adds Qt
Network for the real transport, so a consumer that fetches nothing pays nothing.

## Licence and data

MIT. Prayer data from Iqamah — free for personal and commercial use, attribution
required: **Prayer times: Iqamah (iqamah.org)**. Official timetables carry their
authority's attribution in `source.attribution`, which the parser preserves.
