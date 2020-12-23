#include "reyao/http/http_connection.h"
#include "reyao/http/http_parser.h"
#include "reyao/log.h"

namespace reyao {


std::string HttpResult::toString() const {
    std::stringstream ss;
    ss << "error:" << error
       << "response:" << (response ? response->toString() : "nullptr");
    return ss.str();
}

HttpConnection::HttpConnection(std::shared_ptr<Socket> sock, bool owner)
    : SocketStream(sock, owner) {
    create_time_ = getCreateTime();
}

HttpConnection::~HttpConnection() { 
    // LOG_DEBUG << "~HttpConnection"; 
}

bool HttpConnection::recvResponse(HttpResponse* rsp) {
    HttpResponseParser parser(this, rsp);
    return parser.parseResponse();
}

bool HttpConnection::sendRequest(HttpRequest* req) {
    std::stringstream ss;
    req->dump(ss);
    std::string msg = ss.str();
    return write(msg.c_str(), msg.size());
}

HttpResult::SPtr HttpConnection::DoGet(const std::string& url,
                                       uint64_t timeout,
                                       const HttpRequest::StrMap& headers) {
    auto uri = Uri::Create(url);
    if (!uri) {
        return std::make_shared<HttpResult>(HttpResult::Error::INVALID_URL,
                                            nullptr, "invalid url:" + url);
    }
    return DoGet(uri, timeout, headers);
}

HttpResult::SPtr HttpConnection::DoPost(const std::string& url,
                                        uint64_t timeout,
                                        const HttpRequest::StrMap& headers,
                                        const std::string& body) {
    auto uri = Uri::Create(url);
    if (!uri) {
        return std::make_shared<HttpResult>(HttpResult::Error::INVALID_URL,
                                            nullptr, "invalid url:" + url);
    }
    return DoPost(uri, timeout, headers, body);
}

HttpResult::SPtr HttpConnection::DoGet(Uri::SPtr uri,
                                       uint64_t timeout,
                                       const HttpRequest::StrMap& headers) {
    return DoRequest(HttpMethod::GET, uri, timeout, headers);
}

HttpResult::SPtr HttpConnection::DoPost(Uri::SPtr uri,
                                        uint64_t timeout,
                                        const HttpRequest::StrMap& headers,
                                        const std::string& body) {
    return DoRequest(HttpMethod::POST, uri, timeout, headers, body);
}                                     

HttpResult::SPtr HttpConnection::DoRequest(HttpMethod method,
                                           Uri::SPtr uri,
                                           uint64_t timeout,
                                           const HttpRequest::StrMap& headers,
                                           const std::string& body) {
    HttpRequest req;
    req.setMethod(method);
    req.setPath(uri->getPath());
    req.setQuery(uri->getQuery());
    req.setFragment(uri->getFragment());
    // req.setVersion();
    bool has_host = false;
    for (auto& header : headers) {
        if (strcasecmp(header.first.c_str(), "Connection") == 0) {
            if (strcasecmp(header.second.c_str(), "keep-Alive") == 0) {
                req.setKeepAlive(true);
            }
            continue;
        }
        if (!has_host && strcasecmp(header.first.c_str(), "host") == 0) {
            has_host = !header.second.empty();
        }
        req.addHeader(header.first, header.second);
    }
    if (!has_host) {
        //TODO: 
        req.addHeader("Host", uri->getHost());
    }
    req.setBody(body);
    return DoRequest(&req, uri, timeout);
}

HttpResult::SPtr HttpConnection::DoRequest(HttpMethod method,
                                           const std::string& url,
                                           uint64_t timeout,
                                           const HttpRequest::StrMap& headers,
                                           const std::string& body) {
    auto uri = Uri::Create(url);
    if (!uri) {
        return std::make_shared<HttpResult>(HttpResult::Error::INVALID_URL,
                                            nullptr, "invalid url:" + url);
    }
    return DoRequest(method, uri, timeout, headers, body);
}                                        

HttpResult::SPtr HttpConnection::DoRequest(HttpRequest* req,
                                           Uri::SPtr uri,
                                           uint64_t timeout) {
    IPv4Address::SPtr addr = uri->getAddr();
    if (!addr) {
        return std::make_shared<HttpResult>(HttpResult::Error::INVALID_HOST, 
                                            nullptr, "invalid host:" + uri->getHost());
    }
    Socket::SPtr sock = Socket::CreateTcp();
    
    sock->setRecvTimeout(timeout);
    if (!sock->connect(*addr)) {
        return std::make_shared<HttpResult>(HttpResult::Error::CONNECT_FAIL, 
                                            nullptr, "peer addr:" + addr->toString());
    }
    LOG_INFO << "request:" << req->toString();
    HttpConnection::SPtr connect(new HttpConnection(sock));
    int rt = connect->sendRequest(req);
    if (rt == 0) {
        return std::make_shared<HttpResult>(HttpResult::Error::CLOSE_BY_PEER, 
                                            nullptr, "send request close by peer addr:" + addr->toString());
    }
    if (rt < 0) {
        return std::make_shared<HttpResult>(HttpResult::Error::SOCKET_ERROR, 
                                            nullptr, "socket error:" + std::string(strerror(errno)));
    }
    HttpResponse::SPtr rsp(new HttpResponse);
    if (!connect->recvResponse(rsp.get())) { //TODO: maybe recv an invalid response message
        return std::make_shared<HttpResult>(HttpResult::Error::TIMEOUT, 
                                            nullptr, "recv timout out from addr:" + addr->toString()
                                            + "timeout:" + std::to_string(timeout));
    }

    return std::make_shared<HttpResult>(HttpResult::Error::OK, rsp, "ok");
}



} //namespace reyao