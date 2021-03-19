#include "reyao/rpc/codec.h"
#include "reyao/bytearray.h"
#include "reyao/endian.h"
#include "reyao/socket_stream.h"

#include <zlib.h>
#include <string>
#include <assert.h>

namespace reyao {

namespace rpc {

// message 对象序列化
void ProtobufCodec::serializeToByteArray(ByteArray& ba, MessageSPtr msg) {

    const std::string& typeName = msg->GetTypeName();
    int32_t nameLen = static_cast<int32_t>(typeName.size() + 1);
    ba.writeInt32(nameLen);
    
    ba.write(typeName.c_str(), typeName.size());
    ba.write("\0", 1);

    int byteSize = msg->ByteSizeLong();
    uint8_t * buf = reinterpret_cast<uint8_t*>(ba.getWriteArea(byteSize));
    msg->SerializeWithCachedSizesToArray(buf);
    ba.setWritePos(ba.getWritePos() + byteSize);

    int32_t checkSum = static_cast<int32_t>(adler32(1, reinterpret_cast<const Bytef*>(ba.peek()),
                                                    static_cast<int>(ba.getReadSize())));
    ba.writeInt32(checkSum);
    assert(ba.getReadSize() == sizeof(nameLen) + nameLen + byteSize + sizeof(checkSum));
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
                auto errMsg = std::make_shared<ErrMsg>(ErrorCode::kNoError, "no error");
                msg = Parse(ba, len, errMsg);
                return errMsg;
            }
        }
    }
    return std::make_shared<ErrMsg>(ErrorCode::kServerClosed, "closed by peer");
}


google::protobuf::Message* ProtobufCodec::CreateMessage(const std::string& typeName) {
    google::protobuf::Message* msg = nullptr;
    const google::protobuf::Descriptor* descriptor
        = google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(typeName);
    
    if (descriptor) {
        const google::protobuf::Message* prototype 
            = google::protobuf::MessageFactory::generated_factory()->GetPrototype(descriptor);
        if (prototype) {
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

MessageSPtr ProtobufCodec::Parse(ByteArray& ba, int len, ErrMsg::SPtr errMsg) {
    MessageSPtr msg;
    int32_t expectChecksum = asInt32(ba.peek() + len - kHeaderLen);
    int32_t checksum = static_cast<int32_t>(::adler32(1, reinterpret_cast<const Bytef*>(ba.peek()), 
                                             static_cast<int>(len - kHeaderLen)));
    
    if (expectChecksum != checksum) {
        errMsg->errcode = ErrorCode::kChecksumError;
        errMsg->errstr = "checksum error checksum=" + std::to_string(checksum) + 
                          "expected_checksum=" + std::to_string(expectChecksum);
        return msg;
    }

    int32_t nameLen = ba.readInt32();
    if (nameLen < 2 || nameLen > len - 2 * kHeaderLen) {
        errMsg->errcode = ErrorCode::kInvalidNameLength;
        errMsg->errstr = "invalid name length = " + std::to_string(nameLen);
        return msg;
    }
    std::string name = std::string(ba.peek(), nameLen - 1);
    ba.setReadPos(ba.getReadPos() + nameLen - 1);
    if (static_cast<char>(ba.readInt8()) != '\0') {
        errMsg->errcode = ErrorCode::kParseError;
        errMsg->errstr = "invalid type name";
        return msg;
    }
    msg.reset(CreateMessage(name));
    if (!msg) {
        errMsg->errcode = ErrorCode::kUnknownMessageType;
        errMsg->errstr = "type name = " + name;
        return msg;
    }

    const char* payload = ba.peek();
    int32_t payloadLen = len - 2 * kHeaderLen - nameLen;
    if (!msg->ParseFromArray(payload, payloadLen)) {
        errMsg->errcode = ErrorCode::kParseError;
        errMsg->errstr = "message " + name + " parse error";
        return msg;
    }

    return msg;
} 

} // namespace rpc

} // namespace reyao
