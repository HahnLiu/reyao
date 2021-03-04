#include "reyao/http/http_server.h"

namespace reyao {

HttpServer::HttpServer(Scheduler* scheduler,
                       IPv4Address::SPtr addr,
                       bool keep_alive) 
    : TcpServer(scheduler, addr, "HttpServer"),
      keep_alive_(keep_alive) {
    dispatch_.reset(new ServletDispatch); //TODO: server name
}

void HttpServer::handleClient(Socket::SPtr client) {
    HttpSession session(client);
    do {
        HttpRequest req;
        if (!session.recvRequest(&req)) {
            LOG_WARN << "recv http request fail, client="
                     << client->toString();
            break;
        }
        std::string con_info = req.getHeader("Connection");
        if (!strcasecmp(con_info.c_str(), "Keep-Alive")) {
            req.setKeepAlive(true);
        }

        HttpResponse rsp(req.getVersion(),
                         req.isKeepAlive() && keep_alive_);
  
        dispatch_->handle(req, &rsp, session);
        // rsp.addHeader("Server", "reyao");
        // rsp.addHeader("Conetnt-Type", "text/plain");
        // rsp.setBody("hello, world!\n");
 
        session.sendResponse(&rsp);

        if (!keep_alive_ || !req.isKeepAlive()) {
            break;
        }
    } while (true);
    
    session.close();
}

} //namespace reyaop