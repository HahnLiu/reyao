#pragma once

#include <sys/types.h>

#include <string>

namespace reyao {

//当前时间的毫秒数
int64_t GetCurrentTime();

//16进制字符串转10进制整形
int HexToDec(std::string hex);

} //namespace reyao