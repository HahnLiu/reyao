#include "reyao/uri.h"

namespace reyao {

Uri::SPtr Uri::Create(const std::string& uri) {
    size_t right = uri.size() - 1;
    Uri::SPtr ret(new Uri);
    size_t left = 0, pos = 0;
    pos = uri.find_first_of(':', left);
    if (pos == std::string::npos) {
        //scheme no found
        return nullptr;
    }
    ret->setScheme(uri.substr(left, pos - left));
    //TODO:
    if (ret->getScheme() == "http") {
        ret->setPort(80);
    }
    if (ret->getScheme() == "https") {
        ret->setPort(443);
    }
    left = pos + 3;
    if (left >= right) {
        return nullptr;
    }
    pos = uri.find_first_of("@", left);
    if (pos != std::string::npos) {
        ret->setUserifo(uri.substr(left, pos - left));
        left = pos + 1;
        if (left >= right) {
            return nullptr;
        }
    }
    //从尾部搜索# ?
    //left-- host:port/path?query#fragment --right
    pos = uri.rfind('#', right);
    if (pos != std::string::npos) {
        ret->setFragment(uri.substr(pos + 1, right - pos));
        right = pos - 1;
    }
    //left-- host:port/path?query --right
    pos = uri.rfind('?', right);
    if (pos != std::string::npos) {
        ret->setQuery(uri.substr(pos + 1, right - pos));
        right = pos - 1;
    }
    //left-- host:port/path --right
    pos = uri.find_first_of('/', left);
    if (pos != std::string::npos) {
        ret->setPath(uri.substr(pos, right - pos + 1));
        right = pos - 1;
    }
    //left-- host:port --right
    pos = uri.find_first_of(':', left);
    if (pos != std::string::npos) {
        ret->setPort(std::stoi(uri.substr(pos + 1, right - pos)));
        right = pos - 1;
    }
    ret->setHost(uri.substr(left, right - left + 1));
    return ret;
}

const std::string& Uri::getPath() const {
    static std::string s_default_path = "/";
    return path_.empty() ? s_default_path : path_;
}

IPv4Address::SPtr Uri::getAddr() const {
    return IPv4Address::CreateByName(host_.c_str(), port_);
}

std::string Uri::toString() const {
    std::stringstream ss;
    ss << scheme_ << "://"
       << userifo_ 
       << (userifo_.empty() ? "" : "@")
       << host_ 
       << ((port_ == 80 || port_ == 442) ? 
            ":" + std::to_string(port_) : "")
       << path_
       << (query_.empty() ? "" : "?")
       << query_
       << (fragment_.empty() ? "" : "#")
       << fragment_;
    return ss.str();
}

} //namespace reyao