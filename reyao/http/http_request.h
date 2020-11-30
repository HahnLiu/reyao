#pragma once 

#include <string.h>

#include <string>
#include <map>
#include <iostream>
#include <sstream>
#include <memory>

namespace reyao {


/* Request Methods */
#define HTTP_METHOD_MAP(XX)         \
  XX(1,  GET,         GET)          \
  XX(2,  HEAD,        HEAD)         \
  XX(3,  POST,        POST)         \

enum class HttpMethod {
#define XX(num, name, string) name = num,
    HTTP_METHOD_MAP(XX)
#undef XX
    INVALID_METHOD
};

HttpMethod StringToHttpMethod(const std::string& s);

const char* HttpMethodToString(const HttpMethod& m);

class HttpRequest {
public:
    class CaseInsensitiveLess {
    public:
        bool operator()(const std::string& lhs, const std::string& rhs) const {
            return strcasecmp(lhs.c_str(), rhs.c_str()) < 0;
        }
    };
    
    typedef std::shared_ptr<HttpRequest> SPtr;
    typedef std::map<std::string, std::string,
                     CaseInsensitiveLess> StrMap;
    HttpRequest(uint8_t version = 0x11, bool keep_alive = false); //TODO:

    HttpMethod getMethod() const { return method_; }
    int8_t getVersion() const { return version_; }
    bool isKeepAlive() const { return keep_alive_; }
    const std::string& getPath() const { return path_; }
    const std::string& getQuery() const { return query_; }
    const std::string& getFragment() const { return fragment_; }
    const std::string& getBody() const { return body_; }
    const StrMap& getHeaders() const { return headers_; }
    const StrMap& getParams() const { return params_; }
    const StrMap& getCookies() const { return cookies_; }

    void setMethod(HttpMethod method) { method_ = method; }
    void setVersion(uint8_t version) { version_ = version; }
    void setKeepAlive(bool v) { keep_alive_ = v; }
    void setPath(const std::string& path) { path_ = path; }
    void setQuery(const std::string& query) { query_ = query; }
    void setFragment(const std::string& fragment) { fragment_ = fragment; }
    void setBody(const std::string& body) { body_ = body; }
    void setHeaders(const StrMap& headers) { headers_ = headers; }
    void setParams(const StrMap& params) { params_ = params; }
    void setCookies(const StrMap& cookies) { cookies_ = cookies; }

    std::string getHeader(const std::string& key);
    std::string getParam(const std::string& key);
    std::string getCookie(const std::string& key);

    void addHeader(const std::string& key, const std::string& val);
    void addParam(const std::string& key, const std::string& val);
    void addCookie(const std::string& key, const std::string& val);

    void delHeader(const std::string& key);
    void delParam(const std::string& key);
    void delCookie(const std::string& key);

    std::ostream& dump(std::ostream& os) const;
    std::string toString();

private:
    HttpMethod method_;
    uint8_t version_;
    bool keep_alive_;

    std::string path_;
    std::string query_;
    std::string fragment_;
    std::string body_;

    StrMap headers_;
    StrMap params_;
    StrMap cookies_;

};


} //namespace reyao