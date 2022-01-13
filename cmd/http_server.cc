#include "reyao/http/http_server.h"
#include "reyao/util.h"

#include <iostream>

using namespace reyao;

int main(int argc, char** argv) {
    g_logger->setLevel(LogLevel::WARN);
     if (argc < 3) {
         std::cerr << "usage: ./http_server thread_name port\n";
         exit(0);
     }
    int num = atoi(argv[1]);
    int port = atoi(argv[2]);
    Scheduler sh(num);
    sh.startAsync();
    auto addr = IPv4Address::CreateAddress("0.0.0.0", port);
    HttpServer server(&sh, addr, true);
    auto dispatch = server.getDispatch();
    dispatch->addServlet("/", [](const HttpRequest& req,
			    	 HttpResponse* rsp,
				 const HttpSession& session) {
	    rsp->addHeader("Server", "Reyao");
	    rsp->addHeader("Content-Type", "text/html");
        rsp->setBody(reyao::ReadFile("/home/crash/server-cfg/readme.html"));
		return 0;
    });
    dispatch->addGlobalServlet("/", [](const HttpRequest& req,
			    	 HttpResponse* rsp,
				 const HttpSession& session) {
	    rsp->addHeader("Server", "Reyao");
	    rsp->addHeader("Content-Type", "text/html");
        rsp->setBody(reyao::ReadFile("/home/crash/server-cfg/readme.html"));
		return 0;
    });

    server.start();
    sh.wait();
    return 0;
}