#pragma once

#include "reyao/rpc/codec.h"
#include "reyao/log.h"
#include "reyao/nocopyable.h"
#include "reyao/tcp_client.h"
#include "reyao/scheduler.h"


namespace reyao {

namespace rpc {


template<typename RspMessage>
class TypeTraits {
    static_assert(std::is_base_of<::google::protobuf::Message, RspMessage>::value, 
                  "T must be subclass of google::protobuf::Message");

public:
    typedef std::function<void(std::shared_ptr<RspMessage>)> ResponseHandler;
};

class RpcClient : public NoCopyable {
public:
    typedef std::shared_ptr<RpcClient> SPtr;
    RpcClient(Scheduler* sche, IPv4Address::SPtr addr): client_(sche, addr) {}

    template<typename RspMessage>
    void Call(MessageSPtr req, typename TypeTraits<RspMessage>::ResponseHandler handler) {
        client_.setConnectCallBack([this, req, handler](Socket::SPtr conn) {
            // LOG_DEBUG << "RpcClient connect to " << client_.getConn()->getPeerAddr()->toString();
            ProtobufCodec codec(conn);
            codec.send(req);

            ByteArray ba;
            MessageSPtr rsp = nullptr;
            auto errMsg = codec.receive(rsp);
            if (errMsg->errcode == ProtobufCodec::kNoError && rsp) {
                std::shared_ptr<RspMessage> concreateRsp
                    = std::static_pointer_cast<RspMessage>(rsp);
                assert(handler);
                assert(concreateRsp); 
                handler(concreateRsp);
            } else {
                LOG_ERROR << "receive response error: " << errMsg->errstr;
            }
            conn->close();
        });
        client_.start();
    }

private:
    TcpClient client_;
};

} // namespace rpc

} // namespace reyao
