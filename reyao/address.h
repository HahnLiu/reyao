#pragma once

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <memory>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <iostream>

namespace reyao {

class IPv4Address {
public:
    typedef std::shared_ptr<IPv4Address> SPtr;
    IPv4Address();
    explicit IPv4Address(const sockaddr_in& addr);
    IPv4Address(const char* addr, uint16_t port);

    uint16_t getPort() const;
    void setPort(uint16_t port);
    const sockaddr* getAddr() const;
    sockaddr* getAddr();
    socklen_t getAddrLen() const;
    std::string toString() const;

    static sockaddr_in GetHostByName(const char* hostname, uint16_t port = 0);
    static IPv4Address::SPtr CreateByName(const char* hostname, uint16_t port = 0);


private:
    sockaddr_in addr_;
};


} //namespace reyao