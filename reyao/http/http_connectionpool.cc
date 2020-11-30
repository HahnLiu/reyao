#include "reyao/http/http_connectionpool.h"
#include "reyao/util.h"
#include "reyao/log.h"

#include <functional>

namespace reyao {

HttpConnectionPool::HttpConnectionPool(const std::string& host, const std::string& vhost,
                                       uint16_t port,uint32_t max_conn,
                                       int64_t max_alive_time, uint32_t max_request) 
    : host_(host),
      vhost_(vhost),
      port_(port),
      max_conn_(max_conn),
      max_alive_time_(max_alive_time),
      max_request_(max_request) {

}

HttpConnection::SPtr HttpConnectionPool::getConnection() {
    int64_t now = GetCurrentTime();
    std::vector<HttpConnection*> invalid_conns;
    HttpConnection* conn = nullptr;
    {
        MutexGuard lock(mutex_);
        while (!conns_.empty()) {
            auto connection = *conns_.begin();
            conns_.pop_front();
            if (!connection->isConnected()) {
                invalid_conns.push_back(connection);
                continue;
            }
            if (connection->getCreateTime() + max_alive_time_ > now) {
                invalid_conns.push_back(connection);
                continue; 
            }
            connection = conn;
            break;
        }
    }

    for (auto it : invalid_conns) {
        delete it;
    }
    total_ -= invalid_conns.size();

    if (!conn) {
        auto addr = IPv4Address::CreateByName(host_.c_str(), port_);
        if (!addr) {
            LOG_ERROR << "getAddrByHost:" << host_;
            return nullptr;
        }
        Socket::SPtr sock(new Socket(SOCK_STREAM));
        sock->socket();
        if (!sock->connect(*addr)) {
            LOG_ERROR << "connect fail:" << addr->toString();
            return nullptr;
        }
        ++total_;
        conn = new HttpConnection(sock);

    }
    return HttpConnection::SPtr(conn, std::bind(&HttpConnectionPool::RelasePtr,
                                std::placeholders::_1, this));
}

HttpResult::SPtr HttpConnectionPool::doGet(const std::string& url,
                                           uint64_t timeout,
                                           const HttpRequest::StrMap& headers) {
    return doRequest(HttpMethod::GET, url, timeout, headers);
}

HttpResult::SPtr HttpConnectionPool::doPost(const std::string& url,
                                            uint64_t timeout,
                                            const HttpRequest::StrMap& headers,
                                            const std::string& body) {
    return doRequest(HttpMethod::POST, url, timeout, headers, body);                                                  
}

HttpResult::SPtr HttpConnectionPool::doGet(Uri::SPtr uri,
                                           uint64_t timeout,
                                           const HttpRequest::StrMap& headers ) {
    std::stringstream ss;
    ss << uri->getPath()
       << (uri->getQuery().empty() ? "" : "?")
       << uri->getQuery()
       << (uri->getFragment().empty() ? "" : "#")
       << uri->getFragment();
    return HttpConnection::DoGet(ss.str(), timeout, headers);
}

HttpResult::SPtr HttpConnectionPool::doPost(Uri::SPtr uri,
                                            uint64_t timeout,
                                            const HttpRequest::StrMap& headers,
                                            const std::string& body) {
    std::stringstream ss;
    ss << uri->getPath()
       << (uri->getQuery().empty() ? "" : "?")
       << uri->getQuery()
       << (uri->getFragment().empty() ? "" : "#")
       << uri->getFragment();
    return HttpConnection::DoPost(ss.str(), timeout, headers, body);
}                                  

HttpResult::SPtr HttpConnectionPool::doRequest(HttpMethod method,
                                               Uri::SPtr uri,
                                               uint64_t timeout,
                                               const HttpRequest::StrMap& headers,
                                               const std::string& body) {
    std::stringstream ss;
    ss << uri->getPath()
       << (uri->getQuery().empty() ? "" : "?")
       << uri->getQuery()
       << (uri->getFragment().empty() ? "" : "#")
       << uri->getFragment();
    return doRequest(method, ss.str(), timeout, headers, body);
}

HttpResult::SPtr HttpConnectionPool::doRequest(HttpMethod method,
                                               const std::string& url,
                                               uint64_t timeout,
                                               const HttpRequest::StrMap& headers,
                                               const std::string& body) {
    HttpRequest req;
    req.setMethod(method);
    req.setPath(url);
    bool has_host = false;
    for (auto& header : headers) {
        if (strcasecmp(header.first.c_str(), "connection") == 0) {
            if (strcasecmp(header.second.c_str(), "keep-alive") == 0) {
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
        if (vhost_.empty()) {
            req.addHeader("Host", host_);
        } else {
            req.addHeader("Host", vhost_);
        }
    }
    req.setBody(body);
    return doRequest(&req, timeout);
}                                       

HttpResult::SPtr HttpConnectionPool::doRequest(HttpRequest* req,
                                               uint64_t timeout) {
    auto conn = getConnection();
    if (!conn) {
        return std::make_shared<HttpResult>(HttpResult::Error::POOL_GET_CONN, 
                                            nullptr, "pool host:" + host_ + 
                                            " port:" + std::to_string(port_));
    }
    auto sock = conn->getSock();
    if (!sock) {
        return std::make_shared<HttpResult>(HttpResult::Error::POOL_INVALID_CONN, 
                                            nullptr, "pool host:" + host_ + 
                                            " port:" + std::to_string(port_));
    }
    sock->setRecvTimeout(timeout);
    int rt = conn->sendRequest(req);
    if (rt == 0) {
        return std::make_shared<HttpResult>(HttpResult::Error::CLOSE_BY_PEER, 
                                            nullptr, "send request close by peer addr:" 
                                            + sock->getPeerAddr()->toString());
    }
    if (rt < 0) {
        return std::make_shared<HttpResult>(HttpResult::Error::SOCKET_ERROR, 
                                            nullptr, "socket error:" + std::string(strerror(errno)));
    }
    HttpResponse rsp;
    if (!conn->recvResponse(&rsp)) { //TODO: maybe recv an invalid response message
        return std::make_shared<HttpResult>(HttpResult::Error::TIMEOUT, 
                                            nullptr, "recv timout out from addr:" 
                                            + sock->getPeerAddr()->toString()
                                            + "timeout:" + std::to_string(timeout));
    }

    return std::make_shared<HttpResult>(HttpResult::Error::OK, &rsp, "ok");
}

//TODO:
void HttpConnectionPool::RelasePtr(HttpConnection* conn, HttpConnectionPool* pool) {
    if (!conn->isConnected() ||
        conn->getCreateTime() + pool->max_alive_time_ >= GetCurrentTime() ||
        conn->getRequestCount() >= pool->max_request_) { //TODO: how to detect peer close
        delete conn;
        --pool->total_;
        return;
    }
    MutexGuard lock(pool->mutex_);
    pool->conns_.push_back(conn);
}

} //namespace reyao