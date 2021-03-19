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
    : writePos_(kCheapPrepend),
      readPos_(kCheapPrepend),
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

void ByteArray::writeFloat(float value) {
    uint32_t v;
    memcpy(&v, &value, sizeof(value));
    writeInt32(v);
}

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

void ByteArray::reset() {
    readPos_ = 0;
    writePos_ = 0;
    buf_.resize(kInitSize);
}
 
void ByteArray::write(const void* buf, size_t size) {
    if (size == 0) {
        return;
    }
    addCapacity(size);

    memcpy(&buf_[writePos_], buf, size);
    writePos_ += size;

}

void ByteArray::read(void* buf, size_t size) {
    if (size > getReadSize()) {
        throw std::out_of_range("have no enough data to read");
    }

    memcpy(buf, &buf_[readPos_], size);
    readPos_ += size;
}

char* ByteArray::getWriteArea(size_t len) {
    addCapacity(len);
    return &buf_[writePos_];
}

const char* ByteArray::getReadArea(size_t* len) const {
    *len = *len > getReadSize() ? getReadSize() : *len;

    return &buf_[readPos_];
}

bool ByteArray::writeToFile(const std::string& name) const {
    std::ofstream ofs;
    ofs.open(name, std::ios::binary | std::ios::trunc);
    if (!ofs) {
        LOG_DEBUG << "ByteArray::writeToFile(" << name << ")"
                  << " errro=" << strerror(errno);
        return false;
    }
    ofs.write(&buf_[readPos_], getReadSize());
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
    const char* crlf = std::search(&buf_[readPos_], 
                                   &buf_[writePos_],
                                   kCRLF, kCRLF + 2);
    return crlf == &buf_[writePos_] ? nullptr : crlf;
}

void ByteArray::addCapacity(int size) {
    if (getWriteSize() + getReadPos() < size + kCheapPrepend) {
        buf_.resize(writePos_ + size);
    } else {
        assert(kCheapPrepend <= readPos_);
        size_t read_size = getReadSize();
        std::copy(buf_.begin() + readPos_,
                  buf_.begin() + writePos_,
                  buf_.begin() + kCheapPrepend);
        readPos_ = kCheapPrepend;
        writePos_ = readPos_ + read_size;
    }
}

void ByteArray::writePrepend(const void* data, size_t len) {
    assert(len <= getReadPos());
    readPos_ -= len;
    const char* buf = static_cast<const char*>(data);
    std::copy(buf, buf + len, buf_.begin() + readPos_);
}

} // namespace reyao
