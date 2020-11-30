#pragma once

namespace reyao {

template <typename T>
class Singleton {
public:
    static T* GetInstance() {
        static T instance; //c++11后的静态变量的创建为线程安全
        return &instance;
    }
    
private:
    Singleton();
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
};

} //namespace reyao