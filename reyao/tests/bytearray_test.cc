#include "reyao/bytearray.h"
#include "reyao/log.h"

#include <stdlib.h>
#include <assert.h>

#include <vector>

using namespace reyao;

void test1() {
#define XX(type, len, write_func, read_func) { \
    std::vector<type> vec; \
    for (int i = 0; i < len; i++) { \
        vec.push_back(rand()); \
    } \
    ByteArray b; \
    for (auto& i : vec)  { \
        b.write_func(i); \
    } \
    for (size_t i = 0; i < vec.size(); i++) { \
        type v = b.read_func(); \
        assert(v == vec[i]); \
    } \
    assert(b.getReadSize() == 0); \
    LOG_INFO << #write_func "/" #read_func "(" #type ") len=" << len; \
} 

    XX(int8_t, 100, writeInt8, readInt8);
    XX(uint8_t, 100, writeUint8, readUint8);
    XX(int16_t, 100, writeInt16, readInt16);
    XX(uint16_t, 100, writeUint16, readUint16);
    XX(int32_t, 100, writeInt32, readInt32);
    XX(uint32_t, 100, writeUint32, readUint32);
    XX(int64_t, 100, writeInt64, readInt64);
    XX(uint64_t, 100, writeUint64, readUint64);

    XX(int32_t, 100, writeVarInt32, readVarInt32);
    XX(uint32_t, 100, writeVarUint32, readVarUint32);
    XX(int64_t, 100, writeVarInt64, readVarInt64);
    XX(uint64_t, 100, writeVarUint64, readVarUint64);
#undef XX
}


void test2() {
#define XX(type, len, write_func, read_func) { \
    std::vector<type> vec; \
    for (int i = 0; i < len; i++) { \
        vec.push_back(rand()); \
    } \
    ByteArray b; \
    for (auto& i : vec)  { \
        b.write_func(i); \
        LOG_INFO << "write type:" << #type " :" << (uint64_t)i; \
    } \
    for (size_t i = 0; i < vec.size(); i++) { \
        type v = b.read_func(); \
        assert(v == vec[i]); \
        LOG_INFO << "read type:" << #type " :" << (uint64_t)v; \
    } \
}

    XX(int8_t, 10, writeInt8, readInt8);
    XX(uint8_t, 10, writeUint8, readUint8);
    XX(int16_t, 10, writeInt16, readInt16);
    XX(uint16_t, 10, writeUint16, readUint16);
    XX(int32_t, 10, writeInt32, readInt32);
    XX(uint32_t, 10, writeUint32, readUint32);
    XX(int64_t, 10, writeInt64, readInt64);
    XX(uint64_t, 10, writeUint64, readUint64);

    XX(int32_t, 10, writeVarInt32, readVarInt32);
    XX(uint32_t, 10, writeVarUint32, readVarUint32);
    XX(int64_t, 10, writeVarInt64, readVarInt64);
    XX(uint64_t, 10, writeVarUint64, readVarUint64);
#undef XX   
}

int main(int argc, char** argv) {
    test1();
    // test2();
    return 0;
}