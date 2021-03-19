#pragma once
#include "reyao/http/http_session.h"

#include <memory>
#include <vector>

namespace reyao {

class HttpRequest;
class HttpResponse;

class HttpParser {
public:
    enum ParseState {
        PARSE_FIRST_LINE,
        PARSE_HEADER,
        PARSE_BODY
    };

    enum ParseChunkState {
        PARSE_CHUNK_SIZE,
        PARSE_CHUNK_CONTENT
    };

    typedef std::shared_ptr<HttpParser> SPtr;
    explicit HttpParser(SocketStream* stream);
    virtual ~HttpParser() {}


protected:

    virtual void parseFirstLine(const char* start, const char* end) = 0;
    virtual void parseHeader(const char* start, const char* end) = 0;
    virtual void parseChunkedBody() = 0;
    virtual void parseFixedBody() = 0;

protected:
    SocketStream* stream_;
    ByteArray ba_;
    size_t readSize_ = 0;

    ParseState parseState_ = PARSE_FIRST_LINE;
    bool error_ = false;
    bool finish_ = false;
    std::string body_;

    size_t contentLen_;
    bool chunked_ = false;
    size_t chunkSize_ = 0;
    ParseChunkState chunkState_ = PARSE_CHUNK_SIZE;
};

class HttpRequestParser : public HttpParser {
public:
    HttpRequestParser(SocketStream* stream, HttpRequest* req);
    
    bool parseRequest();

protected:
    virtual void parseFirstLine(const char* start, const char* end) override;
    virtual void parseHeader(const char* start, const char* end) override;
    virtual void parseChunkedBody() override;
    virtual void parseFixedBody() override;

private:
    const char* parseURI(const char* begin, const char* end, const char& des);

private:
    HttpRequest* req_;
};


class HttpResponseParser : public HttpParser {
public:
    HttpResponseParser(SocketStream* stream, HttpResponse* rsp);

    bool parseResponse();

protected:
    virtual void parseFirstLine(const char* start, const char* end) override;
    virtual void parseHeader(const char* start, const char* end) override;
    virtual void parseChunkedBody() override;
    virtual void parseFixedBody() override;

private:
    HttpResponse* rsp_;
};

} // namespace reyao