#pragma once

#include "reyao/rpc/codec.h"
#include "reyao/log.h"
#include "reyao/nocopyable.h"
#include "reyao/tcp_client.h"
#include "reyao/scheduler.h"


namespace reyao {

namespace rpc {


template<typename T>
class TypeTraits {
    static_assert(std::is_base_of<::google::protobuf::Message, T>::value, 
                  "T must be subclass of google::protobuf::Message");

public:
    typedef std::function<void(std::shared_ptr<T>)> ResponseHandler;
};

class RpcClient : public NoCopyable {
public:
    typedef std::shared_ptr<RpcClient> SPtr;
    RpcClient(Scheduler* sche, IPv4Address::SPtr addr): client_(sche, addr) {}

    template<typename T>
    bool Call(MessageSPtr req, typename TypeTraits<T>::ResponseHandler handler) {
        client_.setConnectCallBack([=](Socket::SPtr conn) {
            LOG_DEBUG << "RpcClient connect to " << client_.getConn()->getPeerAddr()->toString();
            ProtobufCodec codec(conn);
            codec.send(req);

            ByteArray ba;
            MessageSPtr rsp;
            auto err_msg = codec.receive(rsp);
            if (err_msg->errcode == ProtobufCodec::kNoError && rsp) {
                std::shared_ptr<T> concreate_rsp
                    = std::static_pointer_cast<T>(rsp);
                handler(concreate_rsp);
            } else {
                LOG_ERROR << "receive response error: " << err_msg->errstr;
            }
            conn->close();
        });
        return client_.start();
    }

private:
    TcpClient client_;
};

} //namespace rpc

} //namespace reyao
