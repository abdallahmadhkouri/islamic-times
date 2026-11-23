#include "core/PrayerStateMachine.h"

#include <QtTest>

using namespace islamic;

namespace {

Prayer make(PrayerKey key, int adhanH, int adhanM, int iqamaH = -1, int iqamaM = 0)
{
    Prayer prayer;
    prayer.key = key;
    prayer.adhan = QTime(adhanH, adhanM);
    if (iqamaH >= 0)
        prayer.iqama = QTime(iqamaH, iqamaM);
    return prayer;
}

PrayerDay sampleDay(const QDate &date = QDate(2027, 2, 8))
{
    PrayerDay day;
    day.date = date;
    day.hijri = HijriDate{1448, 9, 1};
    day.prayers = {
        make(PrayerKey::Fajr, 5, 30, 5, 50),
        make(PrayerKey::Sunrise, 7, 0),
        make(PrayerKey::Dhuhr, 12, 30, 12, 45),
        make(PrayerKey::Asr, 15, 40, 15, 55),
        make(PrayerKey::Maghrib, 18, 10, 18, 12),
        make(PrayerKey::Isha, 19, 30, 19, 45),
    };
    return day;
}

QDateTime at(int hour, int minute, int second = 0)
{
    return QDateTime(QDate(2027, 2, 8), QTime(hour, minute, second));
}

} // namespace

class TestStateMachine : public QObject
{
    Q_OBJECT

private slots:
    void beforeFajr_countsToTheAdhan()
    {
        const auto snapshot = evaluate(sampleDay(), nullptr, at(4, 0));
        QCOMPARE(snapshot.state, PrayerState::WaitingAdhan);
        QCOMPARE(snapshot.key, PrayerKey::Fajr);
        QCOMPARE(snapshot.target.time(), QTime(5, 30));
        QCOMPARE(snapshot.seconds, 5400);
    }

    void atTheAdhan_switchesToTheIqamaCountdown()
    {
        const auto snapshot = evaluate(sampleDay(), nullptr, at(5, 30));
        QCOMPARE(snapshot.state, PrayerState::WaitingIqama);
        QCOMPARE(snapshot.key, PrayerKey::Fajr);
        QCOMPARE(snapshot.target.time(), QTime(5, 50));
        QCOMPARE(snapshot.seconds, 1200);
    }

    void insideTheIqamaWindow_countsDownMinutes()
    {
        const auto snapshot = evaluate(sampleDay(), nullptr, at(5, 45));
        QCOMPARE(snapshot.state, PrayerState::WaitingIqama);
        QCOMPARE(snapshot.seconds, 300);
    }

    void atTheIqama_thePrayerHasStarted()
    {
        const auto snapshot = evaluate(sampleDay(), nullptr, at(5, 50));
        QCOMPARE(snapshot.state, PrayerState::InPrayer);
        QCOMPARE(snapshot.key, PrayerKey::Fajr);
        QCOMPARE(snapshot.seconds, 0);
    }

    void afterTheIqama_theWindowStillCountsAsPraying()
    {
        const auto snapshot = evaluate(sampleDay(), nullptr, at(6, 5));
        QCOMPARE(snapshot.state, PrayerState::InPrayer);
        QCOMPARE(snapshot.seconds, 900);
    }

    void whenTheWindowEnds_itCountsToTheNextPrayer()
    {
        const auto snapshot = evaluate(sampleDay(), nullptr, at(6, 15));
        QCOMPARE(snapshot.state, PrayerState::WaitingAdhan);
        QCOMPARE(snapshot.key, PrayerKey::Dhuhr);
        QCOMPARE(snapshot.seconds, int(QTime(6, 15).secsTo(QTime(12, 30))));
    }

    void aTwoMinuteMaghribGapIsRespected()
    {
        const auto snapshot = evaluate(sampleDay(), nullptr, at(18, 11));
        QCOMPARE(snapshot.state, PrayerState::WaitingIqama);
        QCOMPARE(snapshot.key, PrayerKey::Maghrib);
        QCOMPARE(snapshot.seconds, 60);
    }

