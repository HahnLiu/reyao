#include "reyao/uri.h"

#include <iostream>

using namespace reyao;

void test(const std::string& uri_str) {
    auto uri = Uri::Create(uri_str);
    std::cout << uri->toString() << "\n";
    std::cout << uri->getAddr()->toString() << "\n";
}

int main(int argc, char** argv) {

    std::string uri = 
    "http://www.baidu.com/中文";
    test(uri);

    uri = "https://www.baidu.com/path/?id=xxx&pw=xxx#xxx";
    test(uri);
    
    return 0;
}