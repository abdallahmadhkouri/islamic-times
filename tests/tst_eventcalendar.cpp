#include "core/EventCalendar.h"
#include "core/HijriYear.h"

#include <QtTest>

using namespace islamic;

namespace {

// The real provider data, loaded once: 2026 and 2027, 730 days.
HijriYear loadYear(const QString &path)
{
    HijriYear year;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return year;
    const QJsonArray days = QJsonDocument::fromJson(file.readAll())
                                .object().value(QStringLiteral("days")).toArray();
    for (const QJsonValue &value : days) {
        const QJsonArray pair = value.toArray();
        const QDate date = QDate::fromString(pair.at(0).toString(), Qt::ISODate);
        const QStringList parts = pair.at(1).toString().split(QLatin1Char('-'));
        year.add(date, HijriDate{parts.at(0).toInt(), parts.at(1).toInt(), parts.at(2).toInt()});
    }
    return year;
}

} // namespace

class TestEventCalendar : public QObject
{
    Q_OBJECT

    HijriYear m_year;

private slots:
    void initTestCase()
    {
        m_year = loadYear(QFINDTESTDATA("fixtures/hijri_calendar.json"));
        QVERIFY(!m_year.isEmpty());
    }

    void theEventTableIsOrderedAndComplete()
    {
        const QVector<IslamicEvent> events = EventCalendar::events();
        QCOMPARE(events.size(), 9);
        QCOMPARE(events.first().id, QStringLiteral("newyear"));
        QCOMPARE(events.last().id, QStringLiteral("adha"));
        for (const IslamicEvent &event : events)
            QVERIFY(!event.minor);
    }

    void minorEventsAreOptIn()
    {
        const QVector<IslamicEvent> withMinor = EventCalendar::events(true);
        QCOMPARE(withMinor.size(), 11);
        int minor = 0;
        for (const IslamicEvent &event : withMinor)
            minor += event.minor ? 1 : 0;
        QCOMPARE(minor, 2);
    }

    // ---- the sequence Saif asked for, on real dates ----

    void theDayBeforeRamadan_countsToRamadan()
    {
        const QDate today(2027, 2, 7);           // 29 Sha'ban 1448
        const auto snapshot = EventCalendar::around(m_year.hijriOf(today), today, m_year);
        QVERIFY(snapshot.next.has_value());
        QCOMPARE(snapshot.next->event.id, QStringLiteral("ramadan"));
        QCOMPARE(snapshot.next->date, QDate(2027, 2, 8));
        QCOMPARE(snapshot.next->daysAway, 1);
    }

    void firstOfRamadan_theLastTenNightsIsNext()
    {
        const QDate today(2027, 2, 8);           // 1 Ramadan 1448
        const auto snapshot = EventCalendar::around(m_year.hijriOf(today), today, m_year);
        QVERIFY(snapshot.current.has_value());
        QCOMPARE(snapshot.current->event.id, QStringLiteral("ramadan"));
        QCOMPARE(snapshot.current->daysAway, 0);
        QVERIFY(snapshot.next.has_value());
        QCOMPARE(snapshot.next->event.id, QStringLiteral("lastten"));
        QCOMPARE(snapshot.next->date, QDate(2027, 2, 28));   // 21 Ramadan
    }

    void insideTheLastTen_the27thIsNext()
    {
        const QDate today(2027, 2, 28);
        const auto snapshot = EventCalendar::around(m_year.hijriOf(today), today, m_year);
        QVERIFY(snapshot.current.has_value());
        QCOMPARE(snapshot.current->event.id, QStringLiteral("lastten"));
        QVERIFY(snapshot.next.has_value());
        QCOMPARE(snapshot.next->event.id, QStringLiteral("qadr"));
        QCOMPARE(snapshot.next->daysAway, 6);
    }

    void onThe27th_qadrIsCurrentAndEidIsNext()
    {
        const QDate today(2027, 3, 6);
        const auto snapshot = EventCalendar::around(m_year.hijriOf(today), today, m_year);
        QVERIFY(snapshot.current.has_value());
        QCOMPARE(snapshot.current->event.id, QStringLiteral("qadr"));
        QVERIFY(snapshot.next.has_value());
        QCOMPARE(snapshot.next->event.id, QStringLiteral("fitr"));
        QCOMPARE(snapshot.next->date, QDate(2027, 3, 9));
        QCOMPARE(snapshot.next->daysAway, 3);
    }

    void eidAlAdhaAndTheNewYear()
    {
        const QDate adhaDay(2027, 5, 16);
        auto snapshot = EventCalendar::around(m_year.hijriOf(adhaDay), adhaDay, m_year);
        QVERIFY(snapshot.current.has_value());
        QCOMPARE(snapshot.current->event.id, QStringLiteral("adha"));

        const QDate newYear(2027, 6, 6);         // 1 Muharram 1449
        snapshot = EventCalendar::around(m_year.hijriOf(newYear), newYear, m_year);
        QVERIFY(snapshot.current.has_value());
        QCOMPARE(snapshot.current->event.id, QStringLiteral("newyear"));
        QVERIFY(snapshot.next.has_value());
        QCOMPARE(snapshot.next->event.id, QStringLiteral("ashura"));
        QCOMPARE(snapshot.next->date, QDate(2027, 6, 15));
    }

    void minorNightsAreIgnoredUnlessAskedFor()
    {
        const QDate today(2027, 1, 15);          // Sha'ban 1448, before mid-Sha'ban
        auto snapshot = EventCalendar::around(m_year.hijriOf(today), today, m_year, false);
        QVERIFY(snapshot.next.has_value());
        QCOMPARE(snapshot.next->event.id, QStringLiteral("ramadan"));

        snapshot = EventCalendar::around(m_year.hijriOf(today), today, m_year, true);
        QVERIFY(snapshot.next.has_value());
        QCOMPARE(snapshot.next->event.id, QStringLiteral("midshaban"));
    }

    void withoutDataNothingIsInvented()
    {
        HijriYear empty;
        const auto snapshot = EventCalendar::around(HijriDate{1448, 9, 1}, QDate(2027, 2, 8), empty);
        QVERIFY(!snapshot.current.has_value());
        QVERIFY(!snapshot.next.has_value());
    }

    void invalidInputIsSafe()
    {
        const auto snapshot = EventCalendar::around(HijriDate{}, QDate(), m_year);
        QVERIFY(!snapshot.current.has_value());
        QVERIFY(!snapshot.next.has_value());
    }
};

QTEST_GUILESS_MAIN(TestEventCalendar)
#include "tst_eventcalendar.moc"
