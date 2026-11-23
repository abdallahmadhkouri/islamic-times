#include "providers/IqamahClient.h"

#include <QtTest>

using namespace islamic;

namespace {

// Records what the client asked for and replays what the test decides.
class FakeTransport : public HttpTransport
{
public:
    QVector<HttpRequest> requests;
    QVector<HttpReply> replies;

    HttpReply get(const HttpRequest &request) override
    {
        requests.append(request);
        if (replies.isEmpty())
            return HttpReply{ 0, {}, QStringLiteral("no reply queued") };
        return replies.takeFirst();
    }

    void push(int status, const QByteArray &body = QByteArray(), const QString &error = QString())
    {
        HttpReply reply;
        reply.status = status;
        reply.body = body;
        reply.error = error;
        replies.append(reply);
    }
};



} // namespace

class TestIqamahClient : public QObject
{
    Q_OBJECT

    QByteArray m_nearby;

private slots:
    void initTestCase()
    {
        QFile file(QFINDTESTDATA("fixtures/nearby_sidi_ali.json"));
        QVERIFY2(file.open(QIODevice::ReadOnly), "nearby fixture missing");
        m_nearby = file.readAll();
    }

    void yearUrlIsExact()
    {
        QCOMPARE(IqamahClient::yearUrl(QStringLiteral("https://api.iqamah.net/v1"), 36.8001, 10.1799, 2027),
                 QUrl(QStringLiteral("https://api.iqamah.net/v1/times/36.80,10.18/2027.json")));
    }

    void nearbyUrlOmitsTheDefaultRadius()
    {
        // Sending the default is answered with a 301 to the bare URL, so the
        // canonical form carries no query at all.
        QCOMPARE(IqamahClient::nearbyUrl(QStringLiteral("https://api.iqamah.net/v1"), 34.55, 9.48, 10),
                 QUrl(QStringLiteral("https://api.iqamah.net/v1/mosques/near/34.55,9.48.json")));
    }

    void nearbyUrlCarriesAnyOtherRadius()
    {
        QCOMPARE(IqamahClient::nearbyUrl(QStringLiteral("https://api.iqamah.net/v1"), 34.55, 9.48, 25),
                 QUrl(QStringLiteral("https://api.iqamah.net/v1/mosques/near/34.55,9.48.json?radius_km=25")));
        QCOMPARE(IqamahClient::nearbyUrl(QStringLiteral("https://api.iqamah.net/v1"), 34.55, 9.48, 2),
                 QUrl(QStringLiteral("https://api.iqamah.net/v1/mosques/near/34.55,9.48.json?radius_km=2")));
    }

    void mosqueUrlIsTheIdAndNothingElse()
    {
        QCOMPARE(IqamahClient::mosqueUrl(QStringLiteral("https://api.iqamah.net/v1"),
                                         QStringLiteral("e9a84733-0f3a-49dc-a5a5-aa94b439f612")),
                 QUrl(QStringLiteral("https://api.iqamah.net/v1/mosque/e9a84733-0f3a-49dc-a5a5-aa94b439f612.json")));
    }

    void onlyTheFiveRadiiTheApiAccepts()
    {
        for (int radius : { 2, 5, 10, 25, 50 })
            QVERIFY(IqamahClient::isValidRadius(radius));
        for (int radius : { 0, 1, 3, 20, 30, 60, 100, -10 })
            QVERIFY2(!IqamahClient::isValidRadius(radius), qPrintable(QString::number(radius)));
    }

    void anUnacceptableRadiusNeverReachesTheNetwork()
    {
        FakeTransport transport;
        IqamahClient client(&transport);
        QString error;
        QCOMPARE(client.fetchNearby(34.55, 9.48, 20, nullptr, nullptr, &error), ClientError::InvalidArgument);
        QVERIFY(!error.isEmpty());
        QCOMPARE(transport.requests.size(), 0);
        QCOMPARE(client.requestCount(), 0);
    }

    void impossibleCoordinatesAreRejected()
    {
        FakeTransport transport;
        IqamahClient client(&transport);
        QCOMPARE(client.fetchYear(91.0, 9.48, 2027, nullptr), ClientError::InvalidArgument);
        QCOMPARE(client.fetchYear(34.55, 181.0, 2027, nullptr), ClientError::InvalidArgument);
        QCOMPARE(client.fetchYear(34.55, 9.48, 1900, nullptr), ClientError::InvalidArgument);
        QCOMPARE(transport.requests.size(), 0);
    }

