#include "reyao/rpc/rpc_client.h"
#include "reyao/log.h"

#include "echo.pb.h"

using namespace reyao;
using namespace reyao::rpc;
using namespace echo;

int main(int argc, char** argv) {
    Scheduler sche(0);
    IPv4Address addr("0.0.0.0", 9000);
    RpcClient client(&sche, addr);
    

    std::shared_ptr<EchoRequest> req(new EchoRequest);
    req->set_msg("helloServer!");
    sche.addTimer(3 * 1000, [&, req]() {
        client.Call<EchoResponse>(req, [](std::shared_ptr<EchoResponse> rsp) {
            LOG_INFO << rsp->msg();
        });
    }, true);
    sche.start();
    return 0;
}