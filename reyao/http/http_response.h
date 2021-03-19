#pragma once 

#include <string.h>

#include <string>
#include <map>
#include <iostream>
#include <sstream>
#include <memory>

namespace reyao {


/* Status Codes */
#define HTTP_STATUS_MAP(XX)                                                 \
  XX(200, OK,                              OK)                              \
  XX(301, MOVED_PERMANENTLY,               Moved Permanently)               \
  XX(400, BAD_REQUEST,                     Bad Request)                     \
  XX(401, UNAUTHORIZED,                    Unauthorized)                    \
  XX(404, NOT_FOUND,                       Not Found)                       \
  XX(500, INTERNAL_SERVER_ERROR,           Internal Server Error)           \


enum class HttpStatus {
#define XX(code, name, string) name = code,
    HTTP_STATUS_MAP(XX)
#undef XX
};

//TODO: 改为静态成员函数
const char* HttpStatusToString(const HttpStatus& s);

bool isHttpStatus(int status);

class HttpResponse {
public:
    class CaseInsensitiveLess {
    public:
        bool operator()(const std::string& lhs, const std::string& rhs) const {
            return strcasecmp(lhs.c_str(), rhs.c_str()) < 0;
        }
    };
    
    typedef std::shared_ptr<HttpResponse> SPtr;
    typedef std::map<std::string, std::string,
                     CaseInsensitiveLess> HeaderMap;
    HttpResponse(uint8_t version = 0x11, bool keepAlive = false);

    HttpStatus getStatus() const { return status_; }
    int8_t getVersion() const { return version_; }
    const std::string& getBody() const { return body_; }
    const std::string& getReason() const { return reason_; }
    const HeaderMap& getHeaders() const { return headers_; }

    void setStatus(HttpStatus status) { status_ = status; }
    void setVersion(uint8_t version) { version_ = version; }
    void setBody(const std::string& body) { body_ = body; }
    void setReason(const std::string& reason) { reason_ = reason; }
    void setHeaders(const HeaderMap& headers) { headers_ = headers; }

    bool isKeepAlive() { return keepAlive_; }
    void setKeepAlive(bool v) { keepAlive_ = v; }

    std::string getHeader(const std::string& key);
    void addHeader(const std::string& key, const std::string& val);
    void delHeader(const std::string& key);

    std::ostream& dump(std::ostream& os) const;
    std::string toString();

private:
    HttpStatus status_;
    uint8_t version_;
    bool keepAlive_;

    std::string body_;
    std::string reason_;
    HeaderMap headers_;

};


} // namespace reyao