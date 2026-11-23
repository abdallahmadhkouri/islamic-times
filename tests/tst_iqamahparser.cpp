#include "providers/IqamahParser.h"
#include "core/DayRules.h"

#include <QtTest>

using namespace islamic;

namespace {

QByteArray readFixture(const char *name)
{
    const QString path = QFINDTESTDATA(name);
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return file.readAll();
}

} // namespace

class TestIqamahParser : public QObject
{
    Q_OBJECT

private slots:
    // ---- the mosque day: the real payload captured on 2026-11-23 ----

    void mosqueDayParsesEveryField()
    {
        const QByteArray payload = readFixture("fixtures/mosque_day.json");
        QVERIFY(!payload.isEmpty());

        PrayerDay day;
        QString error;
        QVERIFY2(IqamahParser::parseMosqueDay(payload, &day, &error), qPrintable(error));
        QCOMPARE(day.date, QDate(2026, 11, 23));
        QCOMPARE(day.hijri, (HijriDate{1448, 6, 13}));
        QCOMPARE(day.prayers.size(), 6);

        const Prayer *fajr = day.prayer(PrayerKey::Fajr);
        QVERIFY(fajr);
        QCOMPARE(fajr->adhan, QTime(6, 34));    // azaan as the mosque calls it
        QCOMPARE(fajr->iqama, QTime(6, 42));    // +8

        const Prayer *maghrib = day.prayer(PrayerKey::Maghrib);
        QVERIFY(maghrib);
        QCOMPARE(maghrib->iqama, maghrib->adhan);   // maghrib iqama is immediate

        const Prayer *sunrise = day.prayer(PrayerKey::Sunrise);
        QVERIFY(sunrise);
        QCOMPARE(sunrise->adhan, QTime(8, 11));
        QVERIFY(!sunrise->hasIqama());
    }

    void mosqueDayCarriesItsProvenance()
    {
        const QByteArray payload = readFixture("fixtures/mosque_day.json");
        PrayerDay day;
        QVERIFY(IqamahParser::parseMosqueDay(payload, &day, nullptr));
        QCOMPARE(day.provenance.source, TimesSource::IqamahCalculated);   // "source":{"kind":"calculated"}
        QVERIFY(!day.provenance.stale);                                   // this capture says "fresh"
        QCOMPARE(day.provenance.asOf.date(), QDate(2026, 11, 23));        // from "extracted_at"
        QCOMPARE(day.provenance.publishedFor, QDate(2026, 11, 23));
        QVERIFY(!day.provenance.predicted);
    }

    void jumuahServicesAreKept()
    {
        const QByteArray payload = readFixture("fixtures/mosque_day.json");
        PrayerDay day;
        QVERIFY(IqamahParser::parseMosqueDay(payload, &day, nullptr));
        // This mosque publishes two services, and no khutbah time: the service
        // time in "prayer" is what we fall back to.
        QCOMPARE(day.jumuah.size(), 2);
        QCOMPARE(day.jumuah.at(0).location, QStringLiteral("Main Hall"));
        QCOMPARE(day.jumuah.at(0).khutbah, QTime(12, 50));
        QCOMPARE(day.jumuah.at(1).khutbah, QTime(13, 30));
    }

