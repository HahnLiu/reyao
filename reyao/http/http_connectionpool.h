#pragma once

#include "reyao/socket_stream.h"
#include "reyao/http/http_request.h"
#include "reyao/http/http_response.h"
#include "reyao/socket.h"
#include "reyao/uri.h"
#include "reyao/mutex.h"
#include "reyao/http/http_connection.h"

#include <list>
#include <atomic>
#include <memory>

namespace reyao {



class HttpConnectionPool {
public:
    typedef std::shared_ptr<HttpConnectionPool> SPtr;
    HttpConnectionPool(const std::string& host, const std::string& vhost,
                       uint16_t port, uint32_t max_conn,
                       int64_t max_alive_time, uint32_t max_request);

    HttpConnection::SPtr getConnection();

    HttpResult::SPtr doGet(const std::string& url,
                           uint64_t timeout,
                           const HttpRequest::StrMap& headers = {});

    HttpResult::SPtr doPost(const std::string& url,
                            uint64_t timeout,
                            const HttpRequest::StrMap& headers = {},
                            const std::string& body = "");

    HttpResult::SPtr doGet(Uri::SPtr uri,
                           uint64_t timeout,
                           const HttpRequest::StrMap& headers = {});

    HttpResult::SPtr doPost(Uri::SPtr uri,
                            uint64_t timeout,
                            const HttpRequest::StrMap& headers = {},
                            const std::string& body = "");                                     

    HttpResult::SPtr doRequest(HttpMethod method,
                               Uri::SPtr uri,
                               uint64_t timeout,
                               const HttpRequest::StrMap& headers = {},
                               const std::string& body = "");

    HttpResult::SPtr doRequest(HttpMethod method,
                               const std::string& url,
                               uint64_t timeout,
                               const HttpRequest::StrMap& headers = {},
                               const std::string& body = "");                                        

    HttpResult::SPtr doRequest(HttpRequest* req,
                               uint64_t timeout);

    static void RelasePtr(HttpConnection* conn, HttpConnectionPool* pool);

private:
    std::string host_;
    std::string vhost_;
    uint16_t port_;
    uint32_t max_conn_;
    int64_t max_alive_time_;
    uint32_t max_request_; //一条连接最多请求

    Mutex mutex_;
    std::list<HttpConnection*> conns_;
    std::atomic<int32_t> total_{0};

};


} //namespace reyao