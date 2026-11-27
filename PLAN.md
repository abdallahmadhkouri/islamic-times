# Plan

Written 2026-11-23. Each phase ends with green tests and one sentence about what a
consumer would notice.

## Done

| Phase | Result |
|---|---|
| P1 cache | `PayloadCache`, raw bytes, atomic writes, keys, pruning — 9 tests |
| P2 client | `IqamahClient` + `QNetworkTransport` + fake transport — 13 tests; live smoke green |
| P3 chain | `IslamicTimes::dayFor` — online, warm offline, cold offline, no-source mosque — 7 tests |
| P4 poles | `adhanDayOffset`; Oslo and Tromsø pinned by tests |

## Next

| Phase | Work | Gate |
|---|---|---|
| P5 | Optional QML binding (`IslamicTimes` singleton + `PrayerDayModel`) so QML consumers do not hand-roll a map | a QML smoke app binds hijri, next prayer, state and seconds |
| P6 | Package: `find_package(islamic-times)` + install rules, so a consumer adds one dependency | a fresh project links and runs against the installed package |
| P7 | Android pass: app-private cache path, TLS on device, airplane-mode launch, cold install | observed on a device, not inferred |

## Consumers

Everything visual lives in the projects that consume this library; nothing about a
particular screen belongs here. That separation is the point: the library is generic,
the screens are not.