    void aFridayUsesTheKhutbahForDhuhr()
    {
        const QByteArray friday = R"({
            "status": "ok",
            "as_of": "2026-11-27",
            "source": { "kind": "mosque" },
            "days": [{
                "date": "2026-11-27",
                "hijri": "1448-06-17",
                "prayers": {
                    "fajr":  { "iqamah": { "time": "06:42" }, "azaan": { "mosque": { "time": "06:34" }, "method": "06:19" } },
                    "dhuhr": { "iqamah": { "time": "13:30" }, "azaan": { "mosque": { "time": "13:25" }, "method": "12:38" } }
                },
                "sunrise": { "mosque": { "time": "08:11" } },
                "jumuah": [{ "location": "Main Hall", "azaan": { "time": "13:20" }, "khutbah": { "time": "13:00" } }]
            }]
        })";
        PrayerDay day;
        QString error;
        QVERIFY2(IqamahParser::parseMosqueDay(friday, &day, &error), qPrintable(error));
        QVERIFY(DayRules::isFriday(day.date));
        QCOMPARE(day.prayer(PrayerKey::Dhuhr)->adhan, QTime(13, 0));   // khutbah
        QCOMPARE(day.prayer(PrayerKey::Dhuhr)->iqama, QTime(13, 20));  // jumuah iqama
    }

    // A real day from a real mosque in Saif's town, as its own widget prints it.
    // The point of this fixture is that the gaps are not uniform: fajr waits 30
    // minutes, everything else 10. Any code that assumes one global iqama offset
    // fails here.
    void sidiAliOffsetsAreNotUniform()
    {
        const QByteArray payload = readFixture("fixtures/mosque_sidi_ali_2026-11-23.json");
        QVERIFY(!payload.isEmpty());
        PrayerDay day;
        QString error;
        QVERIFY2(IqamahParser::parseMosqueDay(payload, &day, &error), qPrintable(error));
        QCOMPARE(day.date, QDate(2026, 11, 23));
        QCOMPARE(day.hijri, (HijriDate{1448, 6, 13}));

        QCOMPARE(day.prayer(PrayerKey::Fajr)->adhan.secsTo(day.prayer(PrayerKey::Fajr)->iqama) / 60, 30);
        QCOMPARE(day.prayer(PrayerKey::Dhuhr)->adhan.secsTo(day.prayer(PrayerKey::Dhuhr)->iqama) / 60, 10);
        QCOMPARE(day.prayer(PrayerKey::Asr)->adhan.secsTo(day.prayer(PrayerKey::Asr)->iqama) / 60, 10);
        QCOMPARE(day.prayer(PrayerKey::Maghrib)->adhan.secsTo(day.prayer(PrayerKey::Maghrib)->iqama) / 60, 10);
        QCOMPARE(day.prayer(PrayerKey::Isha)->adhan.secsTo(day.prayer(PrayerKey::Isha)->iqama) / 60, 10);
        QCOMPARE(day.prayer(PrayerKey::Sunrise)->adhan, QTime(7, 2));
        QVERIFY(!day.prayer(PrayerKey::Sunrise)->hasIqama());
    }

    void aStaleDayIsFlaggedAsSuch()
    {
        // The live payload switches between "fresh" and "stale" depending on the
        // mosque, so both paths are pinned here rather than trusting a capture.
        const QByteArray stale = R"({
            "status": "stale",
            "stale": true,
            "as_of": "2026-11-17",
            "source": { "kind": "mosque" },
            "days": [{ "date": "2026-11-23", "hijri": "1448-06-13",
                       "prayers": { "fajr": { "iqamah": { "time": "06:42" },
                                              "azaan": { "mosque": { "time": "06:34" } } } } }]
        })";
        PrayerDay day;
        QString error;
        QVERIFY2(IqamahParser::parseMosqueDay(stale, &day, &error), qPrintable(error));
        QVERIFY(day.provenance.stale);
        QCOMPARE(day.provenance.source, TimesSource::IqamahMosque);
        QCOMPARE(day.provenance.asOf.date(), QDate(2026, 11, 17));
    }

    void brokenPayloadsFailLoudly()
    {
        PrayerDay day;
        QString error;
        QVERIFY(!IqamahParser::parseMosqueDay("{not json", &day, &error));
        QVERIFY(!error.isEmpty());

        error.clear();
        QVERIFY(!IqamahParser::parseMosqueDay("{\"days\":[]}", &day, &error));
        QVERIFY(!error.isEmpty());

        QVERIFY(!IqamahParser::parseMosqueDay("{}", nullptr, nullptr));
    }

    // ---- the year: 365 calculated days with their Hijri dates ----

    void yearTimesParseInto365Days()
    {
        const QByteArray payload = readFixture("fixtures/year_times.json");
        QVERIFY(!payload.isEmpty());

        QVector<PrayerDay> days;
        MethodPreset method;
        QString error;
        QVERIFY2(IqamahParser::parseYearTimes(payload, &days, &method, &error), qPrintable(error));
        QCOMPARE(days.size(), 365);
        QCOMPARE(days.first().date, QDate(2027, 1, 1));
        QCOMPARE(days.first().hijri, (HijriDate{1448, 7, 23}));
        QCOMPARE(days.last().date, QDate(2027, 12, 31));
        QCOMPARE(method.id, QStringLiteral("tn_ministry"));
        QCOMPARE(method.name, QStringLiteral("Ministry of Religious Affairs, Tunisia"));
        QCOMPARE(method.fajrAngle, 18.0);
        QCOMPARE(method.ishaAngle, 18.0);
        QCOMPARE(method.asr, QStringLiteral("standard"));
        QCOMPARE(method.highLat, QStringLiteral("middle_of_the_night"));
    }

    void everyYearDayIsComplete()
    {
        const QByteArray payload = readFixture("fixtures/year_times.json");
        QVector<PrayerDay> days;
        QVERIFY(IqamahParser::parseYearTimes(payload, &days, nullptr, nullptr));
        for (int i = 0; i < days.size(); ++i) {
            const PrayerDay &day = days.at(i);
            QVERIFY2(day.date.isValid(), qPrintable(QString::number(i)));
            QVERIFY2(day.hijri.isValid(), qPrintable(QString::number(i)));
            QCOMPARE(day.prayers.size(), 6);
            QVERIFY(day.prayer(PrayerKey::Fajr)->hasAdhan());
            QVERIFY(day.prayer(PrayerKey::Isha)->hasAdhan());
            // calculated days carry no iqama — that is the mosque's business
            QVERIFY(!day.prayer(PrayerKey::Dhuhr)->hasIqama());
        }
    }

    void aYearIsContinuousDayByDay()
    {
        const QByteArray payload = readFixture("fixtures/year_times.json");
        QVector<PrayerDay> days;
        QVERIFY(IqamahParser::parseYearTimes(payload, &days, nullptr, nullptr));
        for (int i = 1; i < days.size(); ++i)
            QCOMPARE(days.at(i - 1).date.addDays(1), days.at(i).date);
    }
};

QTEST_GUILESS_MAIN(TestIqamahParser)
#include "tst_iqamahparser.moc"
