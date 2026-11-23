# Decisions — what is standard, what is mine, what is still open

Every entry is a decision the library makes that a reader could reasonably question.
Statuses: **settled** (evidence-backed), **chosen** (a defensible default, changeable),
**open** (I need Saif's call).

---

## 1. Sunrise is not a countdown target — settled

The state machine skips `Sunrise`. Counting to sunrise would answer "sunrise in 45 min"
at 06:15 and hide Dhuhr behind it for an hour. Sunrise is still in the day, so widgets
can show it (and the Ramadan widget needs it for suhoor/imsak).

The day sweep test (`theWholeDayIsCoveredAndEveryWindowHolds`) walks all 1440 minutes
and fails if any minute is unclassified or a target moves backwards.

## 2. The prayer window is a flat 20 minutes — OPEN, needs Saif

After the iqama the tile shows "prayer in progress" for `inPrayerMinutes`, default 20.
Nothing in the data says how long the congregation takes. Options: keep 20 for all,
per-prayer values (maghrib shorter, isha longer), until the next adhan, or a setting.
My recommendation: keep 20 as the default but expose it in the library call, so the
future widget settings popup can override it per widget.

## 3. Jumua replaces Dhuhr on Friday — chosen, one part open

What the code does, exactly:

- `DayRules::isFriday(date)` → the day's Dhuhr becomes **جمعة**.
- The anchor becomes the **khutbah** time: `dhuhr.adhan = jumuah[].khutbah`,
  `dhuhr.iqama = jumuah[].azaan`.
- If `khutbah` is null, the service time in `prayer` is used instead — real payloads
  send both shapes.
- If a mosque publishes several services, the **first one with a usable time** wins.

**Open:** (a) whether the countdown should target the khutbah start or the prayer
start — I target the khutbah, because that is what a person must not miss; (b) whether
the user should be able to pick service #2 when a mosque has two. My recommendation:
target the khutbah, and add a service selector only when a mosque actually publishes
more than one (rare, but it happens — the Paris payload has two).

## 4. Hijri dates come from the provider, never from arithmetic — settled

The provider ships `(Gregorian day, Hijri day)` for a whole year, so every Hijri answer
is a lookup. Qt's Islamic calendars were measured one day late on **13 of 24** month
starts in 1448, including 1 Shawwal (Eid al-Fitr). A computed calendar can be wrong in
a way nobody notices for months; a lookup is wrong only when the data is.

Consequence: the year payload must be refetched when a month actually starts, because
the provider itself may shift by a day. That is the "fresh data matters more than
cache" rule, and it is why the Hijri date is read from the newest payload, not the
oldest cached one.

## 5. No local prayer calculation — settled by Saif

If there is no payload, the widget shows a dash. No fallback calculator, no guessed
times. `CalculatedProvider` stays out of the library until there is a real reason.

## 6. Events are instants, not spans — chosen

`ramadan` is 1 Ramadan, `lastten` is 21 Ramadan, `qadr` is 27 Ramadan, `fitr` is
1 Shawwal, `adha` is 10 Dhuwal-Hijjah, and so on. The "current" event is the most
recent one that has started. So on 21 Ramadan the widget's hero becomes
**العشر الأواخر** and Ramadan is no longer the current event — it has become the
period you are inside, not the thing you are counting to. This is what Saif asked
for ("once Ramadan started it should be the last ten nights, then the 27th").

Consequence worth knowing: during the last ten nights the Ramadan day number (e.g.
"day 23") has to come from the Hijri date, not from the current event.

## 7. Minor events are off by default — chosen

Mid-Sha'ban and Isra wal-Miraj are in the table but excluded unless
`EventCalendar::events(/*includeMinor=*/true)`. They are contested as observances and
would add noise to a six-slot grid.

## 13. Which mosque — chosen, and gated by the provider's own flag

The user gives a city, so we have a latitude and a longitude. The lookup is
`GET /v1/mosques/near/{lat},{lon}.json?radius_km=`, and the API accepts exactly
2, 5, 10, 25 and 50 km.

Measured for Sidi Ali Ben Aoun: **0** mosques within 10 km, **1** within 50 km
(NiCd Valley Mosque, 17.36 km, `has_source: false`). Fetching that mosque returns
`status: "none"`, every `iqama` null, and the Tunisian ministry calculation in
`azaan.method` — 05:35, 12:16, 14:54, 17:17, 18:42.

Saif confirmed the same behaviour in the official app: choosing his position
automatically ended on the Tunisian ministry, with no mosque offered. He prefers
that to being shown a mosque that is not his.

Note what the ladder allows: **2, 5, 10, 25, 50** km — there is no 20. So the real
choice is 10 or 25, and 25 is exactly the one that would drag in Bu Ballal at
17.36 km. Hence:

- Query at **10 km** — a town, not a region.
- Show the mosque's name and distance in small text **only** when it is found, and
  only claim it as "your mosque" when `has_source` is true.
- Never adopt a mosque 17 km away: its jamaa belongs to another town.
- No mosque (or `has_source: false`) means the **iqama layer is absent**, not
  broken: the widget counts to the adhan and looks finished without it.

Consequence for the design: the iqama countdown is an *optional second layer*, not
the baseline. `tst_nosource` walks a whole day that has no iqama at all.

## 8. Predicted iqama = the mosque's own offsets — chosen

The mosque endpoint publishes **today** (about 1.7 KB). Tomorrow's iqama is today's
`iqama − adhan` offsets applied to tomorrow's calculated adhan, and the result is
flagged `provenance.predicted = true`. This is how the whole year can still show iqama
while offline. If a mosque changes its iqama, the next daily fetch corrects it and the
widget stops saying "predicted".

## 9. Maghrib with a zero gap — settled

Real payloads send `iqama == adhan` for maghrib. The machine then goes straight from
`WaitingAdhan` to `InPrayer`, with no `WaitingIqama` state. That is correct, and the
day sweep test pins it (five prayers, five iqama entries, five prayer windows).

## 10. Staleness is read, never assumed — settled

`status: "fresh" | "stale"`, `stale: bool` and `extracted_at` all change between days
and between mosques. The parser copies them into `Provenance`; the UI decides how to
show them. Nothing is hardcoded, and both paths are covered by tests.

## 14. Eid is announced by the provider, not by us — settled by Saif

The app will not print "subject to announcement" on the 29th and 30th. Iqamah updates
the Hijri with the official announcement, and the daily mosque fetch plus the year
payload are what carry it. So the rule is a freshness rule: re-read today's Hijri from
the newest payload, never from an older cache, and refetch the mosque day every six
hours and on resume. If the provider is a day late, so are we — identically, and
without inventing a second opinion.

## 12. We never repair the provider's payload — settled (a rule, not a preference)

If a field is null it stays null; if a value looks odd it is passed through; if a
method field is missing we show nothing rather than infer it. The provider owns its
data and may fix or change it at any time, so any correction we apply locally would
be a lie that outlives their fix.

Two consequences already implemented:

- The day carries the provider's own `status`: **fresh**, **stale** or **none**.
  `none` means "this mosque has no published timetable" — a real answer, shown as
  the absence of iqama, never as an error and never as a substituted value.
- The provider sends **two** adhan times and both are kept: `mosqueAdhan` (what the
  mosque calls) and `methodAdhan` (what the calculation says). The effective
  `adhan` is the mosque's when it exists. Nothing is overwritten, so a widget can
  still tell the user "this is the calculated time, not your mosque's".

Rejected on this basis: inferring Umm al-Qura's 90-minute Isha interval from the
numbers when the payload sends `isha: null`. If they send null, we display a dash.

## 11. Method is data, not a constant — settled

`MethodPreset` keeps everything the provider sends: `id`, `name`, Fajr and Isha as an
angle **or** as a fixed interval (both shapes exist), `asr`, `highlat`, the per-prayer
`adjustments` map, and `defaults_version`. The library never invents a method, so the
same code works in any country — and a future settings popup can show the method and
let the user override it without a schema change.
