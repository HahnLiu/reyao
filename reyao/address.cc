#include "reyao/address.h"
#include "reyao/log.h"
#include "reyao/endian.h"

#include <netdb.h>
#include <ifaddrs.h>
#include <stddef.h>

namespace reyao {


IPv4Address::IPv4Address() {
    memset(&addr_, 0, sizeof(addr_));
    addr_.sin_family = AF_INET;
}

IPv4Address::IPv4Address(const sockaddr_in& addr):addr_(addr) {

}

IPv4Address::IPv4Address(const char* addr, uint16_t port) {
    addr_.sin_family = AF_INET;
    addr_.sin_port = byteSwapOnLittleEndian(port);
    if (inet_pton(AF_INET, addr, &addr_.sin_addr) == -1) {
        LOG_ERROR << "inet_pton(AF_INET, " << addr << "," << port 
                  << ") " << "error=" << strerror(errno);
    }
}

uint16_t IPv4Address::getPort() const {
    return byteSwapOnLittleEndian(addr_.sin_port);
}

void IPv4Address::setPort(uint16_t port) {
    addr_.sin_port = byteSwapOnLittleEndian(port);
}

const sockaddr* IPv4Address::getAddr() const {
    return (sockaddr*)&addr_;
}

sockaddr* IPv4Address::getAddr() {
    return (sockaddr*)&addr_;
}

socklen_t IPv4Address::getAddrLen() const {
    return sizeof(addr_);
}

std::string IPv4Address::toString() const {
    std::stringstream ss;
    uint32_t addr = byteSwapOnLittleEndian(addr_.sin_addr.s_addr);
    ss << ((addr >> 24) & 0xff) << "."
       << ((addr >> 16) & 0xff) << "."
       << ((addr >> 8) & 0xff) << "."
       << (addr & 0xff) << ":" << getPort();
    return ss.str();
}

sockaddr_in IPv4Address::GetHostByName(const char* hostname, uint16_t port) {
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = byteSwapOnLittleEndian(port);
    hostent* host = gethostbyname(hostname);
    if (host== nullptr) {
        LOG_ERROR << "gethostbyname(" << hostname << ")" 
                  << "err " << strerror(errno);
        return addr;
    }
    addr.sin_addr = *(in_addr*)(*host->h_addr_list);
    return addr;
}

IPv4Address::SPtr IPv4Address::CreateByName(const char* hostname, uint16_t port) {
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = byteSwapOnLittleEndian(port);
    hostent* host = gethostbyname(hostname);
    if (host== nullptr) {
        LOG_ERROR << "gethostbyname(" << hostname << ")" 
                  << " host not fount";
        return std::make_shared<IPv4Address>("0.0.0.0", 0);
    }
    addr.sin_addr = *(in_addr*)(*host->h_addr_list);
    return std::make_shared<IPv4Address>(addr);
}

IPv4Address::SPtr IPv4Address::CreateAddress(const char* addr, uint16_t port) {
    IPv4Address::SPtr ret(new IPv4Address(addr, port));
    return ret;
}


} //namepsace reyao