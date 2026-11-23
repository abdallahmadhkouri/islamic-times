#include "chain/IslamicTimes.h"

#include <QTemporaryDir>
#include <QtTest>

using namespace islamic;

namespace {

class FakeTransport : public HttpTransport
{
public:
    QVector<QUrl> urls;
    QVector<HttpReply> replies;

    HttpReply get(const HttpRequest &request) override
    {
        urls.append(request.url);
        if (replies.isEmpty())
            return HttpReply{ 0, {}, QStringLiteral("offline") };
        return replies.takeFirst();
    }

    void pushFile(const QString &path)
    {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly))
            return;
        HttpReply reply;
        reply.status = 200;
        reply.body = file.readAll();
        replies.append(reply);
    }

    void pushBody(const QByteArray &body)
    {
        HttpReply reply;
        reply.status = 200;
        reply.body = body;
        replies.append(reply);
    }
};

// The chain asks in this order: year, nearby, mosque day. Every test queues in it.
const QString kYear = QStringLiteral("fixtures/year_times.json");
const QString kNearbySource = QStringLiteral("fixtures/nearby_with_source.json");
const QString kNearbyNone = QStringLiteral("fixtures/nearby_sidi_ali.json");
const QString kMosqueDay = QStringLiteral("fixtures/mosque_2027-02-08.json");

} // namespace

class TestChain : public QObject
{
    Q_OBJECT

    QTemporaryDir m_dir;

private slots:
    void init()
    {
        QDir(m_dir.path()).removeRecursively();
        QDir().mkpath(m_dir.path());
    }

    void coldOnline_takesTheYearAndFindsNoMosque()
    {
        FakeTransport transport;
        transport.pushFile(QFINDTESTDATA(kYear));
        transport.pushBody("{\"total\":0,\"mosques\":[]}");

        PayloadCache cache(m_dir.path());
        IqamahClient client(&transport);
        IslamicTimes times(&client, &cache);

        const DayResult result = times.dayFor(36.80, 10.18, QDate(2027, 2, 8), true);
        QVERIFY2(result.ok(), qPrintable(result.notice));
        QCOMPARE(result.day.prayers.size(), 6);
        QVERIFY(!result.mosque.isValid());
        QVERIFY(!result.day.prayer(PrayerKey::Fajr)->hasIqama());
        QCOMPARE(result.day.hijri, (HijriDate{1448, 9, 1}));
        QCOMPARE(result.method.id, QStringLiteral("tn_ministry"));
        QVERIFY(!result.yearFromCache);
    }

    void warmOffline_usesExactlyWhatWasCached()
    {
        {
            FakeTransport transport;
            transport.pushFile(QFINDTESTDATA(kYear));
            transport.pushBody("{\"total\":0,\"mosques\":[]}");
            PayloadCache cache(m_dir.path());
            IqamahClient client(&transport);
            IslamicTimes times(&client, &cache);
            const DayResult first = times.dayFor(36.80, 10.18, QDate(2027, 2, 8), true);
            QVERIFY2(first.ok(), qPrintable(first.notice));
        }

        FakeTransport offline;                       // nothing queued at all
        PayloadCache cache(m_dir.path());
        IqamahClient client(&offline);
        IslamicTimes times(&client, &cache);
        const DayResult result = times.dayFor(36.80, 10.18, QDate(2027, 2, 8), false);

        QVERIFY2(result.ok(), qPrintable(result.notice));
        QVERIFY(result.yearFromCache);
        QCOMPARE(result.day.prayer(PrayerKey::Dhuhr)->adhan, QTime(12, 40));
        QCOMPARE(offline.urls.size(), 0);
    }

    void coldOffline_saysSoInsteadOfGuessing()
    {
        FakeTransport offline;
        PayloadCache cache(m_dir.path());
        IqamahClient client(&offline);
        IslamicTimes times(&client, &cache);

        const DayResult result = times.dayFor(36.80, 10.18, QDate(2027, 2, 8), false);
        QVERIFY(!result.ok());
        QVERIFY(!result.notice.isEmpty());
        QVERIFY(result.day.prayers.isEmpty());
    }

