#include "reyao/util.h"
#include "reyao/log.h"

#include <sys/time.h>
#include <fstream>
#include <sstream>

namespace reyao {

int64_t GetCurrentMs() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

int HexToDec(std::string hex) {
    size_t res = 0;
    for (size_t i = 0; i < hex.size(); i++) {
        char c = hex[i];
        if (c >= '0' && c <= '9') {
            res = res * 16 + c - '0';
        } else if (toupper(c) >= 'A' && toupper(c) <= 'F') {
            res = res * 16 + toupper(c) - 'A' + 10;
        } else {
            LOG_ERROR << "HexToDec error hex" << hex; 
            return -1;
        }
    }
    return res;
}

std::string ReadFile(const std::string& pathname) {
    std::ifstream in(pathname, std::ios::in);
    if (!in.is_open()) {
        LOG_ERROR << "open " << pathname << "error: " << strerror(errno);
        return "";
    }
    std::stringstream buf;
    std::string line;
    while (getline(in, line)) {
        buf << line;
    }
    in.close();
    return buf.str();
}

} // namespace reyao