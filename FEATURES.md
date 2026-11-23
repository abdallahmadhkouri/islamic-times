# Features

*What the library does, what each widget shows, and what is deliberately absent.*

## The library

**Times** — a full year of calculated prayer times per location, from the provider,
with the method that produced them (`tn_ministry`, `mwl`, `isna`, `umm_al_qura`…),
Fajr/Isha as an angle or as an interval, the Asr rule, the high-latitude rule, the
per-prayer adjustments and the provider's own `defaults_version`. Nothing is
recomputed locally, so nothing can disagree with the source.

**The mosque** — nearest mosque within 10 km, adopted only when it publishes a
timetable; its own adhan and jamaa times, its Friday services, its freshness.

**Two sources of truth kept apart** — the mosque's call and the calculated time are
always both available, so a widget can say "calculated" when that is the truth.

**The smart countdown** — `WaitingAdhan → WaitingIqama → InPrayer → next`, with the
prayer window configurable, sunrise excluded, maghrib's zero-gap handled, and the
evening counting to tomorrow's Fajr. Works with or without a mosque: without one it
counts to the adhan and shows no jamaa.

**Hijri** — a lookup against the provider's own day records, never arithmetic; 730
consecutive days are a test fixture.

**Events** — nine, with the current and the next resolved, so Ramadan gives way to the
last ten nights on the 21st and to Laylat al-Qadr on the 27th, as asked.

**Friday** — Dhuhr becomes Jumua, the khutbah becomes the anchor, and a mosque with two
services keeps both.

**Offline** — a year of times and the last mosque day survive with no network; anything
served from cache is marked stale; a cold offline install shows dashes rather than
invented numbers.

**Worldwide** — Tunisia, France, Norway above the Arctic circle, Saudi Arabia: the
method is data, the Hijri is data, and a prayer that crosses midnight is recorded
rather than misplaced.

## The widgets

### NextPrayer — 1×1
The prayer that is coming, in one glance: its name in Amiri, the countdown in Tajawal,
and one quiet line carrying the state and the clock time ("إلى الأذان · 06:23"). The
accent colour is the state: gold for waiting, amber for the jamaa, teal while the
prayer is being performed.

### PrayerBoard — 2×2
All six rows, right-to-left, with the next row lit by a gold band and a bar on its
leading edge. Passed prayers recede; the adhan and the jamaa are separate columns; a
dash stands where there is no jamaa rather than a blank.

### NextPrayerEvent — 2×1
Two halves: the prayer that is coming, and the feast after it, with what is happening
now underneath. Built for the long view: "٢٠ يومًا" to the last ten nights is the kind
of fact a family actually wants on the wall.

### Ramadan — 2×1
The fast as a span: the day number, imsak and iftar, a countdown to whichever end is
next, and a line that fills as the day is fasted.

### HijriEvent — 1×1
Today's Hijri date, large, with the next event and how far away it is.

### Jumua — 2×1
On Friday: the khutbah time and the countdown to it. On any other day: how many days
until Friday, so the tile is never empty and never lies.

## Deliberately absent

- **No local prayer calculation.** No network and no cache means dashes. Adding a
  calculator would create a second source of truth.
- **No scraping of other apps.** Mawaqit refused; iqamah is the source, and its country
  fallback is good enough where it has no mosque.
- **No adhan audio, no Qibla, no Quran, no taraweeh tracking** in this library.
- **No settings UI.** Every parameter the settings popup will need (window minutes,
  radius, minor events, mosque choice) already exists in the API surface.
- **No per-client mosque entry yet.** If the user's mosque publishes nothing, the app
  uses the country calculation — the same thing the official app does.
