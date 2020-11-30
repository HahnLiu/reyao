#include "reyao/http/http_servlet.h"

#include "fnmatch.h"

namespace reyao {

Servlet::Servlet(const std::string& name)
    : name_(name) {

}

FunctionServlet::FunctionServlet(CallBackFunc func)
    : Servlet("FunctionServlet"), 
      func_(func) {

}

int32_t FunctionServlet::handle(const HttpRequest& req,
                                HttpResponse* rsp,
                                const HttpSession& session) {
    return func_(req, rsp, session);
}

ServletDispatch::ServletDispatch()
    : Servlet("ServletDispatch") {
    default_.reset(new NoFoundServlet("Reyao"));
}

int32_t ServletDispatch::handle(const HttpRequest& req,
                                HttpResponse* rsp,
                                const HttpSession& session) {
    auto servlet = getMatchServlet(req.getPath());
    if (servlet) {
        servlet->handle(req, rsp, session);
    }
    return 0;
}

void ServletDispatch::addServlet(const std::string& uri, Servlet::SPtr servlet) {
    WriteLock lock(rw_lock_);
    servlets_[uri] = servlet;
}

void ServletDispatch::addServlet(const std::string& uri, FunctionServlet::CallBackFunc func) {
    WriteLock lock(rw_lock_);
    servlets_[uri].reset(new FunctionServlet(func));
}

void ServletDispatch::addGlobalServlet(const std::string& uri, Servlet::SPtr servlet) {
    WriteLock lock(rw_lock_);
    for (auto it = globals_.begin(); it != globals_.end(); it++) {
        if (it->first == uri) {
            globals_.erase(it);
        }
    }
    globals_.push_back(std::make_pair(uri, servlet));
}

void ServletDispatch::addGlobalServlet(const std::string& uri, FunctionServlet::CallBackFunc func) {
    addGlobalServlet(uri, FunctionServlet::SPtr(new FunctionServlet(func)));
}

void ServletDispatch::delServlet(const std::string& uri) {
    WriteLock lock(rw_lock_);
    servlets_.erase(uri);
}

void ServletDispatch::delGlobalServlet(const std::string& uri) {
    WriteLock lock(rw_lock_);
    for (auto it = globals_.begin(); it != globals_.end(); it++) {
        if (it->first == uri) {
            globals_.erase(it);
            break;
        }
    }
}

Servlet::SPtr ServletDispatch::getServlet(const std::string& uri) {
    ReadLock lock(rw_lock_);
    auto it = servlets_.find(uri);
    return it == servlets_.end() ? nullptr : it->second;
}

Servlet::SPtr ServletDispatch::getGlobalServlet(const std::string& uri) {
    ReadLock lock(rw_lock_);
    for (auto it = globals_.begin(); it != globals_.end(); it++) {
        if (!fnmatch(it->first.c_str(), uri.c_str(), 0)) {
            return it->second;
        }
    }
    return nullptr;
}

Servlet::SPtr ServletDispatch::getMatchServlet(const std::string& uri) {
    ReadLock lock(rw_lock_);
    auto it = servlets_.find(uri);
    if (it != servlets_.end()) {
        return it->second;
    }
    for (auto it = globals_.begin(); it != globals_.end(); it++) {
        if (!fnmatch(it->first.c_str(), uri.c_str(), 0)) {
            return it->second;
        }
    }
    return default_;
}

NoFoundServlet::NoFoundServlet(const std::string& server_name)
    : Servlet("NoFoundServlet") {
    content_ = "<html><head><title>404 Not Found"
        "</title></head><body><center><h1>404 Not Found</h1></center>"
        "<hr><center>" + server_name + "</center></body></html>";
}

int32_t NoFoundServlet::handle(const HttpRequest& req,
                               HttpResponse* rsp,
                               const HttpSession& session) {
    rsp->setStatus(HttpStatus::NOT_FOUND);
    rsp->addHeader("Server", "Reyao");
    rsp->addHeader("Content-Type", "text/html");
    rsp->setBody(content_);
    return 0;
}

} //namespace reyao