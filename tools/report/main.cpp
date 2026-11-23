// it-report — turns the library's own state into JSON, so the behaviour can be
// *seen* (rendered to a picture) instead of described. It is a tool, not part of
// the library: it links the library exactly like a widget would.
//
//   it-report --year <year_times.json> [--mosque <mosque_day.json>] \
//             --date YYYY-MM-DD [--step 5] [--in-prayer 20]

#include "core/DayRules.h"
#include "core/EventCalendar.h"
#include "core/HijriYear.h"
#include "core/PrayerStateMachine.h"
#include "providers/IqamahParser.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDate>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QTextStream>

using namespace islamic;

namespace {

QByteArray readAll(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return file.readAll();
}

QString stateName(PrayerState state)
{
    switch (state) {
    case PrayerState::WaitingAdhan: return QStringLiteral("waitingAdhan");
    case PrayerState::WaitingIqama: return QStringLiteral("waitingIqama");
    case PrayerState::InPrayer: return QStringLiteral("inPrayer");
    case PrayerState::Idle: break;
    }
    return QStringLiteral("idle");
}

QJsonObject prayerJson(const Prayer &prayer)
{
    QJsonObject object;
    object.insert(QStringLiteral("key"), prayerKeyId(prayer.key));
    object.insert(QStringLiteral("adhan"), prayer.hasAdhan() ? prayer.adhan.toString(QStringLiteral("HH:mm")) : QString());
    object.insert(QStringLiteral("iqama"), prayer.hasIqama() ? prayer.iqama.toString(QStringLiteral("HH:mm")) : QString());
    return object;
}

QJsonObject dayJson(const PrayerDay &day)
{
    QJsonObject object;
    object.insert(QStringLiteral("date"), day.date.toString(Qt::ISODate));
    object.insert(QStringLiteral("hijri"), QStringLiteral("%1-%2-%3")
                      .arg(day.hijri.year).arg(day.hijri.month, 2, 10, QLatin1Char('0'))
                      .arg(day.hijri.day, 2, 10, QLatin1Char('0')));
    object.insert(QStringLiteral("isFriday"), DayRules::isFriday(day.date));

    QJsonArray prayers;
    for (const Prayer &prayer : day.prayers)
        prayers.append(prayerJson(prayer));
    object.insert(QStringLiteral("prayers"), prayers);

    QJsonObject provenance;
    provenance.insert(QStringLiteral("source"), int(day.provenance.source));
    provenance.insert(QStringLiteral("stale"), day.provenance.stale);
    provenance.insert(QStringLiteral("predicted"), day.provenance.predicted);
    provenance.insert(QStringLiteral("asOf"), day.provenance.asOf.toString(Qt::ISODate));
    object.insert(QStringLiteral("provenance"), provenance);
    return object;
}

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QCommandLineParser parser;
    parser.addOption({ QStringLiteral("year"), QStringLiteral("year_times.json"), QStringLiteral("path") });
    parser.addOption({ QStringLiteral("mosque"), QStringLiteral("mosque_day.json (for iqama offsets)"), QStringLiteral("path") });
    parser.addOption({ QStringLiteral("date"), QStringLiteral("YYYY-MM-DD"), QStringLiteral("date") });

    QCommandLineOption stepOption(QStringLiteral("step"),
                                  QStringLiteral("minutes between samples"),
                                  QStringLiteral("minutes"), QStringLiteral("5"));
    QCommandLineOption windowOption(QStringLiteral("in-prayer"),
                                    QStringLiteral("minutes of the prayer window"),
                                    QStringLiteral("minutes"), QStringLiteral("20"));
    parser.addOption(stepOption);
    parser.addOption(windowOption);
    parser.process(app);

    // Two ways to run: a year payload (pick a day out of it, optionally dress it
    // with a mosque's iqama offsets), or a single mosque day on its own — which
    // is how a real mosque's published day gets checked against the library.
    QVector<PrayerDay> year;
    MethodPreset method;
    QString error;
    const bool haveYear = parser.isSet(QStringLiteral("year"));
    if (haveYear && !IqamahParser::parseYearTimes(readAll(parser.value(QStringLiteral("year"))), &year, &method, &error)) {
        QTextStream(stderr) << "year payload: " << error << Qt::endl;
        return 1;
    }

    PrayerDay day;
    if (haveYear) {
        const QDate date = QDate::fromString(parser.value(QStringLiteral("date")), Qt::ISODate);
        const PrayerDay *picked = nullptr;
        for (const PrayerDay &entry : year) {
            if (entry.date == date) {
                picked = &entry;
                break;
            }
        }
        if (!picked) {
            QTextStream(stderr) << "date not in the payload" << Qt::endl;
            return 1;
        }
        day = *picked;
        day.provenance.predicted = false;
    }

    QJsonObject mosqueJson;
    QMap<QString, int> offsets;
    if (parser.isSet(QStringLiteral("mosque"))) {
        PrayerDay mosqueDay;
        if (!IqamahParser::parseMosqueDay(readAll(parser.value(QStringLiteral("mosque"))), &mosqueDay, &error)) {
            QTextStream(stderr) << "mosque payload: " << error << Qt::endl;
            return 1;
        }
        for (const Prayer &prayer : mosqueDay.prayers) {
            if (prayer.hasAdhan() && prayer.hasIqama())
                offsets.insert(prayerKeyId(prayer.key), prayer.adhan.secsTo(prayer.iqama) / 60);
        }

        if (haveYear) {
            // The "the mosque published today, we extrapolate tomorrow" path.
            for (auto it = offsets.constBegin(); it != offsets.constEnd(); ++it) {
                if (Prayer *target = day.prayer(prayerKeyFromId(it.key())))
                    target->iqama = target->adhan.addSecs(it.value() * 60);
            }
            day.provenance.predicted = true;
        } else {
            day = mosqueDay;    // the mosque's own published day, untouched
        }

        mosqueJson.insert(QStringLiteral("date"), mosqueDay.date.toString(Qt::ISODate));
        mosqueJson.insert(QStringLiteral("provenance"), QJsonObject{
            { QStringLiteral("source"), int(mosqueDay.provenance.source) },
            { QStringLiteral("stale"), mosqueDay.provenance.stale },
            { QStringLiteral("asOf"), mosqueDay.provenance.asOf.toString(Qt::ISODate) }
        });
    } else if (!haveYear) {
        QTextStream(stderr) << "need --year, --mosque, or both" << Qt::endl;
        return 1;
    }

