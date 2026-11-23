#include "store/PayloadCache.h"

#include <QTemporaryDir>
#include <QtTest>

using namespace islamic;

class TestPayloadCache : public QObject
{
    Q_OBJECT

    QTemporaryDir m_dir;

private slots:
    void init()
    {
        QVERIFY(m_dir.isValid());
        // fresh directory per test: the cache must not depend on leftovers
        QDir(m_dir.path()).removeRecursively();
        QDir().mkpath(m_dir.path());
    }

    void roundTripsTheBytesUnchanged()
    {
        PayloadCache cache(m_dir.path());
        const QByteArray body = R"({"days":[{"date":"2026-11-23"}],"note":"bayt \u00e9"})";

        QVERIFY(cache.write(QStringLiteral("year-36.80-10.18-2027"), body));
        QVERIFY(cache.contains(QStringLiteral("year-36.80-10.18-2027")));

        CacheEntry entry;
        QVERIFY(cache.read(QStringLiteral("year-36.80-10.18-2027"), &entry));
        // Byte-for-byte: the cache is evidence, not a copy that gets tidied up.
        QCOMPARE(entry.body, body);
        QCOMPARE(entry.httpStatus, 200);
        QVERIFY(entry.fetchedAt.isValid());
    }

    void keepsTheFetchTimeItWasGiven()
    {
        PayloadCache cache(m_dir.path());
        const QDateTime fetched(QDate(2026, 11, 23), QTime(4, 30), QTimeZone::UTC);
        QVERIFY(cache.write(QStringLiteral("near-36.80-10.18-10"), "{\"total\":0}", fetched, 200));

        CacheEntry entry;
        QVERIFY(cache.read(QStringLiteral("near-36.80-10.18-10"), &entry));
        QCOMPARE(entry.fetchedAt.toUTC(), fetched);
        QCOMPARE(entry.ageSeconds(fetched.addSecs(3600)), 3600);
    }

    void aMissingKeyIsAFailureNotAnEmptyEntry()
    {
        PayloadCache cache(m_dir.path());
        CacheEntry entry;
        QVERIFY(!cache.read(QStringLiteral("year-1.00-2.00-2027"), &entry));
        QVERIFY(!entry.isValid());
        QVERIFY(!cache.contains(QStringLiteral("year-1.00-2.00-2027")));
        QVERIFY(!cache.read(QStringLiteral("year-1.00-2.00-2027"), nullptr));
    }

    void overwritingLeavesOneValidFile()
    {
        PayloadCache cache(m_dir.path());
        QVERIFY(cache.write(QStringLiteral("mosque-abc-2026-11-23"), "{\"v\":1}"));
        QVERIFY(cache.write(QStringLiteral("mosque-abc-2026-11-23"), "{\"v\":2}"));
        CacheEntry entry;
        QVERIFY(cache.read(QStringLiteral("mosque-abc-2026-11-23"), &entry));
        QCOMPARE(entry.body, QByteArray("{\"v\":2}"));
        QCOMPARE(cache.keys().size(), 1);
    }

    void keysCannotEscapeTheRoot()
    {
        PayloadCache cache(m_dir.path());
        for (const QString &bad : { QStringLiteral("../evil"), QStringLiteral("/etc/passwd"),
                                    QStringLiteral("a/b"), QString(), QString(200, QLatin1Char('x')) }) {
            QVERIFY2(!PayloadCache::isValidKey(bad), qPrintable(bad.left(20)));
            QVERIFY(!cache.write(bad, "{}"));
            CacheEntry entry;
            QVERIFY(!cache.read(bad, &entry));
        }
        QVERIFY(!QFile::exists(m_dir.path() + QStringLiteral("/../evil.json")));
    }

    void pruningDropsOnlyWhatIsOld()
    {
        PayloadCache cache(m_dir.path());
        const QDateTime now = QDateTime::currentDateTimeUtc();
        QVERIFY(cache.write(QStringLiteral("year-old"), "{}", now.addDays(-40)));
        QVERIFY(cache.write(QStringLiteral("year-new"), "{}", now.addSecs(-60)));
        QVERIFY(cache.write(QStringLiteral("mosque-old"), "{}", now.addDays(-3)));

        QCOMPARE(cache.pruneOlderThan(now.addDays(-30)), 1);
        QVERIFY(!cache.contains(QStringLiteral("year-old")));
        QVERIFY(cache.contains(QStringLiteral("year-new")));
        QVERIFY(cache.contains(QStringLiteral("mosque-old")));

        QCOMPARE(cache.pruneOlderThan(now), 2);
        QVERIFY(cache.keys().isEmpty());
    }

    void aCorruptFileIsRemovedRatherThanServed()
    {
        PayloadCache cache(m_dir.path());
        QVERIFY(cache.write(QStringLiteral("year-broken"), "{}"));
        QFile file(m_dir.path() + QStringLiteral("/year-broken.json"));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("not json at all");
        file.close();

        const int removed = cache.pruneOlderThan(QDateTime::currentDateTimeUtc().addYears(-1));
        QCOMPARE(removed, 1);
        QVERIFY(!cache.contains(QStringLiteral("year-broken")));
    }

    void reportsItsFootprint()
    {
        PayloadCache cache(m_dir.path());
        QVERIFY(cache.write(QStringLiteral("year-a"), QByteArray(1000, 'x')));
        QVERIFY(cache.write(QStringLiteral("year-b"), QByteArray(2000, 'y')));
        QVERIFY(cache.totalSize() > 3000);      // envelope overhead on top of the bodies
        QCOMPARE(cache.keys().size(), 2);
    }

    void keysAreLocationAndDaySpecific()
    {
        // Same town at two decimal places shares an entry; two towns do not.
        QCOMPARE(PayloadCache::keyForYear(36.8001, 10.1802, 2027),
                 PayloadCache::keyForYear(36.8049, 10.1799, 2027));
        QVERIFY(PayloadCache::keyForYear(36.80, 10.18, 2027)
                != PayloadCache::keyForYear(36.90, 10.18, 2027));
        QCOMPARE(PayloadCache::keyForNearby(36.8, 10.18, 10), QStringLiteral("near-36.80-10.18-10"));
        QCOMPARE(PayloadCache::keyForMosqueDay(QStringLiteral("b22c7139-555e-4d41-81cd-186b7392ce8c"),
                                               QDate(2026, 11, 23)),
                 QStringLiteral("mosque-b22c7139-555e-4d41-81cd-186b7392ce8c-2026-11-23"));
    }
};

QTEST_GUILESS_MAIN(TestPayloadCache)
#include "tst_payloadcache.moc"
