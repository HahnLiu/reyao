#pragma once

#include "reyao/endian.h"

#include <sys/uio.h>

#include <string>
#include <vector>
#include <memory>

namespace reyao {

class ByteArray {
public:

    typedef std::shared_ptr<ByteArray> SPtr;
    typedef std::unique_ptr<ByteArray> UPtr;
    static const size_t kInitSize = 1024;
    static const size_t kCheapPrepend = 8;
    static const char kCRLF[];
    ByteArray(size_t initSize = kInitSize);
    ~ByteArray() = default;

    void writeInt8(int8_t value);
    void writeUint8(uint8_t value);
    void writeInt16(int16_t value);
    void writeUint16(uint16_t value);
    void writeInt32(int32_t value);
    void writeUint32(uint32_t value);
    void writeInt64(int64_t value);
    void writeUint64(uint64_t value);
    void writeFloat(float value);
    void writeDouble(double value);
    void writeString(const std::string& value);
    void writeString16(const std::string& value);
    void writeString32(const std::string& value);
    void writeString64(const std::string& value);


    int8_t readInt8();
    uint8_t readUint8();
    int16_t readInt16();
    uint16_t readUint16();
    int32_t readInt32();
    uint32_t readUint32();
    int64_t readInt64();
    uint64_t readUint64();
    float readFloat();
    double readDouble();
    std::string readString16();
    std::string readString32();
    std::string readString64();


    char* getWriteArea(size_t len);
    const char* getReadArea(size_t* len) const;

    void reset();
    void write(const void* buf, size_t size);
    void read(void* buf, size_t size);
    bool writeToFile(const std::string& name) const;
    bool readFromFile(const std::string& name);
    std::string toString();
    std::string toHexString();

    void setWritePos(size_t pos) { writePos_ = pos; }
    size_t getWritePos() const { return writePos_; }
    void setReadPos(size_t pos) { readPos_ = pos; }
    size_t getReadPos() const { return readPos_; }

    const char* peek() const { return &buf_[0] + readPos_; }
    const char* findCRLF() const;

    int getEndian() const { return endian_; }
    void setEndian(int endian) { endian_ = endian; }

    size_t getReadSize() const { return writePos_ - readPos_; }
    size_t getWriteSize() const { return buf_.size() - writePos_; }
    size_t getCapacity() const { return buf_.size(); }

    void writePrepend(const void* data, size_t len);

private:
    void addCapacity(int size);

    int endian_ = BIG_ENDIAN;
    size_t writePos_;
    size_t readPos_;
    std::vector<char> buf_;
};

} // namespace reyao