    // Events: a HijriYear over the payload, so the report's calendar is real.
    HijriYear hijriYear;
    if (haveYear) {
        for (const PrayerDay &entry : year)
            hijriYear.add(entry.date, entry.hijri);
    }
    const CalendarSnapshot snapshot = EventCalendar::around(day.hijri, day.date, hijriYear);

    QJsonObject root;
    root.insert(QStringLiteral("day"), dayJson(day));

    QJsonObject methodJson;
    methodJson.insert(QStringLiteral("id"), method.id);
    methodJson.insert(QStringLiteral("name"), method.name);
    methodJson.insert(QStringLiteral("fajrAngle"), method.fajrAngle);
    methodJson.insert(QStringLiteral("ishaAngle"), method.ishaAngle);
    methodJson.insert(QStringLiteral("asr"), method.asr);
    methodJson.insert(QStringLiteral("highLat"), method.highLat);
    methodJson.insert(QStringLiteral("defaultsVersion"), method.defaultsVersion);
    QJsonObject adjustments;
    for (auto it = method.adjustments.constBegin(); it != method.adjustments.constEnd(); ++it)
        adjustments.insert(it.key(), it.value());
    methodJson.insert(QStringLiteral("adjustments"), adjustments);
    root.insert(QStringLiteral("method"), methodJson);

    QJsonObject offsetsJson;
    for (auto it = offsets.constBegin(); it != offsets.constEnd(); ++it)
        offsetsJson.insert(it.key(), it.value());
    root.insert(QStringLiteral("iqamaOffsets"), offsetsJson);
    if (!mosqueJson.isEmpty())
        root.insert(QStringLiteral("mosque"), mosqueJson);

    // The day after, with the same times: only so the after-Isha state can be
    // shown counting to tomorrow's Fajr instead of stopping at Idle.
    PrayerDay tomorrow = day;
    tomorrow.date = day.date.addDays(1);

    const int step = qMax(1, parser.value(stepOption).toInt());
    const int window = qMax(1, parser.value(windowOption).toInt());
    QJsonArray states;
    for (int minute = 0; minute < 24 * 60; minute += qMax(1, step)) {
        const QDateTime now(day.date, QTime(minute / 60, minute % 60));
        const PrayerSnapshot state = evaluate(day, &tomorrow, now, window);
        QJsonObject entry;
        entry.insert(QStringLiteral("at"), now.toString(QStringLiteral("HH:mm")));
        entry.insert(QStringLiteral("minute"), minute);
        entry.insert(QStringLiteral("state"), stateName(state.state));
        entry.insert(QStringLiteral("key"), state.state == PrayerState::Idle ? QString() : prayerKeyId(state.key));
        entry.insert(QStringLiteral("seconds"), state.seconds);
        entry.insert(QStringLiteral("target"), state.target.isValid()
                                                ? state.target.toString(QStringLiteral("HH:mm"))
                                                : QString());
        states.append(entry);
    }
    root.insert(QStringLiteral("states"), states);

    QJsonObject events;
    if (haveYear && snapshot.current) {
        events.insert(QStringLiteral("current"), QJsonObject{
            { QStringLiteral("id"), snapshot.current->event.id },
            { QStringLiteral("date"), snapshot.current->date.toString(Qt::ISODate) },
            { QStringLiteral("daysAway"), snapshot.current->daysAway }
        });
    }
    if (haveYear && snapshot.next) {
        events.insert(QStringLiteral("next"), QJsonObject{
            { QStringLiteral("id"), snapshot.next->event.id },
            { QStringLiteral("date"), snapshot.next->date.toString(Qt::ISODate) },
            { QStringLiteral("daysAway"), snapshot.next->daysAway }
        });
    }
    root.insert(QStringLiteral("events"), events);

    // The month around the chosen day, with the Hijri day and any event on it.
    QJsonArray month;
    if (haveYear) {
    const QDate first(day.date.year(), day.date.month(), 1);
    const QDate last = first.addMonths(1).addDays(-1);
    for (QDate cursor = first; cursor <= last; cursor = cursor.addDays(1)) {
        QJsonObject entry;
        entry.insert(QStringLiteral("date"), cursor.toString(Qt::ISODate));
        entry.insert(QStringLiteral("dayOfMonth"), cursor.day());
        const HijriDate hijri = hijriYear.hijriOf(cursor);
        entry.insert(QStringLiteral("hijri"), hijri.isValid()
                       ? QStringLiteral("%1/%2").arg(hijri.month).arg(hijri.day)
                       : QString());
        entry.insert(QStringLiteral("friday"), DayRules::isFriday(cursor));
        for (const IslamicEvent &event : EventCalendar::events()) {
            if (hijri.isValid() && hijri.month == event.month && hijri.day == event.day)
                entry.insert(QStringLiteral("event"), event.id);
        }
        month.append(entry);
    }
    }
    root.insert(QStringLiteral("month"), month);

    QTextStream(stdout) << QJsonDocument(root).toJson(QJsonDocument::Indented);
    return 0;
}
