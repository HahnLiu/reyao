#include "reyao/rpc/rpc_client.h"

namespace reyao {
namespace rpc {

RpcClient::RpcClient(const IPv4Address& server_addr, Scheduler* scheduler)
    : conn_(new Socket(SOCK_STREAM)),
      scheduler_(scheduler) {
    conn_->socket();
}

} //namespace rpc

} //namespace reyao