#include "reyao/rpc/codec.h"
#include "reyao/bytearray.h"
#include "reyao/endian.h"

#include <zlib.h> // asInt32

#include <string>

namespace reyao {
namespace rpc {

void ProtobufCodec::send(const MessageSPtr& message) {
    ByteArray ba;

    const std::string& type = message->GetTypeName();
    int32_t name_len = static_cast<int32_t>(type.size() + 1);
    ba.writeInt32(name_len);
    ba.write(type.c_str(), name_len); // end with '\0'

    size_t byte_size = message->ByteSizeLong();
    uint8_t* buf = reinterpret_cast<uint8_t*>(ba.getWriteArea(byte_size));
    message->SerializeWithCachedSizesToArray(buf);
    ba.setWritePos(ba.getWritePos() + byte_size);

    int32_t check_sum = static_cast<int32_t>(
        adler32(1, 
                reinterpret_cast<const Bytef*>(ba.peek()),
                static_cast<int>(ba.getReadSize())));
    ba.writeInt32(check_sum);
    assert(ba.getReadSize() == 2 * kHeaderLen + name_len + byte_size);
    int32_t len = byteSwapOnLittleEndian(ba.getReadSize());
    ba.writePrepend(&len, sizeof(len));

    ss_.write(&ba, ba.getReadSize());
}

ErrorMsg::SPtr ProtobufCodec::receive(MessageSPtr& message) {
    ByteArray ba;
    // while (ss_.read(&ba, 1024) > 0) {
    //     if (ba.getReadSize() >= kHeaderLen + kMinMessageLen) {
    //         // read byte_len + kMinMessageLen
    //         const int32_t len = ba.readInt32();
    //         if (len > kMaxMessageLen || len < kMinMessageLen) {
    //             return std::make_shared<ErrorMsg>(ErrorMsg::ErrorCode::InvalidLength, 
    //                                               "invalid length");
    //         } else if (ba.getReadSize() >= static_cast<size_t>(len + kHeaderLen)) {
    //             auto err = std::make_shared<ErrorMsg>(ErrorMsg::ErrorCode::NoError,
    //                                                   "no error");
    //             message = Parse(ba.peek() + kHeaderLen, len, err);
    //             return err;
    //         }
    //     }
    // }
    // return std::make_shared<ErrorMsg>(ErrorMsg::ErrorCode::ServerClosed, "server closed");
    bool received_len = false;
    int32_t len = 0;
    while (ss_.read(&ba, 1024) > 0) {
        if (!received_len) {
            if (ba.getReadSize() >= kHeaderLen) {
                len = ba.readInt32();
                received_len = true;
            }
        } else {
            if (len > kMaxMessageLen || len < kMinMessageLen) {
                return std::make_shared<ErrorMsg>(ErrorMsg::ErrorCode::InvalidLength, 
                                                  "invalid length");
            } else if (ba.getReadSize() == len) {
                ErrorMsg::SPtr err_msg(new ErrorMsg(ErrorMsg::ErrorCode::NoError, 
                                       "no error"));
                ProtobufCodec::Parse(ba.peek(), len, err_msg);
                return err_msg;
            }
            // ba.getReadSize() < len, need more data
        }
    }
    return std::make_shared<ErrorMsg>(ErrorMsg::ErrorCode::PeerClosed, 
                                      "peer closed");
}

int32_t asInt32(const char* buf) {
	int32_t be32 = 0;
	::memcpy(&be32, buf, sizeof(be32));
	return byteSwapOnLittleEndian(be32);
}

MessageSPtr ProtobufCodec::CreateMessage(const std::string& type) {
    // see more in muduo/examples/protobuf/code/codec.cc (chapter 7.5 in chenshuo's book)
    using namespace google::protobuf;
    MessageSPtr msg = nullptr;
    const Descriptor* descriptor = 
        DescriptorPool::generated_pool()->FindMessageTypeByName(type);
    
    if (descriptor) {
        const Message* prototype = 
            MessageFactory::generated_factory()->GetPrototype(descriptor);
        if (prototype) {
            msg.reset(prototype->New());
        }
    }

    return msg;
}

MessageSPtr ProtobufCodec::Parse(const char* buf, int len, ErrorMsg::SPtr err) {
    MessageSPtr msg;
    int32_t expected_checksum = asInt32(buf + len - kHeaderLen);
    int32_t checksum = static_cast<int32_t>(
        adler32(1, 
                reinterpret_cast<const Bytef*>(buf), 
                static_cast<int>(len - kHeaderLen)));

    if (checksum != expected_checksum) {
        err->eno = ErrorMsg::ErrorCode::ChecksumError;
        err->estr = "checksum error checksum=" + std::to_string(checksum) + 
                    "expected_checksum=" + std::to_string(expected_checksum);
        return msg;
    }

    int32_t name_len = asInt32(buf);
    if (name_len >= 2 && name_len <= len - 2 * kHeaderLen) {
        std::string type(buf + kHeaderLen, buf + kHeaderLen + name_len - 1);

        msg = CreateMessage(type);
        if (!msg) {
            err->eno = ErrorMsg::ErrorCode::UnknownMessageType;
            err->estr = "unknow message type";
            return msg;
        }

        const char* data = buf + kHeaderLen + name_len;
        int32_t data_len = len - name_len - 2 * kHeaderLen;
        if (!msg->ParseFromArray(data, data_len)) {
            err->eno = ErrorMsg::ErrorCode::ParseError;
            err->estr = "parse error";
            return msg;
        }
    } else {
        err->eno = ErrorMsg::ErrorCode::InvalidNameLength;
        err->estr = "invalid name length=" + std::to_string(name_len);
    }
    return msg;
}

} // namespace rpc

} // namespace reyao