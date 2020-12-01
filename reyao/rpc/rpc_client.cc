#include "reyao/rpc/rpc_client.h"

namespace reyao {
namespace rpc {

RpcClient::RpcClient(const IPv4Address& server_addr, Scheduler* scheduler)
    : serv_addr_(server_addr),
      scheduler_(scheduler) {}

} //namespace rpc

} //namespace reyao