#pragma once

#include <sys/types.h>

#include <string>

namespace reyao {

int64_t GetCurrentMs();

int HexToDec(std::string hex);

std::string ReadFile(const std::string& pathname);

} // namespace reyao