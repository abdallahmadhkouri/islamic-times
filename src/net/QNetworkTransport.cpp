#include "net/QNetworkTransport.h"

#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>

namespace islamic {

QNetworkTransport::QNetworkTransport()
    : m_manager(std::make_unique<QNetworkAccessManager>())
{
}

QNetworkTransport::~QNetworkTransport() = default;

HttpReply QNetworkTransport::get(const HttpRequest &request)
{
    HttpReply reply;
    if (!request.url.isValid() || request.url.scheme() != QLatin1String("https")) {
        reply.error = QStringLiteral("only https is allowed");
        return reply;
    }

    QNetworkRequest networkRequest(request.url);
    networkRequest.setHeader(QNetworkRequest::UserAgentHeader,
                             request.userAgent.isEmpty() ? QStringLiteral("islamic-times") : request.userAgent);
    networkRequest.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                               QNetworkRequest::ManualRedirectPolicy);
    networkRequest.setTransferTimeout(request.timeoutMs);

    QNetworkReply *pending = m_manager->get(networkRequest);
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(request.timeoutMs);

    QObject::connect(pending, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start();
    loop.exec();

    if (!pending->isFinished()) {
        pending->abort();
        reply.error = QStringLiteral("timeout after %1 ms").arg(request.timeoutMs);
    } else if (pending->error() != QNetworkReply::NoError
               && pending->error() != QNetworkReply::ContentNotFoundError) {
        reply.error = pending->errorString();
    }

    reply.status = pending->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    reply.body = pending->readAll();
    pending->deleteLater();
    return reply;
}

} // namespace islamic
