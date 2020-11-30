#include "reyao/bytearray.h"
#include "reyao/log.h"

#include <string.h>
#include <assert.h>

#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace reyao {

const char ByteArray::kCRLF[] = "\r\n";

ByteArray::ByteArray(size_t init_size)
    : write_pos_(kCheapPrepend),
      read_pos_(kCheapPrepend),
      buf_(kCheapPrepend + init_size) {}

void ByteArray::writeInt8(int8_t value) {
    write(&value, sizeof(value));
}

void ByteArray::writeUint8(uint8_t value) {
    write(&value, sizeof(value));
}

void ByteArray::writeInt16(int16_t value) {
    if (endian_ != BYTE_ORDER) {
        value = byteSwap(value);
    }
    write(&value, sizeof(value));
}

void ByteArray::writeUint16(uint16_t value) {
    if (endian_ != BYTE_ORDER) {
        value = byteSwap(value);
    }
    write(&value, sizeof(value));
}

void ByteArray::writeInt32(int32_t value) {
    if (endian_ != BYTE_ORDER) {
        value = byteSwap(value);
    }
    write(&value, sizeof(value));
}

void ByteArray::writeUint32(uint32_t value) {
    if (endian_ != BYTE_ORDER) {
        value = byteSwap(value);
    }
    write(&value, sizeof(value));
}

void ByteArray::writeInt64(int64_t value) {
    if (endian_ != BYTE_ORDER) {
        value = byteSwap(value);
    }
    write(&value, sizeof(value));
}

void ByteArray::writeUint64(uint64_t value) {
    if (endian_ != BYTE_ORDER) {
        value = byteSwap(value);
    }
    write(&value, sizeof(value));
}

uint32_t ByteArray::encodeZigzag32(const int32_t& value) {
    if (value < 0) {
        return ((uint32_t)(-value)) * 2 - 1;
    } else {
        return value * 2;
    }
}

uint64_t ByteArray::encodeZigzag64(const int64_t& value) {
    if (value < 0) {
        return ((uint64_t)(-value)) * 2 - 1;
    } else {
        return value * 2;
    }
}

void ByteArray::writeVarInt32(int32_t value) {
    writeVarUint32(encodeZigzag32(value));
}

void ByteArray::writeVarUint32(uint32_t value) {
    uint8_t temp[5];
    uint8_t i = 0;
    //一次取出7位，并在最高位置一构成一个字节
    //如果val大于28位，则需要取5次，最后变为5个字节
    while (value > 0x7F) {
        temp[i++] = (value & 0x7F) | 0x80;
        value >>= 7;
    }
    temp[i++] = value;
    write(temp, i);
}

void ByteArray::writeVarInt64(int64_t value) {
    writeVarUint64(encodeZigzag64(value));
}

void ByteArray::writeVarUint64(uint64_t value) {
    uint8_t temp[10];
    uint8_t i = 0;
    while (value > 0x7F) {
        temp[i++] = (value & 0x7F) | 0x80;
        value >>= 7;
    }
    temp[i++] = value;
    write(temp, i);
}

void ByteArray::writeFloat(float value) {
    uint32_t v;
    memcpy(&v, &value, sizeof(value));
    writeInt32(v);
}

//TODO:
void ByteArray::writeDouble(double value) {
    uint64_t v;
    memcpy(&v, &value, sizeof(value));
    writeInt64(v);
}

void ByteArray::writeString(const std::string& value) {
    write(value.c_str(), value.size());
}

void ByteArray::writeString16(const std::string& value) {
    writeUint16(value.size());
    write(value.c_str(), value.size());
}

void ByteArray::writeString32(const std::string& value) {
    writeUint32(value.size());
    write(value.c_str(), value.size());
}

void ByteArray::writeString64(const std::string& value) {
    writeUint64(value.size());
    write(value.c_str(), value.size());
}

void ByteArray::writeStringVarint(const std::string& value) {
    writeVarUint64(value.size());
    write(value.c_str(), value.size());
}

int8_t ByteArray::readInt8() {
    int8_t v;
    read(&v, sizeof(v));
    return v;
}

