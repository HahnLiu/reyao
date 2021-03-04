#include "reyao/rpc/rpc_server.h"
#include "reyao/log.h"

#include "echo.pb.h"

using namespace reyao;
using namespace reyao::rpc;
using namespace echo;

MessageSPtr Echo(std::shared_ptr<EchoRequest> req) {
    LOG_INFO << "server receive msg: " << req->msg();
    std::shared_ptr<echo::EchoResponse> rsp(new EchoResponse);
    rsp->set_msg(req->msg());
    return rsp;
}


int main(int argc, char** argv) {
    Scheduler sche(0);
    sche.startAsync();
    auto addr = IPv4Address::CreateAddress("0.0.0.0", 9000);
    RpcServer server(&sche, addr);
    server.registerRpcHandler<EchoRequest>(Echo);

    server.start();
    sche.wait();

    return 0;
}