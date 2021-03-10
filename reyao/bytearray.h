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
    ByteArray(size_t init_size = kInitSize);
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

    // 获取可以写入的内存
    // const char* getWriteArea(size_t len);
    char* getWriteArea(size_t len);
    //只读数据，读取到 iovec 结构
    const char* getReadArea(size_t* len) const;

    void reset();
    void write(const void* buf, size_t size);
    void read(void* buf, size_t size);
    //把可读数据全部取出写到文件中
    bool writeToFile(const std::string& name) const;
    //把文件的数据全部放进bytearray储存
    bool readFromFile(const std::string& name);
    //以std::string格式输出可读数据，不取出数据
    //TODO:tostring已改为取数据
    std::string toString();
    std::string toHexString();
    //获取可以写入的内存

    void setWritePos(size_t pos) { write_pos_ = pos; }
    size_t getWritePos() const { return write_pos_; }
    void setReadPos(size_t pos) { read_pos_ = pos; }
    size_t getReadPos() const { return read_pos_; }

    const char* peek() const { return &buf_[0] + read_pos_; }
    const char* findCRLF() const;

    int getEndian() const { return endian_; }
    void setEndian(int endian) { endian_ = endian; }

    size_t getPrependSize() const { return read_pos_; }
    size_t getReadSize() const { return write_pos_ - read_pos_; }
    size_t getWriteSize() const { return buf_.size() - write_pos_; }
    size_t getCapacity() const { return buf_.size(); }

    void writePrepend(const void* data, size_t len);

private:
    void addCapacity(int size);

    int endian_ = BIG_ENDIAN;
    size_t write_pos_;
    size_t read_pos_;
    std::vector<char> buf_;
};

} //namespace reyao
