#pragma once

#include "providers/HttpTransport.h"

#include <memory>

class QNetworkAccessManager;

namespace islamic {

// The real transport. QtNetwork lives in its own target (islamic-times-net) so the
// core library stays QtCore-only and a consumer that never fetches pays nothing.
//
// Blocking: call it from a worker thread. Redirects are manual, because a 301 from
// this API means "bad parameter", not "moved here".
class QNetworkTransport : public HttpTransport
{
public:
    QNetworkTransport();
    ~QNetworkTransport() override;

    HttpReply get(const HttpRequest &request) override;

private:
    std::unique_ptr<QNetworkAccessManager> m_manager;
};

} // namespace islamic
