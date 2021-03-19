#include "reyao/socket.h"
#include "reyao/log.h"
#include "reyao/nocopyable.h"
#include "reyao/address.h"
#include "reyao/scheduler.h"

#include <memory>
#include <functional>

namespace reyao {

class TcpServer : public NoCopyable {
public:
    TcpServer(Scheduler* sche, 
              IPv4Address::SPtr addr,
              const std::string& name = "TcpServer");
    virtual ~TcpServer();

    virtual void listenAndAccpet();
    virtual void start();

    Scheduler* getScheduler() const { return sche_; }
    std::string getName() const { return name_; }
    void setName(const std::string& name) { name_ = name; }
    bool isStop() const { return !running_; }
    uint64_t getRecvTimeout() const { return recvTimeout_; }
    void setRecvTimeout(uint64_t timeout) { recvTimeout_ = timeout; }

protected:
    virtual void handleClient(Socket::SPtr client);
    virtual void accept();

private:
    Scheduler* sche_;
    IPv4Address::SPtr addr_;
    std::string name_;
    bool running_;
    Socket::SPtr listenSock_;
    uint64_t recvTimeout_;
};


} // namespace reyao
