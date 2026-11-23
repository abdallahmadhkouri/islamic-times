# islamic-times

A C++/Qt library for Islamic time facts: what time it is for prayer, which Islamic
day it is, and what comes next. It is deliberately not a widget — widgets are views
built on top of it.

**Status:** core, parsers, tests and the behaviour report. The network client, the
cache, the QML plugin and the widgets follow.

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

## Seeing it

`tools/report` turns the library's own state into JSON, and `render.py` turns that
JSON into one page you can read at a glance:

```bash
./build/tools/report/it-report \
  --year tests/fixtures/year_times.json \
  --mosque tests/fixtures/mosque_day.json \
  --date 2027-02-08 --step 5 > report.json
python3 tools/report/render.py report.json > report.html
```

The page shows, for one real day: where every number came from (method, angles,
adjustments, iqama provenance), a 24-hour band chart of the four states, six tiles
showing what a widget would display at six moments, the prayer table, a half-hourly
state table, and a Hijri month calendar with the events on it.

It exists because a state machine is hard to trust from unit tests alone: the first
run of the report showed that the `InPrayer` state **never fired** — the tool's own
`--in-prayer` value defaulted to 0 — which no single-instant test had noticed. The day
sweep test now pins the whole 1440-minute path.

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

## Storage, and why it is Android-shaped

Cache is a backup, not the source of truth: the network is asked first, every launch
and every few hours; the cache fills the gaps and marks what it serves as stale.

| | |
|---|---|
| Where | `QStandardPaths::AppDataLocation` → on Android `/data/data/<package>/files`, app-private, no permission, no storage prompt, survives reboot, removed on uninstall |
| What | one JSON per year (`times-2027.json`, ~122 KB) and one per mosque day (`mosque-<id>-<date>.json`, ~1.7 KB) |
| How written | `QSaveFile` — temp file plus atomic rename, so an interrupted write can never leave a half file |
| How read | synchronously at startup; 122 KB parses in a few milliseconds, so the first frame is already correct instead of flashing placeholders |
| Why not SQLite | the access pattern is "one file per day, read once" — a database would add a dependency and size for no query |

Nothing is written outside the app's private directory, so the library needs no
storage permission on any Android version, and nothing breaks when the OS tightens
scoped storage further.

## Roadmap

1. `YearCache` — one JSON per year under `QStandardPaths`, read synchronously at startup.
2. `IqamahProvider` — `QNetworkAccessManager`, the two real calls, retry and staleness.
3. `ProviderChain` — iqamah → cached → predicted iqama offsets, each value carrying its source.
4. QML plugin: `IslamicTimes` singleton + `PrayerDayModel`.
5. Six widgets: prayer board, next prayer, next prayer + event, Ramadan, next event + Hijri date, jumua.

## Licence

MIT — see `LICENSE`.
