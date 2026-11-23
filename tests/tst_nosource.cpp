#include "core/PrayerStateMachine.h"
#include "providers/IqamahParser.h"

#include <QtTest>

// The case Saif predicted, captured from the real API: a mosque 17 km from his
// town that exists as a pin but publishes no timetable. The provider answers
// with status "none", every iqama null, and the country calculation in
// azaan.method. Nothing is missing, nothing needs repairing, and nothing may be
// invented: the library's only job is to carry that shape faithfully.

using namespace islamic;

class TestNoSource : public QObject
{
    Q_OBJECT

    PrayerDay m_day;

private slots:
    void initTestCase()
    {
        QFile file(QFINDTESTDATA("fixtures/mosque_no_source.json"));
        QVERIFY(file.open(QIODevice::ReadOnly));
        QString error;
        QVERIFY2(IqamahParser::parseMosqueDay(file.readAll(), &m_day, &error), qPrintable(error));
    }

    void theProviderSaysNoneAndWeRepeatItWithoutDressingItUp()
    {
        QCOMPARE(m_day.provenance.status, DayStatus::None);
        QVERIFY(!m_day.provenance.stale);
        QVERIFY(!m_day.provenance.asOf.isValid());      // it never published
        QCOMPARE(m_day.provenance.source, TimesSource::IqamahCalculated);
        QVERIFY(!m_day.provenance.predicted);
    }

    void everyIqamaIsAbsent()
    {
        for (const Prayer &prayer : m_day.prayers)
            QVERIFY2(!prayer.hasIqama(), qPrintable(prayerKeyId(prayer.key)));
    }

    void theMosqueAdhanIsAbsentButTheCalculatedOneIsThere()
    {
        for (const Prayer &prayer : m_day.prayers) {
            if (prayer.key == PrayerKey::Sunrise) {
                QVERIFY(prayer.hasAdhan());
                continue;
            }
            QVERIFY2(!prayer.hasMosqueAdhan(), qPrintable(prayerKeyId(prayer.key)));
            QVERIFY2(prayer.methodAdhan.isValid(), qPrintable(prayerKeyId(prayer.key)));
            QCOMPARE(prayer.adhan, prayer.methodAdhan);
        }
    }

    void theTimesAreTheTunisianMethod()
    {
        QCOMPARE(m_day.date, QDate(2026, 11, 23));
        QCOMPARE(m_day.hijri, (HijriDate{1448, 6, 13}));
        QCOMPARE(m_day.prayer(PrayerKey::Fajr)->adhan, QTime(5, 35));
        QCOMPARE(m_day.prayer(PrayerKey::Dhuhr)->adhan, QTime(12, 16));
        QCOMPARE(m_day.prayer(PrayerKey::Sunrise)->adhan, QTime(7, 3));
    }

    // Without an iqama the day still works: the machine goes straight from the
    // adhan to the prayer window, and never invents a jamaa.
    void withoutAnIqamaTheDayStillWalks()
    {
        const auto snapshot = evaluate(m_day, nullptr, QDateTime(QDate(2026, 11, 23), QTime(12, 20)), 20);
        QCOMPARE(snapshot.state, PrayerState::InPrayer);
        QCOMPARE(snapshot.key, PrayerKey::Dhuhr);
    }
};

QTEST_GUILESS_MAIN(TestNoSource)
#include "tst_nosource.moc"
