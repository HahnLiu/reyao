#pragma once

#include "timestamp.h"
#include "log_msg.h"

#include <unistd.h>
#include <functional>
#include <memory>

namespace qlog {

static const char* kDefaultLogFormat = "%time %tid %cid %file[%line]:%func%t ";

class LogFormatter {
public:
    static LogFormatter* GetInstance() {
        static LogFormatter formatter;
        return &formatter;
    }
    static void SetFormat(const std::string& fmt) {
        LogFormatter::GetInstance()->setFormat(fmt);
    }

    class FmtItem { 
    public:
        typedef std::shared_ptr<FmtItem> SPtr;
        explicit FmtItem(const std::string& str = "") {}
        virtual ~FmtItem() {}
        virtual void format(LogMsg& msg) = 0;
    };

    // NOTE: no thread-safe.
    void setFormat(const std::string& fmt) {
        pattern_ = fmt;
        init();
    }
    
    const std::vector<FmtItem::SPtr> getFormatItem() const { return items_; }
    
private:
    LogFormatter(const std::string& pattern = kDefaultLogFormat): pattern_(pattern) {
        init();
    }
    ~LogFormatter() = default;

    void init();

private:
    std::string pattern_;
    std::vector<FmtItem::SPtr> items_;  
};

struct InitFormat {
    InitFormat() {
        LogFormatter::GetInstance();
    }
};

static InitFormat s_initFormat;

} // namespace qlog