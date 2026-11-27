# Features

What the library gives a Qt project, and what it deliberately does not.

## Times

A full year of calculated prayer times per location, with the method that produced
them: the id and name (`tn_ministry`, `mwl`, `isna`, `umm_al_qura`…), Fajr and Isha as
an angle **or** as an interval, the Asr rule, the high-latitude rule, the per-prayer
adjustments map and the provider's own `defaults_version`. Every field is carried, none
is interpreted. Nothing is recomputed, so nothing can disagree with the source.

## The mosque

Nearest mosque within a configurable radius (2, 5, 10, 25 or 50 km — the API's own
ladder), adopted only when the provider says it publishes a timetable. Its adhan and
jamaa times, its Friday services, its freshness and staleness. `has_source` is the
gate: a pin without a timetable is reported as such and never dressed up.

## Two adhan times, never merged

`mosqueAdhan` is what the mosque calls, `methodAdhan` is what the calculation says,
`adhan` is the effective one. A consumer can therefore say "calculated" when that is
the truth, instead of silently presenting a computed time as the mosque's.

## Countdown state machine

    WaitingAdhan ──adhan──▶ WaitingIqama ──iqama──▶ InPrayer ──window──▶ next

- Sunrise is excluded: it is an astronomical event, not a prayer.
- No jamaa → the window starts at the adhan, and no iqama is invented.
- `iqama == adhan` (maghrib) goes straight to the prayer window.
- After Isha, with tomorrow's day supplied, it counts to tomorrow's Fajr.
- The window length is a parameter, because Fajr ends at sunrise and Maghrib does not.
- A prayer that falls after midnight carries a day offset, so the evening names the
  right prayer above 55°N in summer.

## Hijri

A lookup against the provider's own `(Gregorian, Hijri)` pairs, never arithmetic. 730
consecutive real days are a test fixture; Qt's Islamic calendars were measured a day
late on 13 of 24 month starts.

## Events

Nine events as instants — Ramadan, the last ten nights, Laylat al-Qadr, Eid al-Fitr,
Arafah, Eid al-Adha, the new year, Ashura, the Mawlid — with the current and the next
resolved. Two contested observances are in the table but opt-in.

## Friday

On a Friday the Dhuhr row becomes Jumua, the khutbah becomes the anchor, a missing
khutbah falls back to the service time, and a mosque with two services keeps both.

## Cache and offline

One file per payload, written with `QSaveFile` (temp + atomic rename), keyed by rounded
location and day, aged out by policy. On Android it lives in the app's private data
directory: no permission, no prompt, gone on uninstall. Offline the library serves the
cached year, marks what is old, and says so — or reports that it has nothing.

## Worldwide

Measured against real payloads: Tunisia (ministry method), Paris (MWL, a mosque with a
timetable), Chicago (ISNA), Makkah (Umm al-Qura, Isha as a 90-minute interval with no
angle) and Norway above the Arctic circle, where prayers do not occur at all in June
and the provider sends null. Nulls are carried, not patched.

## Errors

One small enum — `InvalidArgument`, `Network`, `HttpStatus`, `Redirect`, `Empty` — with
a sentence for each. One retry, only where a retry can help. Redirects are reported, not
followed: this API answers 301 for an unknown parameter, and following it would turn a
mistake into a silent success.

## Deliberately not included

- **Any UI**: no QML, no fonts, no colours, no Arabic strings.
- **Local prayer calculation**: no network and no cache means no times.
- **Scraping**: iqamah is the source; where it has no mosque, its country calculation
   is the answer.
- **Adhan audio, Qibla, Quran, taraweeh tracking.**
- **A settings UI**: every parameter a settings screen needs (window minutes, radius,
   minor events, mosque choice) already exists in the API.
