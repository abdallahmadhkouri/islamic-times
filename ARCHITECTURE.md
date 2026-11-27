# Architecture

*How `islamic-times` is built, why each boundary sits where it does, and what was
worked around to get here. Written 2026-11-23.*

---

## 1. The problem, in one paragraph

Any Qt application that shows prayer times needs the same five facts repeatedly:
today's Hijri date, the five prayer times, the local mosque's jamaa times, whether it
is Friday, and what the next Islamic event is. It needs them at startup, from a cold
install and from a warm cache, on a device with no GPS, no API key and no permission,
and on a network that may be absent. It needs them identical whether the caller is a
launcher, a kiosk, a service or a desktop app. That means one copy of the logic in a
library, with the provider's data carried faithfully and every display decision left
to the caller.

## 2. Layers

```
   consumer (anything: QML, widgets, a service, a CLI)
                    │  DayResult / PrayerSnapshot / IslamicEvent
   ┌────────────────▼─────────────────────────────────────────┐
   │ chain  IslamicTimes: policy, assembly, fallback          │
   └───────▲──────────────────────▲───────────────────────────┘
           │                      │
 ┌─────────┴───────────┐   ┌──────┴─────────────┐
 │ providers           │   │ IqamahParser       │
 │ IqamahClient (URLs, │   │ bytes → value types│
 │ retries, errors)    │   └──────▲─────────────┘
 └─────────▲───────────┘          │
           │                      │
 ┌─────────┴───────────┐   ┌──────┴─────────────┐
 │ store  PayloadCache │   │ raw payloads,      │
 │                     │   │ untouched          │
 └─────────▲───────────┘   └────────────────────┘
           │
 ┌─────────┴──────────────────────────────────────────────┐
 │ core  Types · HijriYear · EventCalendar · DayRules ·   │
 │       PrayerStateMachine                               │
 └────────────────────────────────────────────────────────┘
```

| Layer | Owns | Must not |
|---|---|---|
| `core` | value types, Hijri lookups, the event table, Friday rules, the state machine | touch disk or network, format text |
| `store` | raw payload files, atomic writes, age, pruning | parse or interpret |
| `providers` | HTTP, URL building, parsing, error mapping | cache, or choose a source |
| `chain` | freshness policy, mosque adoption, assembly, offline behaviour | know what the caller draws |

`islamic-times` (core) links **Qt Core only**. `islamic-times-net` adds Qt Network for
the real transport. A consumer that never fetches never links the network stack.

## 3. The provider contract, as measured

Base: `https://api.iqamah.net/v1`, keyless, HTTPS.

| Call | Shape | Notes |
|---|---|---|
| `GET /times/{lat},{lon}/{year}.json` | 365 days, each with `fajr…isha`, `sunrise`, `hijri` | ~122 KB; calculated times only, no jamaa |
| `GET /mosques/near/{lat},{lon}.json?radius_km=` | `mosques[]` with `id, name, distance_km, city, has_source` | radius exactly 2/5/10/25/50; **10 is the default and must be omitted** |
| `GET /mosque/{id}.json` | one day with `azaan.mosque`, `azaan.method`, `iqamah`, `jumuah[]`, `status` | ~1.7 KB; `status: none` when the mosque publishes nothing |

Three behaviours that are tests, not folklore:

1. **No User-Agent → 403.** The client always sends one naming the library.
2. **`?radius_km=10` → 301** to the bare URL, because 10 is the default. The client
   omits it. Any unknown query is also a 301, so the transport refuses redirects and
   reports them.
3. **`%2C` instead of `,` in the path → 404.** The comma stays literal.

`has_source` is the gate for everything mosque-shaped: a pin with `has_source: false`
has no timetable, and its day comes back with `status: "none"`, every `iqamah` null, and
the country calculation in `azaan.method`.

## 4. The model

```cpp
struct Prayer {
    PrayerKey key;
    QTime mosqueAdhan;      // what the mosque calls (invalid if it publishes none)
    QTime methodAdhan;      // the provider's calculated time
    QTime adhan;            // effective: mosque if valid, else method
    QTime iqama;            // invalid = no jamaa
    int  adhanDayOffset;    // 1 when the prayer falls after midnight
    int  iqamaDayOffset;
};
```

Two adhan times, never collapsed. The provider distinguishes "this is what the mosque
calls" from "this is what the calculation says", and a consumer may want to show that
distinction. Merging them destroys information the caller cannot recover.

`adhanDayOffset` exists because of Oslo: on 21 June the provider prints Isha at 01:19,
after Maghrib at 22:44. The clock time is left exactly as sent; the offset records the
ordering the sequence already implies, and the state machine adds the days.

```cpp
struct Provenance {
    TimesSource source;     // mosque | calculated
    DayStatus status;       // fresh | stale | none | unknown
    QDate publishedFor;
    QDateTime asOf;
    bool stale, predicted;
};
```

`DayStatus::None` is a first-class answer — "this mosque has no timetable" — not an
error and not a reason to invent one.

## 5. The state machine