    void aMosqueWithASourceOverlaysItsIqama()
    {
        FakeTransport transport;
        transport.pushFile(QFINDTESTDATA(kYear));
        transport.pushFile(QFINDTESTDATA(kNearbySource));
        transport.pushFile(QFINDTESTDATA(kMosqueDay));

        PayloadCache cache(m_dir.path());
        IqamahClient client(&transport);
        IslamicTimes times(&client, &cache);

        const DayResult result = times.dayFor(48.84, 2.35, QDate(2027, 2, 8), true);
        QVERIFY2(result.ok(), qPrintable(result.notice));
        QVERIFY(result.mosque.isValid());
        QCOMPARE(result.mosque.name, QStringLiteral("Grande Mosquee de Paris"));
        QCOMPARE(result.day.prayer(PrayerKey::Fajr)->mosqueAdhan, QTime(6, 23));
        QCOMPARE(result.day.prayer(PrayerKey::Fajr)->iqama, QTime(6, 31));
        // the calculated time stays the year payload's, untouched
        QCOMPARE(result.day.prayer(PrayerKey::Fajr)->methodAdhan, QTime(5, 48));
        QCOMPARE(result.day.provenance.source, TimesSource::IqamahMosque);
        QCOMPARE(result.day.provenance.status, DayStatus::Fresh);
    }

    void aMosqueWithoutASourceIsNotAdopted()
    {
        FakeTransport transport;
        transport.pushFile(QFINDTESTDATA(kYear));
        transport.pushFile(QFINDTESTDATA(kNearbyNone));

        PayloadCache cache(m_dir.path());
        IqamahClient client(&transport);
        IslamicTimes times(&client, &cache);

        const DayResult result = times.dayFor(34.55, 9.48, QDate(2027, 2, 8), true);
        QVERIFY2(result.ok(), qPrintable(result.notice));
        QVERIFY(!result.mosque.isValid());
        QVERIFY(!result.day.prayer(PrayerKey::Fajr)->hasIqama());
        QCOMPARE(transport.urls.size(), 2);          // year + nearby, no mosque fetch
    }

    void aCachedYearIsNotRefetchedEveryLaunch()
    {
        {
            FakeTransport first;
            first.pushFile(QFINDTESTDATA(kYear));
            first.pushBody("{\"total\":0,\"mosques\":[]}");
            PayloadCache cache(m_dir.path());
            IqamahClient client(&first);
            IslamicTimes times(&client, &cache);
            const DayResult result = times.dayFor(36.80, 10.18, QDate(2027, 2, 8), true);
            QVERIFY2(result.ok(), qPrintable(result.notice));
        }

        FakeTransport second;                        // nothing queued: any fetch fails
        PayloadCache cache(m_dir.path());
        IqamahClient client(&second);
        IslamicTimes times(&client, &cache);
        const DayResult result = times.dayFor(36.80, 10.18, QDate(2027, 2, 8), true);

        QVERIFY2(result.ok(), qPrintable(result.notice));
        QVERIFY(result.yearFromCache);
        QCOMPARE(second.urls.size(), 0);             // the cache answered, online or not
    }

    void aDayOutsideThePayloadIsReportedNotInvented()
    {
        FakeTransport transport;
        transport.pushFile(QFINDTESTDATA(kYear));
        transport.pushBody("{\"total\":0,\"mosques\":[]}");
        PayloadCache cache(m_dir.path());
        IqamahClient client(&transport);
        IslamicTimes times(&client, &cache);

        const DayResult result = times.dayFor(36.80, 10.18, QDate(2031, 5, 5), true);
        QVERIFY(!result.ok());
        QVERIFY(result.notice.contains(QLatin1String("does not cover")));
    }
};

QTEST_GUILESS_MAIN(TestChain)
#include "tst_chain.moc"
