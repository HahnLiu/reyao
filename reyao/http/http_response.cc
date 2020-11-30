#include "reyao/http/http_response.h"


namespace reyao {


//TODO: 优化以下方法
const char* HttpStatusToString(const HttpStatus& s) {
    switch (s) {
#define XX(code, name, string) \
        case HttpStatus::name: \
            return #string;
        HTTP_STATUS_MAP(XX);
#undef XX 
    default:
        return "<unknown>";
    }
}

bool isHttpStatus(int status) {
    switch (status) {
#define XX(code, name, string) \
        case code: \
            return true;
        HTTP_STATUS_MAP(XX);
#undef XX
    }
    return false;
}

HttpResponse::HttpResponse(uint8_t version, bool keep_alive)
    : status_(HttpStatus::OK),
      version_(version),
      keep_alive_(keep_alive) {

}

std::string HttpResponse::getHeader(const std::string& key) {
    auto it = headers_.find(key);
    return it == headers_.end() ? "header not found" : it->second;
}


void HttpResponse::addHeader(const std::string& key, const std::string& val) {
    headers_[key] = val;
}

void HttpResponse::delHeader(const std::string& key) {
    headers_.erase(key);
}

std::ostream& HttpResponse::dump(std::ostream& os) const {
    //响应行
    os << "HTTP/" << ((uint32_t)(version_ >> 4))
       << "." << ((uint32_t)(version_ & 0x0F))
       << " " << (uint32_t)status_ << " "
       << (reason_.empty() ? HttpStatusToString(status_) 
          : reason_) << "\r\n";
    //首部行+主体
    os << "Connection: " << (keep_alive_ ? 
          "Keep-Alive" : "Close") << "\r\n";
    for (auto& it : headers_) {
        if (strcasecmp(it.first.c_str(), "connection") == 0) {
            continue;
        }
        os << it.first << ": " << it.second << "\r\n";
    }
    if (body_.empty()) {
        os << "\r\n";
    } else { //FIXME: add chunked
        os << "Content-Length: " << body_.size()
           << "\r\n\r\n" << body_;
    }
    return os;
}

std::string HttpResponse::toString() {
    std::stringstream ss;
    dump(ss);
    return ss.str();
}


} //namespace reyao