// it-inspector — the "show me" tool for the widgets. Renders every widget at its
// grid size from one command, with the clock pinned to any minute of a real day,
// so design is reviewed in pictures instead of adjectives.
//
//   it-inspector --year fixtures/year_times.json --date 2027-02-08 \
//                --instant 05:52 --out /tmp/shots [--mosque fixtures/...json] [--all]

#include "core/DayRules.h"
#include "core/EventCalendar.h"
#include "core/HijriYear.h"
#include "core/PrayerStateMachine.h"
#include "providers/IqamahClient.h"
#include "providers/IqamahParser.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QEventLoop>
#include <QQuickItem>
#include <QThread>
#include <QQuickView>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QVariantMap>
#include <QtQml>

using namespace islamic;

namespace {

QByteArray readAll(const QString &path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

QString clock(const QTime &time)
{
    return time.isValid() ? time.toString(QStringLiteral("HH:mm")) : QString();
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

// What a row of the board says about itself at this minute.
QString rowState(const Prayer &prayer, const QDateTime &now, int windowMinutes)
{
    const QDateTime adhan(QDate(now.date()), prayer.adhan);
    const QDateTime iqama(QDate(now.date()), prayer.iqama);
    if (!adhan.isValid())
        return QStringLiteral("absent");
    if (now < adhan)
        return QStringLiteral("later");
    if (prayer.hasIqama() && iqama.isValid() && now < iqama)
        return QStringLiteral("waitingIqama");
    const QDateTime start = prayer.hasIqama() && iqama.isValid() ? iqama : adhan;
    if (start.secsTo(now) <= windowMinutes * 60)
        return QStringLiteral("inPrayer");
    return QStringLiteral("passed");
}

QVariantMap buildState(const PrayerDay &day, const MethodPreset &method, const MosqueInfo &mosque,
                       const HijriYear &year, const QDateTime &now, const QString &instant)
{
    QVariantMap state;
    state.insert(QStringLiteral("now"), instant);

    QVariantMap hijri;
    hijri.insert(QStringLiteral("day"), day.hijri.day);
    hijri.insert(QStringLiteral("month"), day.hijri.month);
    hijri.insert(QStringLiteral("year"), day.hijri.year);
    state.insert(QStringLiteral("hijri"), hijri);
    state.insert(QStringLiteral("isFriday"), DayRules::isFriday(day.date));

    QVariantMap mosqueMap;
    mosqueMap.insert(QStringLiteral("name"), mosque.isValid() ? mosque.name : QString());
    mosqueMap.insert(QStringLiteral("distanceKm"), mosque.distanceKm);
    mosqueMap.insert(QStringLiteral("has"), mosque.isValid());
    state.insert(QStringLiteral("mosque"), mosqueMap);

    QVariantList prayers;
    for (const Prayer &prayer : day.prayers) {
        QVariantMap row;
        row.insert(QStringLiteral("key"), prayerKeyId(prayer.key));
        row.insert(QStringLiteral("adhan"), clock(prayer.adhan));
        row.insert(QStringLiteral("iqama"), clock(prayer.iqama));
        row.insert(QStringLiteral("state"), rowState(prayer, now, 20));
        prayers.append(row);
    }
    state.insert(QStringLiteral("prayers"), prayers);

    PrayerDay tomorrow = day;
    tomorrow.date = day.date.addDays(1);
    const PrayerSnapshot snapshot = evaluate(day, &tomorrow, now, 20);
    QVariantMap next;
    next.insert(QStringLiteral("key"), snapshot.state == PrayerState::Idle ? QString() : prayerKeyId(snapshot.key));
    next.insert(QStringLiteral("at"), snapshot.target.isValid() ? snapshot.target.time().toString(QStringLiteral("HH:mm")) : QString());
    next.insert(QStringLiteral("seconds"), snapshot.seconds);
    next.insert(QStringLiteral("state"), stateName(snapshot.state));
    state.insert(QStringLiteral("next"), next);

    const CalendarSnapshot calendar = EventCalendar::around(day.hijri, day.date, year);
    QVariantMap event;
    event.insert(QStringLiteral("currentId"), calendar.current ? calendar.current->event.id : QString());
    event.insert(QStringLiteral("daysAgo"), calendar.current ? calendar.current->daysAway : -1);
    event.insert(QStringLiteral("nextId"), calendar.next ? calendar.next->event.id : QString());
    event.insert(QStringLiteral("daysAway"), calendar.next ? calendar.next->daysAway : -1);
    state.insert(QStringLiteral("event"), event);

    // Ramadan is a span, so the tile needs the two ends and where we are between them.
    if (day.hijri.month == 9) {
        const QTime imsak = day.prayer(PrayerKey::Fajr)->adhan;
        const QTime iftar = day.prayer(PrayerKey::Maghrib)->adhan;
        QVariantMap ramadan;
        ramadan.insert(QStringLiteral("day"), day.hijri.day);
        ramadan.insert(QStringLiteral("imsak"), clock(imsak));
        ramadan.insert(QStringLiteral("iftar"), clock(iftar));
        double progress = 0.0;
        if (imsak.isValid() && iftar.isValid()) {
            const double total = imsak.secsTo(iftar);
            if (total > 0)
                progress = qBound(0.0, QTime(0, 0).secsTo(now.time()) > 0 ? imsak.secsTo(now.time()) / total : 0.0, 1.0);
        }
        ramadan.insert(QStringLiteral("progress"), progress);
        ramadan.insert(QStringLiteral("toIftar"), iftar.isValid() ? now.secsTo(QDateTime(day.date, iftar)) : -1);
        ramadan.insert(QStringLiteral("toImsak"), imsak.isValid()
                           ? now.secsTo(QDateTime(day.date.addDays(1), imsak)) : -1);
        state.insert(QStringLiteral("ramadan"), ramadan);
    } else {
        state.insert(QStringLiteral("ramadan"), QVariant());
    }

    QVariantMap jumuah;
    const bool friday = DayRules::isFriday(day.date);
    jumuah.insert(QStringLiteral("isToday"), friday);
    jumuah.insert(QStringLiteral("daysAway"), (5 - day.date.dayOfWeek() + 7) % 7);
    if (friday) {
        QTime khutbah;
        if (!day.jumuah.isEmpty())
            khutbah = day.jumuah.first().khutbah;
        if (!khutbah.isValid())
            khutbah = day.prayer(PrayerKey::Dhuhr)->adhan;
        jumuah.insert(QStringLiteral("khutbah"), clock(khutbah));
        jumuah.insert(QStringLiteral("seconds"), khutbah.isValid() ? now.secsTo(QDateTime(day.date, khutbah)) : -1);
    }
    state.insert(QStringLiteral("jumuah"), jumuah);

    state.insert(QStringLiteral("sunrise"), clock(day.prayer(PrayerKey::Sunrise)->adhan));
    state.insert(QStringLiteral("method"), method.id);
    return state;
}

struct Shot { QString widget; int width; int height; };

QVector<Shot> shots()
{
    return {
        { QStringLiteral("NextPrayer"), 360, 360 },
        { QStringLiteral("PrayerBoard"), 720, 720 },
        { QStringLiteral("NextPrayerEvent"), 720, 360 },
        { QStringLiteral("Ramadan"), 720, 360 },
        { QStringLiteral("HijriEvent"), 360, 360 },
        { QStringLiteral("Jumua"), 720, 360 },
    };
}

} // namespace

int main(int argc, char **argv)
{
    // Headless and GPU-less: the software scene graph renders the same pixels the
    // TV would, without needing a GL context on a build machine.
    QQuickWindow::setGraphicsApi(QSGRendererInterface::Software);
    QGuiApplication app(argc, argv);

    const QDir fonts(QStringLiteral(ISLAMIC_TIMES_SOURCE_DIR "/qml/fonts"));
    for (const QString &file : fonts.entryList({ QStringLiteral("*.ttf") }, QDir::Files))
        QFontDatabase::addApplicationFont(fonts.filePath(file));

    QCommandLineParser parser;
    QCommandLineOption yearOption(QStringLiteral("year"), QStringLiteral("year payload"), QStringLiteral("path"));
    QCommandLineOption mosqueOption(QStringLiteral("mosque"), QStringLiteral("mosque day payload"), QStringLiteral("path"));
    QCommandLineOption dateOption(QStringLiteral("date"), QStringLiteral("YYYY-MM-DD"), QStringLiteral("date"), QStringLiteral("2027-02-08"));
    QCommandLineOption instantOption(QStringLiteral("instant"), QStringLiteral("HH:mm"), QStringLiteral("time"), QStringLiteral("05:52"));
    QCommandLineOption outOption(QStringLiteral("out"), QStringLiteral("output directory"), QStringLiteral("dir"), QStringLiteral("."));
    QCommandLineOption widgetOption(QStringLiteral("widget"), QStringLiteral("one widget name"), QStringLiteral("name"));
    QCommandLineOption scaleOption(QStringLiteral("scale"), QStringLiteral("render scale"), QStringLiteral("factor"), QStringLiteral("2"));
    parser.addOptions({ yearOption, mosqueOption, dateOption, instantOption, outOption, widgetOption, scaleOption });
    parser.process(app);

    QVector<PrayerDay> days;
    MethodPreset method;
    if (!IqamahParser::parseYearTimes(readAll(parser.value(yearOption)), &days, &method, nullptr) || days.isEmpty()) {
        qCritical("cannot read the year payload");
        return 1;
    }

    const QDate date = QDate::fromString(parser.value(dateOption), Qt::ISODate);
    const PrayerDay *picked = nullptr;
    for (const PrayerDay &day : days) {
        if (day.date == date) {
            picked = &day;
            break;
        }
    }
    if (!picked) {
        qCritical("date not in the payload");
        return 1;
    }

    PrayerDay day = *picked;
    MosqueInfo mosque;
    if (parser.isSet(mosqueOption)) {
        PrayerDay mosqueDay;
        // The demo may render any day of the year, so the mosque fixture is allowed
        // to be re-dated: the jamaa times are the same shape, which is what a
        // visual pass needs. ARCHITECTURE.md calls this out.
        if (IqamahParser::parseMosqueDay(readAll(parser.value(mosqueOption)), &mosqueDay, nullptr)) {
            for (const Prayer &from : mosqueDay.prayers) {
                Prayer *to = day.prayer(from.key);
                if (!to)
                    continue;
                if (from.hasMosqueAdhan())
                    to->mosqueAdhan = from.mosqueAdhan;
                if (from.adhan.isValid())
                    to->adhan = from.adhan;
                if (from.hasIqama())
                    to->iqama = from.iqama;
            }
            day.jumuah = mosqueDay.jumuah;
            mosque.id = QStringLiteral("fixture");
            mosque.name = QStringLiteral("جامع الرحمة 2 · سيدي علي بن عون");
            mosque.hasSource = true;
            mosque.distanceKm = 1.2;
        }
    }

    HijriYear hijriYear;
    for (const PrayerDay &entry : days)
        hijriYear.add(entry.date, entry.hijri);

    const QTime instant = QTime::fromString(parser.value(instantOption), QStringLiteral("HH:mm"));
    const QDateTime now(date, instant.isValid() ? instant : QTime(5, 52));
    const QVariantMap state = buildState(day, method, mosque, hijriYear, now, now.time().toString(QStringLiteral("HH:mm")));

    QDir().mkpath(parser.value(outOption));
    const double scale = qMax(1.0, parser.value(scaleOption).toDouble());
    const QString qmlDir = QStringLiteral(ISLAMIC_TIMES_SOURCE_DIR "/qml/widgets/");

    const QVector<Shot> all = shots();
    const QString only = parser.value(widgetOption);
    int rendered = 0;

    for (const Shot &shot : all) {
        if (!only.isEmpty() && shot.widget != only)
            continue;

        QQuickView view;
        view.setResizeMode(QQuickView::SizeRootObjectToView);
        view.rootContext()->setContextProperty(QStringLiteral("dayData"), state);
        view.setSource(QUrl::fromLocalFile(qmlDir + shot.widget + QStringLiteral(".qml")));
        if (view.status() != QQuickView::Ready) {
            for (const QQmlError &error : view.errors())
                qWarning().noquote() << error.toString();
            continue;
        }
        view.resize(qRound(shot.width * scale), qRound(shot.height * scale));
        view.show();
        // let the software scene graph paint at least one frame before grabbing
        for (int i = 0; i < 12; ++i) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 40);
            QThread::msleep(16);
        }

        const QImage image = view.grabWindow();
        const QString path = QStringLiteral("%1/%2-%3.png").arg(parser.value(outOption), shot.widget).arg(shot.height);
        if (!image.isNull() && image.save(path))
            ++rendered;
        else
            qWarning() << "could not render" << shot.widget;
    }

    qInfo("%d widget(s) rendered", rendered);
    return rendered > 0 ? 0 : 1;
}
