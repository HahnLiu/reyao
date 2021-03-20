#pragma once

namespace reyao {

template <typename T>
class Singleton {
public:
    static T* GetInstance() {
        static T instance;
        return &instance;
    }
    
private:
    Singleton() {}
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
};

} // namespace reyao