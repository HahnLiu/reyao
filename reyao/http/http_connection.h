#pragma once

#include "reyao/socket_stream.h"
#include "reyao/socket.h"
#include "reyao/uri.h"
#include "reyao/mutex.h"
#include "reyao/http/http_request.h"
#include "reyao/http/http_response.h"

#include <list>
#include <atomic>
#include <memory>

namespace reyao {

struct HttpResult {
    typedef std::shared_ptr<HttpResult> SPtr;
    enum class Error {
        OK,
        INVALID_URL,
        INVALID_HOST,
        CONNECT_FAIL,
        CLOSE_BY_PEER,
        SOCKET_ERROR,
        TIMEOUT,
        POOL_GET_CONN,
        POOL_INVALID_CONN
    };


    HttpResult(Error res, HttpResponse::SPtr rsp, const std::string& err)
        : result(res),
          response(rsp),
          error(err) {}

    std::string toString() const;

    Error result;
    HttpResponse::SPtr response;
    std::string error;

};

class HttpConnection : public SocketStream {
public:
    typedef std::shared_ptr<HttpConnection> SPtr;
    HttpConnection(std::shared_ptr<Socket> sock, bool owner = true);
    ~HttpConnection();
    
    int64_t getCreateTime() const { return create_time_; }
    uint32_t getRequestCount() const { return request_count_; }

    bool recvResponse(HttpResponse* rsp);
    bool sendRequest(HttpRequest* req);

    static HttpResult::SPtr DoGet(const std::string& url,
                                  uint64_t timeout,
                                  const HttpRequest::StrMap& headers = {});

    static HttpResult::SPtr DoPost(const std::string& url,
                                   uint64_t timeout,
                                   const HttpRequest::StrMap& headers = {},
                                   const std::string& body = "");

    static HttpResult::SPtr DoGet(Uri::SPtr uri,
                                  uint64_t timeout,
                                  const HttpRequest::StrMap& headers = {});

    static HttpResult::SPtr DoPost(Uri::SPtr uri,
                                   uint64_t timeout,
                                   const HttpRequest::StrMap& headers = {},
                                   const std::string& body = "");                                     

    static HttpResult::SPtr DoRequest(HttpMethod method,
                                      Uri::SPtr uri,
                                      uint64_t timeout,
                                      const HttpRequest::StrMap& headers = {},
                                      const std::string& body = "");

    static HttpResult::SPtr DoRequest(HttpMethod method,
                                      const std::string& url,
                                      uint64_t timeout,
                                      const HttpRequest::StrMap& headers = {},
                                      const std::string& body = "");                                        

    static HttpResult::SPtr DoRequest(HttpRequest* req,
                                      Uri::SPtr uri,
                                      uint64_t timeout);

private:
    int64_t create_time_; //TODO:
    uint32_t request_count_ = 0;
};


} //namespace reyao