#pragma once

#include "reyao/socket.h"
#include "reyao/socket_stream.h"

#include <google/protobuf/message.h>

namespace reyao {
namespace rpc {

typedef std::shared_ptr<google::protobuf::Message> MessageSPtr;

// TODO: 优化ErrorMsg

struct ErrorMsg {
    typedef std::shared_ptr<ErrorMsg> SPtr;
    enum class ErrorCode {
            NoError,
            InvalidLength,
            ChecksumError,
            InvalidNameLength,
            UnknownMessageType,
            ParseError,
            ServerClosed
        };

    ErrorMsg(ErrorCode err, std::string str) 
        : eno(err),
          estr(str) {}
          
    ErrorCode eno;
    std::string estr;
};

class ProtobufCodec {
public:
    ProtobufCodec(Socket::SPtr client): ss_(client, false) {}

    void send(const MessageSPtr& message);
    ErrorMsg::SPtr receive(MessageSPtr& message);

private:
    static google::protobuf::Message* CreateMessage(const std::string& type);
    static MessageSPtr Parse(const char* buf, int len, ErrorMsg::SPtr err);

    const static int kHeaderLen = sizeof(int32_t);
    const static int kMinMessageLen = kHeaderLen + 2 + kHeaderLen; //namelen + typename + checksum
    const static int kMaxMessageLen = 64 * 1024 * 1024;

    SocketStream ss_;
};

} //namespace rpc

} //namespace reyao