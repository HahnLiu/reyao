#pragma once

#include "reyao/socket_stream.h"
#include "reyao/http/http_request.h"
#include "reyao/http/http_response.h"
#include "reyao/socket.h"

#include <memory>

namespace reyao {

class HttpSession : public SocketStream {
public:
    HttpSession(std::shared_ptr<Socket> sock, bool owner = true);
    
    bool recvRequest(HttpRequest* req);
    bool sendResponse(HttpResponse* rsp);

private:
    Socket::SPtr sock_;
    bool owner_;
};


} // namespace reyao