uint8_t ByteArray::readUint8() {
    uint8_t v;
    read(&v, sizeof(v));
    return v;
}

int16_t ByteArray::readInt16() {
    int16_t v;
    read(&v, sizeof(v));
    if (endian_ != BYTE_ORDER) {
        v = byteSwap(v);
    }
    return v;
}

uint16_t ByteArray::readUint16() {
    uint16_t v;
    read(&v, sizeof(v));
    if (endian_ != BYTE_ORDER) {
        v = byteSwap(v);
    }
    return v;
}

int32_t ByteArray::readInt32() {
    int32_t v;
    read(&v, sizeof(v));
    if (endian_ != BYTE_ORDER) {
        v = byteSwap(v);
    }
    return v;
}

uint32_t ByteArray::readUint32() {
    uint32_t v;
    read(&v, sizeof(v));
    if (endian_ != BYTE_ORDER) {
        v = byteSwap(v);
    }
    return v;
}

int64_t ByteArray::readInt64() {
    int64_t v;
    read(&v, sizeof(v));
    if (endian_ != BYTE_ORDER) {
        v = byteSwap(v);
    }
    return v;
}

uint64_t ByteArray::readUint64() {
    uint64_t v;
    read(&v, sizeof(v));
    if (endian_ != BYTE_ORDER) {
        v = byteSwap(v);
    }
    return v;
}

//TODO:
int32_t ByteArray::decodeZigzag32(const uint32_t& value) {
    //左移消掉*2  
    return (value >> 1) ^ -(value & 1);
}

int64_t ByteArray::decodeZigzag64(const uint64_t& value) {
    return (value >> 1) ^ -(value & 1);
}

int32_t ByteArray::readVarInt32() {
    return decodeZigzag32(readVarUint32());
}

uint32_t ByteArray::readVarUint32() {
    uint32_t result = 0;
    for (int i = 0; i < 32; i+= 7) {
        uint8_t b = readUint8();
        if (b < 0x80) {
            result |= ((uint32_t)b) << i;
            break;
        } else {
            result |= ((uint32_t) (b & 0x7f) << i);
        }
    }
    return result;
}

int64_t ByteArray::readVarInt64() {
    return decodeZigzag64(readVarUint64());
}

uint64_t ByteArray::readVarUint64() {
    uint64_t result = 0;
    for (int i = 0; i < 64; i+= 7) {
        uint8_t b = readUint8();
        if (b < 0x80) {
            result |= ((uint32_t)b) << i;
            break;
        } else {
            result |= ((uint32_t) (b & 0x7f) << i);
        }
    }
    return result;
}

float ByteArray::readFloat() {
    uint32_t v = readUint32();
    float result;
    memcpy(&result, &v, sizeof(v));
    return result;
}

double ByteArray::readDouble() {
    uint64_t v = readUint64();
    double result;
    memcpy(&result, &v, sizeof(v));
    return result;
}

std::string ByteArray::readString16() {
    uint16_t len = readUint16();
    std::string buf;
    buf.resize(len);
    read(&buf[0], len);
    return buf;
}

std::string ByteArray::readString32() {
    uint32_t len = readUint32();
    std::string buf;
    buf.resize(len);
    read(&buf[0], len);
    return buf;
}

std::string ByteArray::readString64() {
    uint64_t len = readUint64();
    std::string buf;
    buf.resize(len);
    read(&buf[0], len);
    return buf;
}

std::string ByteArray::readStringVarint() {
    uint64_t len = readUint64();
    std::string buf;
    buf.resize(len);
    read(&buf[0], len);
    return buf;
}

void ByteArray::reset() {
    read_pos_ = 0;
    write_pos_ = 0;
    buf_.resize(kInitSize);
}
 
void ByteArray::write(const void* buf, size_t size) {
    if (size == 0) {
        return;
    }
    addCapacity(size);

    memcpy(&buf_[write_pos_], buf, size);
    write_pos_ += size;

}

void ByteArray::read(void* buf, size_t size) {
    if (size > getReadSize()) {
        throw std::out_of_range("have no enough data to read");
    }

    memcpy(buf, &buf_[read_pos_], size);
    read_pos_ += size;
}

