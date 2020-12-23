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
    TcpServer(Scheduler* scheduler, 
              const IPv4Address& addr,
              const std::string& name = "tcp_server");
    virtual ~TcpServer();

    virtual void listenAndAccpet();
    virtual void start();

    std::string getName() const { return name_; }
    void setName(const std::string& name) { name_ = name; }
    bool isStop() const { return stopped_; }
    uint64_t getRecvTimeout() const { return recv_timeout_; }
    void setRecvTimeout(uint64_t timeout) { recv_timeout_ = timeout; }

protected:
    virtual void handleClient(Socket::SPtr client);
    virtual void accept();

private:
    Scheduler* scheduler_;
    const IPv4Address& addr_;
    std::string name_;
    bool stopped_;
    Socket::SPtr listen_sock_;
    uint64_t recv_timeout_;
};


} //namespace reyao