    void everyRequestIdentifiesItselfAndUsesHttps()
    {
        FakeTransport transport;
        transport.push(200, "{}");
        IqamahClient client(&transport);
        QCOMPARE(client.fetchYear(34.55, 9.48, 2027, nullptr), ClientError::None);
        QCOMPARE(transport.requests.size(), 1);
        QVERIFY(!transport.requests.first().userAgent.isEmpty());
        QVERIFY(transport.requests.first().userAgent.contains(QLatin1String("islamic-times")));
        QCOMPARE(transport.requests.first().url.scheme(), QStringLiteral("https"));
    }

    void a403ExplainsItself()
    {
        FakeTransport transport;
        transport.push(403);
        IqamahClient client(&transport);
        QString error;
        QCOMPARE(client.fetchMosqueDay(QStringLiteral("abc"), nullptr, &error), ClientError::HttpStatus);
        QVERIFY(error.contains(QLatin1String("User-Agent")));
        QCOMPARE(transport.requests.size(), 1);      // no retry on a client error
    }

    void a301IsReportedNotFollowed()
    {
        FakeTransport transport;
        transport.push(301);
        IqamahClient client(&transport);
        QString error;
        QCOMPARE(client.fetchNearby(34.55, 9.48, 10, nullptr, nullptr, &error), ClientError::Redirect);
        QVERIFY(error.contains(QLatin1String("radius")));
    }

    void aServerErrorIsRetriedOnce()
    {
        FakeTransport transport;
        transport.push(500);
        transport.push(500);
        IqamahClient client(&transport);
        QCOMPARE(client.fetchYear(34.55, 9.48, 2027, nullptr), ClientError::HttpStatus);
        QCOMPARE(transport.requests.size(), 2);
    }

    void aFlakyConnectionSucceedsOnTheSecondTry()
    {
        FakeTransport transport;
        transport.push(0, {}, QStringLiteral("connection reset"));
        transport.push(200, "{\"days\":[]}");
        IqamahClient client(&transport);
        QByteArray body;
        QCOMPARE(client.fetchYear(34.55, 9.48, 2027, &body), ClientError::None);
        QCOMPARE(transport.requests.size(), 2);
        QCOMPARE(body, QByteArray("{\"days\":[]}"));
    }

    void anEmptyBodyIsAnErrorNotANullDay()
    {
        FakeTransport transport;
        transport.push(200, QByteArray());
        IqamahClient client(&transport);
        QCOMPARE(client.fetchYear(34.55, 9.48, 2027, nullptr), ClientError::Empty);
    }

    void theBodyComesBackUntouched()
    {
        const QByteArray original(m_nearby);
        FakeTransport transport;
        transport.push(200, original);
        IqamahClient client(&transport);
        QByteArray body;
        QCOMPARE(client.fetchNearby(34.55, 9.48, 50, nullptr, &body), ClientError::None);
        QCOMPARE(body, original);
    }

    void nearbyParsesTheRealAnswerForHisTown()
    {
        FakeTransport transport;
        transport.push(200, m_nearby);
        IqamahClient client(&transport);
        QVector<MosqueInfo> mosques;
        QCOMPARE(client.fetchNearby(34.55, 9.48, 50, &mosques), ClientError::None);
        QCOMPARE(mosques.size(), 1);
        QCOMPARE(mosques.first().name, QStringLiteral("NiCd Valley Mosque"));
        QCOMPARE(mosques.first().distanceKm, 17.36);
        QCOMPARE(mosques.first().city, QStringLiteral("Bu Ballal"));
        QVERIFY(!mosques.first().hasSource);
    }

    void anEmptyAnswerIsZeroMosquesNotAnError()
    {
        FakeTransport transport;
        transport.push(200, "{\"total\":0,\"mosques\":[]}");
        IqamahClient client(&transport);
        QVector<MosqueInfo> mosques;
        QCOMPARE(client.fetchNearby(34.55, 9.48, 10, &mosques), ClientError::None);
        QVERIFY(mosques.isEmpty());
    }
};

QTEST_GUILESS_MAIN(TestIqamahClient)
#include "tst_iqamahclient.moc"
