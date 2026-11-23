# Plan — from a tested core to a widget that runs on the TV

Written 2026-11-23. Nine phases, each one ending with green tests and one sentence
describing what a user would notice. Nothing moves to the next phase until the
current one passes.

## The line between the layers

| Layer | Owns | May not |
|---|---|---|
| `core/` | types, Hijri lookups, events, the prayer state machine | touch the network, read files, format text |
| `store/` | raw payload cache, settings | parse or interpret payloads |
| `providers/` | HTTP, URL building, parsing | cache, or decide which source wins |
| `chain/` (IslamicTimes) | choosing source, assembling a day, offline policy | know what a widget looks like |
| QML plugin | types and models for Qt Quick | contain Arabic wording |
| widgets | appearance and Arabic | do arithmetic on times |

Two rules that come before the plan: **the cache stores the provider's bytes
untouched** (so our model can change without a migration, and so we never keep a
"repaired" copy of their data), and **the library is stateless about location** —
the app passes `lat`/`lon`, because the launcher already has `cityLat`/`cityLng`
from its city picker.

## Phases

### P1 — Payload cache (no network)

`store/PayloadCache`: one file per payload, `QSaveFile` for atomic writes, a small
JSON envelope carrying `fetched_at` and `http_status`, key sanitising that makes
path traversal impossible, pruning by age, and a total-size report.

*Done when:* write/read round-trip, atomicity, key sanitising, pruning and missing
file all have tests, and the tests pass on a fresh temporary directory.

### P2 — Transport and client

`providers/HttpTransport` (interface) and `providers/QNetworkTransport`
(QtNetwork, in a separate `islamic-times-net` target so the core stays QtCore-only).
`providers/IqamahClient` builds the three URLs, sets a User-Agent, refuses
redirects, applies a timeout and one retry, and maps failures to a small error
enum. Tests drive it through a `FakeTransport`, so the suite never touches the
network.

Encoded from measurement: the API answers **403 without a User-Agent**, and
**301 on an unknown query** (`?radius=60`). Both become test cases, not folklore.

*Done when:* URL construction, the radius ladder (2/5/10/25/50 only), the UA
header, a 403, a 500, a timeout and a malformed body each have a test.

### P3 — The chain, and what a widget actually gets

`IslamicTimes::day(lat, lon, now)` → `DayResult { PrayerDay, MethodPreset,
Mosque (optional), Provenance }`.

Policy, in order:
1. Year payload: from cache if present and the year is current; otherwise fetch and
   cache. It supplies the calculated adhan for every day of the year.
2. Mosque: `near?radius_km=10` (never 25, never a neighbouring town). Accept only
   `has_source == true`. Cache the chosen id per rounded location.
3. Mosque day: cache for today, refetch when older than 6 h or `status != fresh`.
4. Assemble: mosque adhan and iqama where they exist, calculated adhan otherwise,
   Jumua on Friday, provenance carried through.
5. Offline: cached year + last mosque day marked stale, or no iqama at all.

*Done when:* an online run, an offline run with a warm cache, an offline run with
an empty cache, a `status: none` mosque and a Tuesday-vs-Friday day all produce the
expected `DayResult`, tested through the fake transport.

### P4 — A day that survives the poles

Store boundaries as instants (the payload ships `utc`; mosque payloads do not, so
rollover is derived by ordering, not by repairing a value). Add a
`PrayerState::DoesNotOccur` for polar days. Oslo's Isha-after-midnight and Tromsø's
null Maghrib become ordinary cases rather than documented exceptions.

*Done when:* the existing Oslo and Tromsø tests assert real snapshots instead of
characterising a defect.

### P5 — QML plugin

`qml/IslamicTimes` singleton (properties: hijri date, next prayer, state, seconds,
mosque name, provenance) and `PrayerDayModel` for lists. Arabic stays in QML.

*Done when:* a QML smoke app binds all six properties to a cached day and renders
without warnings.

### P6 — Desktop inspector

The tool that answers "show me": every widget at every grid size, a clock slider
across 24 h, a source switch (live / cache / empty / no mosque), and snapshot
output per widget/size. This is where widget design gets reviewed, not on Android.

*Done when:* the inspector renders the six widgets at six sizes from one command and
writes dated PNGs.

### P7 — One widget in MomosLauncher

The prayer board first, replacing the current widget behind a chooser entry, as an
MR. Then the next five, one MR each, in the order: next prayer, next prayer + event,
Ramadan, next event + Hijri, Jumua.

*Done when:* the launcher builds, the widget shows today's data, and the MR is
reviewable.

### P8 — Android reality

APK on the TV AVD and on the Mi Box: app-private cache path exists, TLS works on
device, airplane-mode launch shows the cached day, a cold install with no network
shows the country calculation from cache or dashes, and nothing writes outside
`AppDataLocation`.

*Done when:* the four scenarios are observed on a device, not inferred.

## Failure modes and what the user sees

| Situation | What the widget shows |
|---|---|
| No network, warm cache | today's times, mosque name dimmed, iqama marked as not fresh |
| No network, cold install | dashes for times; no guessing |
| Mosque found, `has_source` true | name + distance in small text, iqama layer active |
| Mosque found, `has_source` false | no mosque line, country calculation, no iqama |
| Nothing within 10 km | country calculation, no mosque line |
| Polar day | the prayers that do not occur shown as absent, not skipped |

## Testing tiers

| Tier | Where | What it proves |
|---|---|---|
| T0 | C++ unit, fixtures | parsing, arithmetic, state machine, cache |
| T1 | FakeTransport | URL, headers, retries, every error path, offline policy |
| T2 | live smoke on the VM | the real API answers as expected today |
| T3 | Android device | TLS, storage path, airplane mode |

T0 and T1 run on every change; T2 runs by hand when the API changes; T3 before a
release.

## Out of scope for now

Adhan audio, Qibla, Quran, taraweeh tracking, multi-mosque favourites, the settings
popup UI (the library exposes the parameters, the UI comes with the launcher work),
and any local prayer-time calculation.
