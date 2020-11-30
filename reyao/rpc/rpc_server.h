#pragma once

#include "reyao/tcp_server.h"
#include "reyao/rpc/codec.h"
#include "reyao/mutex.h"

#include <google/protobuf/service.h>
#include <map>
    

namespace reyao {
namespace rpc {

class RpcCallBack {
public:
    virtual ~RpcCallBack() = default;

    virtual MessageSPtr onMessage(const MessageSPtr& msg) = 0;
};

template <class T>
class RpcCallBackType : public RpcCallBack {
    static_assert(std::is_base_of<google::protobuf::Message, T>::value,
                  "T must be sub-class of google::protobuf::Message");

public:
    typedef std::function<MessageSPtr(const std::shared_ptr<T>&)> MessageCallBack;
    RpcCallBackType(const MessageCallBack& cb): cb_(cb) {}
    MessageSPtr onMessage(const MessageSPtr& msg) {
        std::shared_ptr<T> m = std::static_pointer_cast<T>(msg); // down-cast
        return cb_(m);
    }

private:
    MessageCallBack cb_;
};

class RpcServer : public TcpServer {
public:
    typedef std::map<const google::protobuf::Descriptor*, 
                     std::shared_ptr<RpcCallBack>> HandlerMap;
    RpcServer(Scheduler *Scheduler);

    void handleClient(Socket::SPtr client) override;

    template <class T>
    void registerRpcHandler(const typename RpcCallBackType<T>::MessageCallBack& handler) {
        std::shared_ptr<RpcCallBackType<T>> cb(new RpcCallBackType<T>(handler));
        MutexGuard lock(mutex_);
        handlers_[T::descriptor()] = cb;
    }

private:
    Mutex mutex_;
    HandlerMap handlers_;
};

} //namespace rpc

} //namespace reyao
