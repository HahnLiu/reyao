#pragma once

#include "reyao/address.h"
#include "nocopyable.h"

#include <memory>
#include <string>
#include <sstream>

namespace reyao {

class Uri : public NoCopyable {
public:
    typedef std::shared_ptr<Uri> SPtr;
    Uri() = default;

    static Uri::SPtr Create(const std::string& uri);

    const std::string& getScheme() const { return scheme_; }
    const std::string& getUserifo() const { return userifo_; }
    const std::string& getHost() const { return host_; }
    uint16_t getPort() const { return port_;}
    const std::string& getPath() const;
    const std::string& getQuery() const { return query_; }
    const std::string& getFragment() const { return fragment_; }
    IPv4Address::SPtr getAddr() const;

    void setScheme(const std::string& scheme) { scheme_ = scheme; }
    void setUserifo(const std::string& userifo) { userifo_ = userifo; }
    void setHost(const std::string& host) { host_ = host; }
    void setPort(int32_t port) { port_ = port;}
    void setPath(const std::string& path) { path_ = path; }
    void setQuery(const std::string& query) { query_ = query; }
    void setFragment(const std::string& fragment) { fragment_ = fragment; }

    std::string toString() const;

private:
    char* parse(const char* begin,const char* end, const char& c);

private:
    std::string scheme_;
    std::string userifo_;
    std::string host_;
    uint16_t port_ = 0;
    std::string path_;
    std::string query_;
    std::string fragment_;

};


} //namespace reyao