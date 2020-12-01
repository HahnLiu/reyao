#pragma once

#include "reyao/rpc/codec.h"
#include "reyao/nocopyable.h"
#include "reyao/log.h"
#include "reyao/scheduler.h"
#include "reyao/socket.h"
#include "reyao/address.h"

namespace reyao {
namespace rpc {

using namespace google::protobuf;

//
// find out callback belongs to which type
//
template<class T>
class TypeTraits {
    static_assert(std::is_base_of<Message, T>::value,
                  "T must be a sub-class of google::protobuf::Message");
public:
    typedef std::function<void(std::shared_ptr<T>)> MessageCallBack;
};

class RpcClient : public NoCopyable {
public:
    typedef std::shared_ptr<RpcClient> SPtr;
    RpcClient(const IPv4Address& server_addr, Scheduler* scheduler);

    template<class T>
    inline void call(MessageSPtr request, const typename TypeTraits<T>::MessageCallBack& callback) {
        scheduler_->addTask(std::bind(&RpcClient::sendRequire, this, request, callback));
    }
private:
    template<class T>
    void sendRequire(MessageSPtr req, typename TypeTraits<T>::MessageCallBack cb) {
        Socket conn(SOCK_STREAM);
        conn.socket();
        if (!conn.connect(serv_addr_)) {
            LOG_ERROR << "RpcClient sendRequire, cannot conn: " << serv_addr_.toString() 
                      << " error: " << strerror(errno);
        } else {
            LOG_ERROR << "RpcClient sendRequire, conn: " << serv_addr_.toString();
        }

        ProtobufCodec codec(conn);
        codec.send(req);

        ByteArray ba;
        MessageSPtr rsp;
        auto err_msg = codec.receive(rsp);
        if (err_msg->eno != ErrorMsg::ErrorCode::NoError || !rsp) {
            LOG_ERROR << "RpcClient sendRequire, receive RpcRequest error";
        }
        std::shared_ptr<T> msg = std::static_pointer_cast<T>(rsp); // down-cast
        cb(msg);

        conn.close();
    }
    const IPv4Address& serv_addr_;
    Scheduler* scheduler_;
};

} //namespace rpc

} //namespace reyao