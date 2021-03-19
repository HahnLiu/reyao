#pragma once

#include "reyao/tcp_client.h"
#include "reyao/socket.h"
#include "reyao/socket_stream.h"

#include <google/protobuf/message.h>

namespace reyao {

namespace rpc {

typedef std::shared_ptr<::google::protobuf::Message> MessageSPtr;

/*
 Rpc Protobuf 数据格式：
 len: 数据包长度
 name_len: 类型名长度
 name: 类型名
 payload: 数据
 checksum: 校验和
*/
class ProtobufCodec {
public:
    enum ErrorCode {
        kNoError = 0,
        kInvalidLength,
        kChecksumError,
        kInvalidNameLength,
        kUnknownMessageType,
        kParseError,
        kServerClosed,
    };

    struct ErrMsg {
        typedef std::shared_ptr<ErrMsg> SPtr;
        ErrMsg(ErrorCode code, const std::string& str)
            : errcode(code), errstr(str) {}

        ErrorCode errcode;
        std::string errstr;
    };


public:
    ProtobufCodec(Socket::SPtr conn): ss_(conn) {}
    ~ProtobufCodec() {}

    void send(MessageSPtr msg);
    ErrMsg::SPtr receive(MessageSPtr& msg);

private:
    void serializeToByteArray(ByteArray& ba, MessageSPtr msg);

    static google::protobuf::Message* CreateMessage(const std::string& typeName);
    static MessageSPtr Parse(ByteArray& ba, int len, ErrMsg::SPtr errMsg);


    const static int kHeaderLen = sizeof(int32_t);
    // MessageHeader: nameLen + typeName + Checksum
    const static int kMinMessageLen = kHeaderLen + 2 + kHeaderLen; 
    const static int kMaxMessageLen = 64 * 1024 * 1024;

    SocketStream ss_;
};

} // namespace rpc

} // namespace reyao 


