#include "core/Types.h"

#include <QtTest>

using namespace islamic;

class TestTypes : public QObject
{
    Q_OBJECT

private slots:
    void prayerKeyIdRoundTrips_data()
    {
        QTest::addColumn<int>("key");
        QTest::newRow("fajr") << int(PrayerKey::Fajr);
        QTest::newRow("sunrise") << int(PrayerKey::Sunrise);
        QTest::newRow("dhuhr") << int(PrayerKey::Dhuhr);
        QTest::newRow("asr") << int(PrayerKey::Asr);
        QTest::newRow("maghrib") << int(PrayerKey::Maghrib);
        QTest::newRow("isha") << int(PrayerKey::Isha);
    }

    void prayerKeyIdRoundTrips()
    {
        QFETCH(int, key);
        const PrayerKey prayer = PrayerKey(key);
        bool ok = false;
        QCOMPARE(prayerKeyFromId(prayerKeyId(prayer), &ok), prayer);
        QVERIFY(ok);
    }

    void prayerKeyFromIdAcceptsAliases()
    {
        bool ok = false;
        QCOMPARE(prayerKeyFromId(QStringLiteral("shuruq"), &ok), PrayerKey::Sunrise);
        QVERIFY(ok);
        QCOMPARE(prayerKeyFromId(QStringLiteral("zuhr"), &ok), PrayerKey::Dhuhr);
        QVERIFY(ok);
    }

    void prayerKeyFromIdRejectsUnknown()
    {
        bool ok = true;
        prayerKeyFromId(QStringLiteral("tahajjud"), &ok);
        QVERIFY(!ok);
    }

    void hijriDateValidity()
    {
        QVERIFY((HijriDate{1448, 9, 1}.isValid()));
        QVERIFY((!HijriDate{}.isValid()));
        QVERIFY((!HijriDate{1448, 13, 1}.isValid()));
        QVERIFY((!HijriDate{1448, 0, 1}.isValid()));
        QVERIFY((!HijriDate{1448, 9, 31}.isValid()));
    }

    void hijriComparisonIsChronological()
    {
        QVERIFY((hijriLess(HijriDate{1448, 8, 30}, HijriDate{1448, 9, 1})));
        QVERIFY((hijriLess(HijriDate{1448, 12, 10}, HijriDate{1449, 1, 1})));
        QVERIFY((!hijriLess(HijriDate{1449, 1, 1}, HijriDate{1448, 12, 10})));
    }

    void prayerAttributesFollowTheTimes()
    {
        Prayer p;
        p.key = PrayerKey::Fajr;
        QVERIFY(!p.hasAdhan());
        QVERIFY(!p.hasIqama());
        p.adhan = QTime(5, 30);
        p.iqama = QTime(5, 50);
        QVERIFY(p.hasAdhan());
        QVERIFY(p.hasIqama());
    }

    void prayerDayFindsItsPrayers()
    {
        PrayerDay day;
        for (PrayerKey key : allPrayerKeys()) {
            Prayer p;
            p.key = key;
            day.prayers.append(p);
        }
        QCOMPARE(day.prayers.size(), 6);
        QVERIFY(day.prayer(PrayerKey::Maghrib) != nullptr);
        QCOMPARE(day.prayer(PrayerKey::Maghrib)->key, PrayerKey::Maghrib);
        day.prayer(PrayerKey::Asr)->adhan = QTime(15, 40);
        QCOMPARE(day.prayer(PrayerKey::Asr)->adhan, QTime(15, 40));
    }
};

QTEST_GUILESS_MAIN(TestTypes)
#include "tst_types.moc"
