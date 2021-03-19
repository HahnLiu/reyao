#include "reyao/rpc/rpc_client.h"
#include "reyao/log.h"

#include "echo.pb.h"

using namespace reyao;
using namespace reyao::rpc;
using namespace echo;

int main(int argc, char** argv) {
    g_logger->setLevel(LogLevel::INFO);
    Scheduler sche;
    auto addr = IPv4Address::CreateAddress("0.0.0.0", 30000);
    RpcClient client(&sche, addr);
    

    std::shared_ptr<EchoRequest> req(new EchoRequest);
    req->set_msg("helloServer!");
    sche.addTimer(3 * 1000, [&, req]() {
        client.Call<EchoResponse>(req, [](std::shared_ptr<EchoResponse> rsp) {
            LOG_INFO << rsp->msg();
        });
    }, true);
    sche.startAsync();

    sche.wait();
    return 0;
}