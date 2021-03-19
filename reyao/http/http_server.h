#pragma once

#include "reyao/tcp_server.h"
#include "reyao/http/http_session.h"
#include "reyao/http/http_servlet.h"

namespace reyao {

class HttpServer : public TcpServer {
public:
    typedef std::shared_ptr<HttpServer> SPtr;
    HttpServer(Scheduler* sche,
               IPv4Address::SPtr addr,
               bool keepAlive = false);

    void handleClient(Socket::SPtr client) override;

    ServletDispatch::SPtr getDispatch() const { return dispatch_; }
    void setServletDispatch(ServletDispatch::SPtr dispatch) { dispatch_ = dispatch; }

private:
    bool keepAlive_;
    ServletDispatch::SPtr dispatch_;
};

} // namespace reyao