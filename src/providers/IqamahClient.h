#pragma once

#include "providers/HttpTransport.h"

#include <QString>
#include <QVector>

namespace islamic {

enum class ClientError {
    None,
    InvalidArgument,   // a radius the API does not accept, or an impossible coordinate
    Network,           // never reached the provider
    HttpStatus,        // the provider answered, but not with 2xx
    Redirect,          // 3xx: we do not follow, we report
    Empty              // 2xx with no body
};

struct MosqueInfo {
    QString id;
    QString name;
    QString city;
    QString country;
    double distanceKm = -1;
    double lat = 0;
    double lon = 0;
    bool hasSource = false;    // the provider has a published timetable for it
    bool isValid() const { return !id.isEmpty(); }
};

// Talks to api.iqamah.net and nothing else. It returns the provider's bytes
// untouched, so the caller can cache exactly what arrived and parse it whenever it
// likes; parsing never happens twice with two different results.
class IqamahClient
{
public:
    IqamahClient(HttpTransport *transport,
                 QString baseUrl = QStringLiteral("https://api.iqamah.net/v1"),
                 QString userAgent = QStringLiteral("islamic-times/0.1 (+https://github.com/abdallahmadhkouri/islamic-times)"));

    // The API accepts exactly these, and nothing in between.
    static bool isValidRadius(int radiusKm);

    static QUrl yearUrl(const QString &baseUrl, double lat, double lon, int year);
    static QUrl nearbyUrl(const QString &baseUrl, double lat, double lon, int radiusKm);
    static QUrl mosqueUrl(const QString &baseUrl, const QString &mosqueId);

    ClientError fetchYear(double lat, double lon, int year, QByteArray *body, QString *error = nullptr);
    ClientError fetchNearby(double lat, double lon, int radiusKm, QVector<MosqueInfo> *mosques,
                            QByteArray *body = nullptr, QString *error = nullptr);
    ClientError fetchMosqueDay(const QString &mosqueId, QByteArray *body, QString *error = nullptr);

    // Exposed for tests and for the chain, which needs to parse a cached body.
    static QVector<MosqueInfo> parseNearby(const QByteArray &body);

    int requestCount() const { return m_requests; }

private:
    ClientError get(const QUrl &url, QByteArray *body, QString *error);

    HttpTransport *m_transport = nullptr;
    QString m_baseUrl;
    QString m_userAgent;
    int m_requests = 0;
};

} // namespace islamic