// const char* ByteArray::getWriteArea(size_t len) {
//     addCapacity(len);
//     return &buf_[write_pos_];
// }

char* ByteArray::getWriteArea(size_t len) {
    addCapacity(len);
    return &buf_[write_pos_];
}

const char* ByteArray::getReadArea(size_t* len) const {
    *len = *len > getReadSize() ? getReadSize() : *len;

    return &buf_[read_pos_];
}

// uint64_t ByteArray::getReadArea(std::vector<iovec>& bufs, 
//                          uint64_t len, size_t position) const {
//     len = len > getReadSize() ? getReadSize() : len;

//     uint64_t left = len;
//     size_t npos = position % node_size_;
//     size_t ncap = node_size_ - npos;
//     struct iovec iov;
//     int count = position / node_size_;
//     Node* cur = head_;
//     while (count > 0) {
//         cur = cur->next;
//         --count;
//     }
//     while (left > 0) {
//         if (ncap >= left) {
//             iov.iov_base = cur->ptr + npos;
//             iov.iov_len = left;
//             left = 0;
//         } else {
//             iov.iov_base = cur->ptr + npos;
//             iov.iov_len = ncap;
//             left -= ncap;
//             npos = 0;
//             ncap = node_size_;
//             cur = cur->next;
//         }
//         bufs.push_back(iov);
//     }
//     return len;
// }

bool ByteArray::writeToFile(const std::string& name) const {
    std::ofstream ofs;
    ofs.open(name, std::ios::binary | std::ios::trunc);
    if (!ofs) {
        LOG_DEBUG << "ByteArray::writeToFile(" << name << ")"
                  << " errro=" << strerror(errno);
        return false;
    }
    ofs.write(&buf_[read_pos_], getReadSize());
    return true;
}

bool ByteArray::readFromFile(const std::string& name) {
    std::ifstream ifs;
    ifs.open(name, std::ios::binary);
    if (!ifs) {
        LOG_DEBUG << "ByteArray::readFromFile(" << name << ")"
                  << " errro=" << strerror(errno);
        return false;       
    }
    std::shared_ptr<char> buf(new char[kInitSize], [](char* ptr) { delete []ptr; });
    while (!ifs.eof()) {
        ifs.read(buf.get(), kInitSize);
        write(buf.get(), ifs.gcount());
    }
    return true;
}

std::string ByteArray::toString() {
    std::string buf;
    buf.resize(getReadSize());
    if (buf.empty()) {
        return buf;
    }
    read(&buf[0], buf.size());
    return buf;
}

std::string ByteArray::toHexString() {
    std::string buf = toString();
    std::stringstream ss;
    for (size_t i = 0; i < buf.size(); i++) {
        if (i > 0 && i % 32 == 0) {
            ss << "\n";
        }
        ss << std::setw(2) << std::setfill('0') << std::hex
           << (int)(uint8_t)buf[i] << " ";
    }
    return ss.str();
}

const char* ByteArray::findCRLF() const {
    const char* crlf = std::search(&buf_[read_pos_], 
                                   &buf_[write_pos_],
                                   kCRLF, kCRLF + 2);
    return crlf == &buf_[write_pos_] ? nullptr : crlf;
}

void ByteArray::addCapacity(int size) {
    if (write_pos_ + getPrependSize() < size + kCheapPrepend) {
        buf_.resize(write_pos_ + size);
    } else {
        assert(kCheapPrepend <= read_pos_);
        size_t read_size = getReadSize();
        std::copy(buf_.begin() + read_pos_,
                  buf_.begin() + write_pos_,
                  buf_.begin() + kCheapPrepend);
        read_pos_ = kCheapPrepend;
        write_pos_ = read_pos_ + read_size;
    }
}

void ByteArray::writePrepend(const void* data, size_t len) {
    assert(len <= getPrependSize());
    read_pos_ -= len;
    const char* buf = static_cast<const char*>(data);
    std::copy(buf, buf + len, buf_.begin() + read_pos_);
}

} //namespace reyao
