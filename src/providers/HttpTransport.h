#pragma once

#include <QByteArray>
#include <QString>
#include <QUrl>

namespace islamic {

struct HttpRequest {
    QUrl url;
    QString userAgent;
    int timeoutMs = 8000;
};

struct HttpReply {
    int status = 0;          // 0 when the request never completed
    QByteArray body;
    QString error;           // transport-level failure, empty on success

    bool ok() const { return error.isEmpty() && status >= 200 && status < 300; }
};

// One synchronous GET. Synchronous on purpose: the chain runs it on a worker
// thread, and a blocking call is trivial to fake in tests. Implementations must
// not follow redirects silently — the provider answers 301 on an unknown query,
// and that must surface as an error rather than a surprise 200.
class HttpTransport
{
public:
    virtual ~HttpTransport() = default;
    virtual HttpReply get(const HttpRequest &request) = 0;
};

} // namespace islamic
