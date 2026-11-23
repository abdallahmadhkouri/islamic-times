# islamic-times

A C++/Qt library for Islamic time facts: what time it is for prayer, which Islamic
day it is, and what comes next. It is deliberately not a widget — widgets are views
built on top of it.

**Status:** core, parsers and tests. The network client, the cache, the QML plugin and
the widgets follow.

## Why it exists

A launcher, a kiosk and a car HMI all need the same five facts — today's Hijri date,
the five prayer times, the mosque's iqama, jumua, and the next Islamic event — and
none of them should own a second copy of the logic. This library is that single copy.

Two rules shape everything here:

1. **No Hijri arithmetic.** The provider sends `(Gregorian day, Hijri day)` pairs for a
   whole year, so every Hijri answer is a lookup. A lookup cannot drift by a day; a
   computed calendar can, and Qt's built-in Islamic calendars are measured one day late
   on 13 of 24 month starts, including Eid al-Fitr 1448.
2. **No formatting.** The library returns numbers, dates, keys and enums. The Arabic
   lives in QML, where it belongs.

## Data source

Iqamah — <https://iqamah.org> — `api.iqamah.net`. Free for personal and commercial use,
no key, no sign-up; attribution required:

> Prayer times: Iqamah (iqamah.org)

Official timetables carry their authority's attribution in `source.attribution`; the
parser keeps the provenance so a UI can show it.

## Layout

| Path | What |
|---|---|
| `src/core/Types.*` | `HijriDate`, `Prayer`, `PrayerDay`, `Jumuah`, `Provenance`, `MethodPreset` |
| `src/core/HijriYear.*` | Gregorian ⇄ Hijri lookups over the provider's year |
| `src/core/EventCalendar.*` | the nine events, and the one rule that produces the sequence |
| `src/core/DayRules.*` | Friday: the khutbah replaces Dhuhr |
| `src/core/PrayerStateMachine.*` | `WaitingAdhan → WaitingIqama → InPrayer → next` |
| `src/providers/IqamahParser.*` | bytes in, value types out |
| `tests/` | 5 suites, 46 test functions, 178 assertions, no network |

## The state machine

The thing everybody gets wrong. Sunrise is not a prayer and is skipped; without an
iqama the prayer window starts at the adhan; after Isha the countdown continues to
tomorrow's Fajr when the caller provides it.

```
WaitingAdhan ──adhan──▶ WaitingIqama ──iqama──▶ InPrayer ──window──▶ WaitingAdhan
```

## Build and test

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.12.0/gcc_64
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Qt 6.5 or newer, Core only.

## Roadmap

1. `YearCache` — one JSON per year under `QStandardPaths`, read synchronously at startup.
2. `IqamahProvider` — `QNetworkAccessManager`, the two real calls, retry and staleness.
3. `ProviderChain` — iqamah → cached → predicted iqama offsets, each value carrying its source.
4. QML plugin: `IslamicTimes` singleton + `PrayerDayModel`.
5. Six widgets: prayer board, next prayer, next prayer + event, Ramadan, next event + Hijri date, jumua.

## Licence

MIT — see `LICENSE`.
