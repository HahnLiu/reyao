#include "reyao/rpc/rpc_server.h"
#include "reyao/log.h"

namespace reyao {
namespace rpc {

void RpcServer::handleClient(Socket::SPtr client) {
    ProtobufCodec codec(client);

    MessageSPtr msg;
    auto err = codec.receive(msg);
    if (err->eno != ErrorMsg::ErrorCode::NoError || !msg) {
        LOG_ERROR << "RpcServer codec.recevie(msg) error: " << err->estr;
        client->close();
        return;
    }
    bool is_register = false;
    HandlerMap::const_iterator it;
    const google::protobuf::Descriptor* descriptor = msg->GetDescriptor();
    {
        MutexGuard lock(mutex_);
        it = handlers_.find(descriptor);
        is_register = it != handlers_.end();
    }

    if (!is_register) {
        LOG_ERROR << "RpcServer::handleClient, unknown message";
        client->close();
        return;
    }

    MessageSPtr rsp = it->second->onMessage(msg);
    codec.send(rsp);

    client->close();
}

} //namespace rpc

} //namespace reyao