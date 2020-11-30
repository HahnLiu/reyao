#include "reyao/http/http_session.h"
#include "reyao/http/http_parser.h"
#include "reyao/util.h"
#include "reyao/log.h"

namespace reyao {

HttpSession::HttpSession(std::shared_ptr<Socket> sock, bool owner)
    : SocketStream(sock, owner) {

}

bool HttpSession::recvRequest(HttpRequest* req) { //FIXME:
    HttpRequestParser parser(this, req);
    return parser.parseRequest();
}

bool HttpSession::sendResponse(HttpResponse* rsp) {
    std::stringstream ss;
    rsp->dump(ss);
    std::string msg = ss.str();
    return write(msg.c_str(), msg.size());
}


} //namespace reyao