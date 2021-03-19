#pragma once

#include "reyao/http/http_request.h"
#include "reyao/http/http_response.h"
#include "reyao/http/http_session.h"
#include "reyao/mutex.h"

#include <memory>
#include <functional>
#include <string>
#include <unordered_map>

namespace reyao {

class Servlet {
public:
    typedef std::shared_ptr<Servlet> SPtr;
    Servlet(const std::string& name);
    virtual ~Servlet() {}

    virtual int32_t handle(const HttpRequest& req,
                           HttpResponse* rsp,
                           const HttpSession& session) = 0;
    const std::string& getName() const { return name_; }

protected:
     std::string name_;
};

class FunctionServlet : public Servlet {
public:
    typedef std::shared_ptr<FunctionServlet> SPtr;
    typedef std::function<int32_t(const HttpRequest&,
                                  HttpResponse*,
                                  const HttpSession&)> CallBackFunc;

    FunctionServlet(CallBackFunc func);
    virtual int32_t handle(const HttpRequest& req,
                           HttpResponse* rsp,
                           const HttpSession& session) override;
private:
    CallBackFunc func_;
};

class ServletDispatch : public Servlet {
public:
    typedef std::shared_ptr<ServletDispatch> SPtr;
    typedef std::unordered_map<std::string, Servlet::SPtr> ServletMap;
    typedef std::vector<std::pair<std::string, Servlet::SPtr>> ServletVec;

    ServletDispatch();

    virtual int32_t handle(const HttpRequest& req,
                           HttpResponse* rsp,
                           const HttpSession& session) override;

    void addServlet(const std::string& uri, Servlet::SPtr servlet);
    void addServlet(const std::string& uri, FunctionServlet::CallBackFunc func);
    void addGlobalServlet(const std::string& uri, Servlet::SPtr servlet);
    void addGlobalServlet(const std::string& uri, FunctionServlet::CallBackFunc func);

    void delServlet(const std::string& uri);
    void delGlobalServlet(const std::string& uri);

    Servlet::SPtr getServlet(const std::string& uri);
    Servlet::SPtr getGlobalServlet(const std::string& uri);
    Servlet::SPtr getMatchServlet(const std::string& uri);

    Servlet::SPtr getDefault() const { return default_; }
    void setDefault(Servlet::SPtr s) { default_ = s; } 

private:
    //url(/xxx/xxx) --> servlet
    ServletMap servlets_;
    //url(/xxx/*) --> servlet
    ServletVec globals_;
    Servlet::SPtr default_;
    RWLock rwlock_;
};

class NoFoundServlet : public Servlet {
public:
    typedef std::shared_ptr<NoFoundServlet> SPtr;
    NoFoundServlet(const std::string& server_name);
    virtual int32_t handle(const HttpRequest& req,
                           HttpResponse* rsp,
                           const HttpSession& session) override;
private:
    std::string content_;
};


} // namespace reyao