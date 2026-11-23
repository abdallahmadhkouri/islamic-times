#include "providers/IqamahClient.h"

#include <QJsonArray>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>

namespace islamic {

namespace {

QString describeStatus(int status)
{
    if (status == 403)
        return QStringLiteral("the provider rejected the request (403) — it wants a User-Agent");
    if (status == 404)
        return QStringLiteral("the provider does not know this resource (404)");
    if (status >= 300 && status < 400)
        return QStringLiteral("the provider redirected (%1) — an unknown parameter, probably the radius").arg(status);
    return QStringLiteral("the provider answered %1").arg(status);
}

bool validCoordinate(double lat, double lon)
{
    return lat >= -90.0 && lat <= 90.0 && lon >= -180.0 && lon <= 180.0;
}

QString coordinate(double value)
{
    return QString::number(value, 'f', 2);
}

} // namespace

IqamahClient::IqamahClient(HttpTransport *transport, QString baseUrl, QString userAgent)
    : m_transport(transport), m_baseUrl(std::move(baseUrl)), m_userAgent(std::move(userAgent))
{
}

bool IqamahClient::isValidRadius(int radiusKm)
{
    switch (radiusKm) {
    case 2: case 5: case 10: case 25: case 50:
        return true;
    default:
        return false;
    }
}

QUrl IqamahClient::yearUrl(const QString &baseUrl, double lat, double lon, int year)
{
    return QUrl(QStringLiteral("%1/times/%2,%3/%4.json")
                    .arg(baseUrl, coordinate(lat), coordinate(lon)).arg(year));
}

QUrl IqamahClient::nearbyUrl(const QString &baseUrl, double lat, double lon, int radiusKm)
{
    QUrl url(QStringLiteral("%1/mosques/near/%2,%3.json")
                 .arg(baseUrl, coordinate(lat), coordinate(lon)));

    // 10 km is the API's own default, and sending it explicitly is answered with a
    // 301 to the bare URL — measured, twice. So the canonical request omits it.
    // The comma must stay literal too: %2C is a 404.
    if (radiusKm != 10) {
        QUrlQuery query;
        query.addQueryItem(QStringLiteral("radius_km"), QString::number(radiusKm));
        url.setQuery(query);
    }
    return url;
}

QUrl IqamahClient::mosqueUrl(const QString &baseUrl, const QString &mosqueId)
{
    return QUrl(QStringLiteral("%1/mosque/%2.json").arg(baseUrl, mosqueId));
}

ClientError IqamahClient::get(const QUrl &url, QByteArray *body, QString *error)
{
    if (error)
        error->clear();
    if (!m_transport) {
        if (error)
            *error = QStringLiteral("no transport");
        return ClientError::Network;
    }

    HttpRequest request;
    request.url = url;
    request.userAgent = m_userAgent;

    // One retry, and only for the failures a second attempt can plausibly fix.
    HttpReply reply;
    for (int attempt = 0; attempt < 2; ++attempt) {
        reply = m_transport->get(request);
        ++m_requests;
        const bool retryable = !reply.error.isEmpty() || reply.status >= 500;
        if (reply.ok() || !retryable)
            break;
    }

    if (!reply.error.isEmpty()) {
        if (error)
            *error = reply.error;
        return ClientError::Network;
    }
    if (reply.status >= 300 && reply.status < 400) {
        if (error)
            *error = describeStatus(reply.status);
        return ClientError::Redirect;
    }
    if (!reply.ok()) {
        if (error)
            *error = describeStatus(reply.status);
        return ClientError::HttpStatus;
    }
    if (reply.body.isEmpty()) {
        if (error)
            *error = QStringLiteral("empty body");
        return ClientError::Empty;
    }

    if (body)
        *body = reply.body;
    return ClientError::None;
}

ClientError IqamahClient::fetchYear(double lat, double lon, int year, QByteArray *body, QString *error)
{
    if (!validCoordinate(lat, lon) || year < 2000 || year > 2100) {
        if (error)
            *error = QStringLiteral("impossible coordinate or year");
        return ClientError::InvalidArgument;
    }
    return get(yearUrl(m_baseUrl, lat, lon, year), body, error);
}

ClientError IqamahClient::fetchNearby(double lat, double lon, int radiusKm,
                                      QVector<MosqueInfo> *mosques, QByteArray *body, QString *error)
{
    if (!validCoordinate(lat, lon) || !isValidRadius(radiusKm)) {
        if (error)
            *error = QStringLiteral("radius_km must be one of 2, 5, 10, 25, 50");
        return ClientError::InvalidArgument;
    }

    QByteArray payload;
    const ClientError result = get(nearbyUrl(m_baseUrl, lat, lon, radiusKm), &payload, error);
    if (result != ClientError::None)
        return result;

    if (mosques)
        *mosques = parseNearby(payload);
    if (body)
        *body = payload;
    return ClientError::None;
}

ClientError IqamahClient::fetchMosqueDay(const QString &mosqueId, QByteArray *body, QString *error)
{
    if (mosqueId.trimmed().isEmpty()) {
        if (error)
            *error = QStringLiteral("empty mosque id");
        return ClientError::InvalidArgument;
    }
    return get(mosqueUrl(m_baseUrl, mosqueId), body, error);
}

QVector<MosqueInfo> IqamahClient::parseNearby(const QByteArray &body)
{
    QVector<MosqueInfo> result;
    const QJsonObject root = QJsonDocument::fromJson(body).object();
    const QJsonArray mosques = root.value(QStringLiteral("mosques")).toArray();
    for (const QJsonValue &value : mosques) {
        const QJsonObject object = value.toObject();
        MosqueInfo mosque;
        mosque.id = object.value(QStringLiteral("id")).toString();
        mosque.name = object.value(QStringLiteral("name")).toString();
        mosque.city = object.value(QStringLiteral("city")).toString();
        mosque.country = object.value(QStringLiteral("country")).toString();
        mosque.distanceKm = object.value(QStringLiteral("distance_km")).toDouble(-1);
        mosque.hasSource = object.value(QStringLiteral("has_source")).toBool(false);
        const QJsonObject location = object.value(QStringLiteral("location")).toObject();
        mosque.lat = location.value(QStringLiteral("lat")).toDouble();
        mosque.lon = location.value(QStringLiteral("lon")).toDouble();
        if (mosque.isValid())
            result.append(mosque);
    }
    return result;
}

} // namespace islamic
