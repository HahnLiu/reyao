#include "reyao/http/http_request.h"


namespace reyao {

HttpMethod StringToHttpMethod(const std::string& str) {
    const char* s = str.c_str();
#define XX(num, name, string) \
    if (strcmp(#string, s) == 0) { \
        return HttpMethod::name; \
    }
    HTTP_METHOD_MAP(XX);
#undef XX
    return HttpMethod::INVALID_METHOD;
}

static const char* s_method_string[] = {
#define XX(num, name, string) #string,
    HTTP_METHOD_MAP(XX)
#undef XX
};

const char* HttpMethodToString(const HttpMethod& m) {
    uint32_t method = (uint32_t)m;
    if (method >= 4) {
        return "<unknown>";
    }
    return s_method_string[method - 1];
}


HttpRequest::HttpRequest(uint8_t version, bool keepAlive) 
    : method_(HttpMethod::GET),
      version_(version),
      keepAlive_(keepAlive),
      path_("/") {

}

std::string HttpRequest::getHeader(const std::string& key) {
    auto it = headers_.find(key);
    return it == headers_.end() ? "header not found" : it->second;
}

std::string HttpRequest::getParam(const std::string& key) {
    auto it = params_.find(key);
    return it == params_.end() ? "param not found" : it->second;
}

std::string HttpRequest::getCookie(const std::string& key) {
    auto it = cookies_.find(key);
    return it == cookies_.end() ? "cookie not found" : it->second;
}

void HttpRequest::addHeader(const std::string& key, const std::string& val) {
    headers_[key] = val;
}

void HttpRequest::addParam(const std::string& key, const std::string& val) {
    params_[key] = val;
}

void HttpRequest::addCookie(const std::string& key, const std::string& val) {
    cookies_[key] = val;
}

void HttpRequest::delHeader(const std::string& key) {
    headers_.erase(key);
}

void HttpRequest::delParam(const std::string& key) {
    params_.erase(key);
}

void HttpRequest::delCookie(const std::string& key) {
    cookies_.erase(key);
}

std::ostream& HttpRequest::dump(std::ostream& os) const {
    os << HttpMethodToString(method_) << " "
       << path_ << (query_.empty() ? "" : "?")
       << query_ << (fragment_.empty() ? "" : "#")
       << fragment_ << " HTTP/" << (uint32_t)(version_ >> 4)
       << "." << (uint32_t)(version_ & 0x0F) << "\r\n"; 

    os << "connection: " << (keepAlive_ ? "keep-alive" : "close") << "\r\n";
    for (auto& it : headers_) {
        if (strcasecmp(it.first.c_str(), "connection") == 0) {
            continue;
        }
        os << it.first << ": " << it.second << "\r\n";
    }

    if (body_.empty()) {
        os << "\r\n";
    } else {
        os << "content-length: " << body_.size()
           << "\r\n\r\n" << body_;
    }
    return os;
}

std::string HttpRequest::toString() {
    std::stringstream ss;
    dump(ss);
    return ss.str();
}


} // namespace reyao