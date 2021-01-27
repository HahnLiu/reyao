#include "reyao/rpc/codec.h"
#include "reyao/bytearray.h"
#include "reyao/endian.h"
#include "reyao/socket_stream.h"

#include <zlib.h>
#include <string>
#include <assert.h>

namespace reyao {

namespace rpc {

// message对象序列化
void ProtobufCodec::serializeToByteArray(ByteArray& ba, MessageSPtr msg) {

    const std::string& type_name = msg->GetTypeName();
    int32_t name_len = static_cast<int32_t>(type_name.size() + 1);
    ba.writeInt32(name_len);
    
    ba.write(type_name.c_str(), type_name.size());
    ba.write("\0", 1);

    int byte_size = msg->ByteSizeLong();
    uint8_t * buf = reinterpret_cast<uint8_t*>(ba.getWriteArea(byte_size));
    msg->SerializeWithCachedSizesToArray(buf);
    ba.setWritePos(ba.getWritePos() + byte_size);

    int32_t check_sum = static_cast<int32_t>(adler32(1, reinterpret_cast<const Bytef*>(ba.peek()),
                                                     static_cast<int>(ba.getReadSize())));
    ba.writeInt32(check_sum);
    assert(ba.getReadSize() == sizeof(name_len) + name_len + byte_size + sizeof(check_sum));
    int32_t len = byteSwapOnLittleEndian(static_cast<int32_t>(ba.getReadSize()));
    ba.writePrepend(&len, sizeof(len));
} 


void ProtobufCodec::send(MessageSPtr msg) {
    ByteArray ba;
    serializeToByteArray(ba, msg);
    ss_.write(&ba);
}

ProtobufCodec::ErrMsg::SPtr ProtobufCodec::receive(MessageSPtr& msg) {
    ByteArray ba;
    while (ss_.read(&ba, 4096) > 0) {
        if (ba.getReadSize() > kHeaderLen + kMinMessageLen) {
            const int32_t len = ba.readInt32();
            if (len > kMaxMessageLen || len < kMinMessageLen) {
                return std::make_shared<ErrMsg>(ErrorCode::kInvalidLength, "invalid length len= " + std::to_string(len));
            } else if (ba.getReadSize() >= static_cast<size_t>(len)) {
                auto err_msg = std::make_shared<ErrMsg>(ErrorCode::kNoError, "no error");
                msg = Parse(ba, len, err_msg);
                return err_msg;
            }
        }
    }
    return std::make_shared<ErrMsg>(ErrorCode::kServerClosed, "closed by peer");
}

// 在只有类型名没有对象类型的情况下获得该类型的default_instance
// 并通过其创建该类型的对象
// 如 string payload，根据payload生成一个string对象
google::protobuf::Message* ProtobufCodec::CreateMessage(const std::string& type_name) {
    google::protobuf::Message* msg = nullptr;
    // 根据类型名获得其Descriptor
    const google::protobuf::Descriptor* descriptor
        = google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(type_name);
    
    if (descriptor) {
        // 根据Descriptor获取default_instance
        const google::protobuf::Message* prototype 
            = google::protobuf::MessageFactory::generated_factory()->GetPrototype(descriptor);
        if (prototype) {
            // 根据default_instance创建一个相同类型的对象实例
            msg = prototype->New();
        }
    }

    return msg;
}

int32_t asInt32(const char* buf) {
	int32_t be32 = 0;
	::memcpy(&be32, buf, sizeof(be32));
	return byteSwapOnLittleEndian(be32);
}

MessageSPtr ProtobufCodec::Parse(ByteArray& ba, int len, ErrMsg::SPtr err_msg) {
    MessageSPtr msg;
    int32_t expect_checksum = asInt32(ba.peek() + len - kHeaderLen);
    int32_t checksum = static_cast<int32_t>(::adler32(1, reinterpret_cast<const Bytef*>(ba.peek()), 
                                             static_cast<int>(len - kHeaderLen)));
    
    if (expect_checksum != checksum) {
        err_msg->errcode = ErrorCode::kChecksumError;
        err_msg->errstr = "checksum error checksum=" + std::to_string(checksum) + 
                          "expected_checksum=" + std::to_string(expect_checksum);
        return msg;
    }

    int32_t name_len = ba.readInt32();
    if (name_len < 2 || name_len > len - 2 * kHeaderLen) {
        err_msg->errcode = ErrorCode::kInvalidNameLength;
        err_msg->errstr = "invalid name length = " + std::to_string(name_len);
        return msg;
    }
    std::string name = std::string(ba.peek(), name_len - 1);
    ba.setReadPos(ba.getReadPos() + name_len - 1);
    if (static_cast<char>(ba.readInt8()) != '\0') {
        err_msg->errcode = ErrorCode::kParseError;
        err_msg->errstr = "invalid type name";
        return msg;
    }
    msg.reset(CreateMessage(name));
    if (!msg) {
        err_msg->errcode = ErrorCode::kUnknownMessageType;
        err_msg->errstr = "type name = " + name;
        return msg;
    }

    const char* payload = ba.peek();
    int32_t payload_len = len - 2 * kHeaderLen - name_len;
    if (!msg->ParseFromArray(payload, payload_len)) {
        err_msg->errcode = ErrorCode::kParseError;
        err_msg->errstr = "message " + name + " parse error";
        return msg;
    }

    return msg;
} 

} //namespace rpc

} //namespace reyao