```
WaitingAdhan ──adhan──▶ WaitingIqama ──iqama──▶ InPrayer ──window──▶ next WaitingAdhan
```

- **Sunrise is skipped.** It is an astronomical event, not a prayer; counting to it
  would hide Dhuhr for hours.
- **No iqama** → the prayer window starts at the adhan, and no jamaa is ever invented.
- **Maghrib with `iqama == adhan`** → straight to `InPrayer`, no `WaitingIqama`.
- **After Isha** with tomorrow's day supplied → counts to tomorrow's Fajr.
- **Window** is a parameter (default 20 minutes), because the right answer for Fajr
  (until sunrise) is not the right answer for Maghrib.

The whole 1,440-minute day is swept by a test: no minute unclassified, no target moving
backwards, five iqama entries and five prayer windows.

## 6. Events

Nine events, each an **instant**, not a span: Ramadan (1/9), last ten nights (21/9),
Laylat al-Qadr (27/9), Eid al-Fitr (1/10), Arafah (9/12), Eid al-Adha (10/12), new year
(1/1), Ashura (10/1), Mawlid (12/3). Two contested observances (mid-Sha'ban, Isra) are in
the table but opt-in.

`around()` returns the most recent event that has started and the next one, so on
21 Ramadan the current event becomes the last ten nights and Ramadan becomes the period
rather than the target. Ramadan's day number therefore comes from the Hijri date, not
from the event.

## 7. Cache

| | |
|---|---|
| Where | `QStandardPaths::AppDataLocation/iqamah` → on Android `/data/data/<pkg>/files/iqamah`, app-private, no permission |
| What | the provider's **bytes**, in a JSON envelope with `fetched_at` and `http_status` |
| Write | `QSaveFile` — temp + atomic rename, so a crash cannot truncate a payload |
| Read | synchronously at startup (122 KB parses in milliseconds) |
| Keys | `year-36.80-10.18-2027`, `near-34.55-9.48-10`, `mosque-<uuid>-2026-11-23`; validated against traversal |
| Age | year 45 days, nearby 30 days, mosque day 6 hours |

Storing raw bytes is what lets the model change without a migration, and it makes the
cache auditable: what is on disk is what the provider said, byte for byte.

## 8. Chain policy

1. Year: cache if present and younger than 45 days, else fetch; on failure fall back to
   the older cache and say so.
2. Mosque: `near?radius_km=10`; adopt only a mosque with `has_source == true`; an empty
   answer is cached, because "no mosque in this town" is an answer.
3. Mosque day: cache for 6 hours, refetch on expiry; `status: none` leaves the calculated
   day intact and produces a note.
4. Assembly: overlay only what the mosque publishes; the rest stays calculated.
5. Offline: cached year + last mosque day (marked), or no jamaa at all.

## 9. Testing

| Tier | What | Where |
|---|---|---|
| unit | parsing, arithmetic, events, state machine, cache | `tests/tst_*.cpp` |
| transport | URLs, UA, 403, 301, retries, empty bodies | `tst_iqamahclient` via `FakeTransport` |
| chain | online, warm offline, cold offline, no-source mosque | `tst_chain` |
| worldwide | Oslo midsummer, Tromsø polar, Umm al-Qura interval | `tst_worldwide` |
| live | the real API, real TLS | `tools/smoke/it-smoke` |

Ten suites, 364 assertions. **Eleven defects were caught by these tests and tools, not
by review:**

1. Sunrise was being counted down to instead of Dhuhr.
2. Jumuah times were dropped: real payloads carry them in `prayer`, not `khutbah`.
3. A mosque with two Friday services lost the second one.
4. The Tunisian year uses `tn_ministry`, not MWL — any hardcoded method would be wrong.
5. The provider flips `status` between fresh and stale, so provenance must be parsed.
6. `InPrayer` never fired: the report tool's own `--in-prayer` defaulted to 0.
7. `?radius_km=10` is answered with a 301; the default must be omitted.
8. An empty nearby cache triggered a refetch (and a retry) on every launch.
9. `moc` cannot parse a braced raw string at namespace scope — the payload moved to a file.
10. Oslo's Isha-after-midnight made the consumer name the wrong prayer.
11. A mosque's own adhan and the calculated one were being merged into one field.

## 10. Deliberate workarounds

- **Synchronous HTTP.** One blocking GET on a worker thread rather than a signal dance;
  it makes the fake transport trivial and every failure path testable.
- **Day offsets instead of full instants.** Storing `QDateTime` per boundary is the
  cleaner model; `adhanDayOffset` achieves the same ordering with a much smaller blast
  radius. The instant form stays on the roadmap.
- **A re-dated mosque fixture.** `mosque_2027-02-08.json` keeps a real payload's shape
  but is dated to the year fixture's Ramadan day; it is labelled as such.
- **A behaviour report that reads fixtures.** `tools/report` renders a day's state
  timeline from a payload, without a network, so a change to the state machine can be
  reviewed as a picture.

## 11. Not in scope

Local prayer calculation, any UI, Arabic strings, fonts, audio, Qibla and Quran. The
library answers "what are the times and what is happening now"; what that looks like is
the consumer's business.
