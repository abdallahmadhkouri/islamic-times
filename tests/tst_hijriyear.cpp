#include "core/HijriYear.h"

#include <QtTest>

using namespace islamic;

class TestHijriYear : public QObject
{
    Q_OBJECT

private slots:
    void mapsBothWays()
    {
        HijriYear year;
        year.add(QDate(2027, 2, 8), HijriDate{1448, 9, 1});
        year.add(QDate(2027, 2, 9), HijriDate{1448, 9, 2});

        QCOMPARE(year.size(), 2);
        QCOMPARE(year.hijriOf(QDate(2027, 2, 8)), (HijriDate{1448, 9, 1}));
        QCOMPARE(year.dateOf(HijriDate{1448, 9, 2}), QDate(2027, 2, 9));
    }

    void unknownDateIsInvalidNotGuessed()
    {
        HijriYear year;
        year.add(QDate(2027, 2, 8), HijriDate{1448, 9, 1});
        QVERIFY(!year.hijriOf(QDate(2027, 2, 9)).isValid());
        QVERIFY(!year.dateOf(HijriDate{1448, 9, 2}).isValid());
        QVERIFY(!year.covers(QDate(2030, 1, 1)));
    }

    void ignoresInvalidInput()
    {
        HijriYear year;
        year.add(QDate(), HijriDate{1448, 9, 1});
        year.add(QDate(2027, 2, 8), HijriDate{});
        QVERIFY(year.isEmpty());
    }

    void reportsItsRange()
    {
        HijriYear year;
        year.add(QDate(2027, 2, 9), HijriDate{1448, 9, 2});
        year.add(QDate(2027, 2, 8), HijriDate{1448, 9, 1});
        year.add(QDate(2027, 2, 10), HijriDate{1448, 9, 3});
        QCOMPARE(year.firstDate(), QDate(2027, 2, 8));
        QCOMPARE(year.lastDate(), QDate(2027, 2, 10));
    }

    // The real fixture: 730 consecutive days captured from the provider. If the
    // mapping is ever fed by a different source, this is the test that notices.
    void realYearIsContinuousAndOrdered()
    {
        const QString path = QFINDTESTDATA("fixtures/hijri_calendar.json");
        QVERIFY2(!path.isEmpty(), "fixture missing");

        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QJsonArray days = QJsonDocument::fromJson(file.readAll())
                                    .object().value(QStringLiteral("days")).toArray();
        QCOMPARE(days.size(), 730);

        HijriYear year;
        for (const QJsonValue &value : days) {
            const QJsonArray pair = value.toArray();
            const QDate date = QDate::fromString(pair.at(0).toString(), Qt::ISODate);
            const QStringList parts = pair.at(1).toString().split(QLatin1Char('-'));
            year.add(date, HijriDate{parts.at(0).toInt(), parts.at(1).toInt(), parts.at(2).toInt()});
        }
        QCOMPARE(year.size(), 730);

        QDate date = year.firstDate();
        int checked = 0;
        while (date <= year.lastDate()) {
            const HijriDate today = year.hijriOf(date);
            QVERIFY2(today.isValid(), qPrintable(date.toString(Qt::ISODate)));
            const QDate tomorrow = date.addDays(1);
            if (tomorrow <= year.lastDate()) {
                const HijriDate next = year.hijriOf(tomorrow);
                const bool sameMonth = next.month == today.month && next.year == today.year;
                if (sameMonth)
                    QCOMPARE(next.day, today.day + 1);
                else
                    QVERIFY2(next.day == 1, qPrintable(tomorrow.toString(Qt::ISODate)));
            }
            date = tomorrow;
            ++checked;
        }
        QCOMPARE(checked, 730);
    }

    void knownAnchorsFromTheProvider()
    {
        const QString path = QFINDTESTDATA("fixtures/hijri_calendar.json");
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QJsonArray days = QJsonDocument::fromJson(file.readAll())
                                    .object().value(QStringLiteral("days")).toArray();
        HijriYear year;
        for (const QJsonValue &value : days) {
            const QJsonArray pair = value.toArray();
            const QDate date = QDate::fromString(pair.at(0).toString(), Qt::ISODate);
            const QStringList parts = pair.at(1).toString().split(QLatin1Char('-'));
            year.add(date, HijriDate{parts.at(0).toInt(), parts.at(1).toInt(), parts.at(2).toInt()});
        }

        QCOMPARE(year.dateOf(HijriDate{1448, 9, 1}), QDate(2027, 2, 8));   // 1 Ramadan
        QCOMPARE(year.dateOf(HijriDate{1448, 10, 1}), QDate(2027, 3, 9));  // Eid al-Fitr
        QCOMPARE(year.dateOf(HijriDate{1449, 1, 1}), QDate(2027, 6, 6));   // new year
    }
};

QTEST_GUILESS_MAIN(TestHijriYear)
#include "tst_hijriyear.moc"