    void afterIsha_itStops()
    {
        const auto snapshot = evaluate(sampleDay(), nullptr, at(20, 10));
        QCOMPARE(snapshot.state, PrayerState::Idle);
    }

    void afterIsha_withTomorrow_itCountsToFajr()
    {
        PrayerDay tomorrow = sampleDay(QDate(2027, 2, 9));
        const auto snapshot = evaluate(sampleDay(), &tomorrow, at(20, 10));
        QCOMPARE(snapshot.state, PrayerState::WaitingAdhan);
        QCOMPARE(snapshot.key, PrayerKey::Fajr);
        QCOMPARE(snapshot.target.date(), QDate(2027, 2, 9));
        QCOMPARE(snapshot.seconds, int(at(20, 10).secsTo(QDateTime(QDate(2027, 2, 9), QTime(5, 30)))));
    }

    void withoutAnIqama_thePrayerStartsAtTheAdhan()
    {
        PrayerDay day = sampleDay();
        day.prayer(PrayerKey::Dhuhr)->iqama = QTime();
        const auto snapshot = evaluate(day, nullptr, at(12, 35));
        QCOMPARE(snapshot.state, PrayerState::InPrayer);
        QCOMPARE(snapshot.key, PrayerKey::Dhuhr);
        QCOMPARE(snapshot.seconds, 300);
    }

    // The sweep that catches "one state never fires" — a class of bug that unit
    // assertions on single instants miss entirely.
    void theWholeDayIsCoveredAndEveryWindowHolds()
    {
        const PrayerDay day = sampleDay();
        const PrayerDay tomorrow = sampleDay(day.date.addDays(1));
        int inPrayer = 0, enteredIqama = 0, enteredPrayer = 0;
        PrayerState previous = PrayerState::Idle;
        QDateTime previousTarget;

        for (int minute = 0; minute < 24 * 60; ++minute) {
            const QDateTime now(day.date, QTime(minute / 60, minute % 60));
            const auto snapshot = evaluate(day, &tomorrow, now, 20);
            QVERIFY2(snapshot.state != PrayerState::Idle,
                     qPrintable(QStringLiteral("idle at minute %1").arg(minute)));

            if (snapshot.state == PrayerState::InPrayer)
                ++inPrayer;
            if (snapshot.state == PrayerState::WaitingIqama && previous != PrayerState::WaitingIqama)
                ++enteredIqama;
            if (snapshot.state == PrayerState::InPrayer && previous != PrayerState::InPrayer)
                ++enteredPrayer;
            previous = snapshot.state;

            // The target may only move forward: 06:15 counting to Dhuhr must not
            // fall back to a target it already passed.
            if (previousTarget.isValid() && snapshot.target.isValid())
                QVERIFY2(previousTarget <= snapshot.target,
                         qPrintable(QStringLiteral("target went backwards at %1")
                                        .arg(now.toString(Qt::ISODate))));
            if (snapshot.target.isValid())
                previousTarget = snapshot.target;
        }

        QCOMPARE(enteredIqama, 5);        // every prayer here has its own iqama
        QCOMPARE(enteredPrayer, 5);       // and each is entered exactly once
        QCOMPARE(inPrayer, 5 * 20);       // twenty minutes of window per prayer
    }

    void anEmptyDayIsIdleNotACrash()
    {
        const auto snapshot = evaluate(PrayerDay{}, nullptr, at(12, 0));
        QCOMPARE(snapshot.state, PrayerState::Idle);
    }

    void theMidnightBoundaryCountsToFajrNotToThePast()
    {
        const auto snapshot = evaluate(sampleDay(), nullptr, at(0, 0));
        QCOMPARE(snapshot.state, PrayerState::WaitingAdhan);
        QCOMPARE(snapshot.key, PrayerKey::Fajr);
        QCOMPARE(snapshot.seconds, 5 * 3600 + 30 * 60);
    }
};

QTEST_GUILESS_MAIN(TestStateMachine)
#include "tst_statemachine.moc"
