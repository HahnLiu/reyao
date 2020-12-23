#include "reyao/http/http_server.h"

using namespace reyao;

// void test() {
//     net::HttpServer::SPtr_t server(new net::HttpServer(false));
//     auto addr = net::IPAddress::GetAddressByHost("0.0.0.0", 30000);
//    // addr->setPort(30000);
// //    auto dispatch = server->getDispatch();
// //    dispatch->addServlet("/hahn/request", [](net::HttpRequest::SPtr_t request,
// //                                                   net::HttpResponse::SPtr_t response,
// //                                                   net::HttpSession::SPtr_t session) {
// //        response->setBody(request->toString());
// //        return 0;           
// //    });
// //        dispatch->addGlobalServlet("/hahn/*", [](net::HttpRequest::SPtr_t request,
// //                                                             net::HttpResponse::SPtr_t response,
// //                                                             net::HttpSession::SPtr_t session) {
// //        response->setBody("Global-Match: \r\n" + request->toString());
// //        return 0;           
// //    });
//     if (server->bind(addr)) {
//         server->start();
//     } else {
//         LOG_ERROR << "bind addr=" << addr->toString() 
//                   << " error=" << strerror(errno);
//     }
// }

int main(int argc, char** argv) {
    g_logger->setLevel(LogLevel::WARN);
    int num = 0;
    if (argc > 1) {
        num = atoi(argv[1]);
    }
    Scheduler sh(num);
    IPv4Address addr("0.0.0.0", 8010);
    HttpServer server(&sh, addr, true);
    auto dispatch = server.getDispatch();
    dispatch->addServlet("/hello", [](const HttpRequest& req,
                                      HttpResponse* rsp,
                                      const HttpSession& session) {
        rsp->addHeader("Server", "Reyao");
        rsp->addHeader("Conetnt-Type", "text/plain");
        rsp->setBody("hello, world!");
        return 0;           
    });


    server.start();
    sh.start();
    return 0;
}
