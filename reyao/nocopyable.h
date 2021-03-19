#pragma once

namespace reyao {

class NoCopyable {
public:
    NoCopyable() = default;
    ~NoCopyable() = default;
    NoCopyable(const NoCopyable&) = delete;
    NoCopyable& operator=(const NoCopyable&) = delete;
};

} // namespace reyao