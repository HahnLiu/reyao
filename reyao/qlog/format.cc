#include "format.h"
#include "util.h"
#include "reyao/coroutine.h"

namespace qlog {

class LevelFmtItem : public LogFormatter::FmtItem {
 public:
    explicit LevelFmtItem(const std::string& str = "") {}
    void format(LogMsg& msg) override { 
        msg << LogLevelToString(msg.getLevel());
    }
};

class DateTimeFmtItem : public LogFormatter::FmtItem {
 public:
    explicit DateTimeFmtItem(const std::string& str = "") {}
    void format(LogMsg& msg) override {   
        msg << detail::TimeStamp::Now().format();
    }
};

class ThreadIdFmtItem : public LogFormatter::FmtItem {
 public:
    explicit ThreadIdFmtItem(const std::string& str = "") {}
        void format(LogMsg& msg) override {
        msg << detail::Gettid();
    }
};

class FileNameFmtItem : public LogFormatter::FmtItem {
 public:
    explicit FileNameFmtItem(const std::string& str = "") {}
    void format(LogMsg& msg) override {
        msg << msg.getFile();
    }
};

class FunctionFmtItem : public LogFormatter::FmtItem {
 public:
    explicit FunctionFmtItem(const std::string& str = "") {}
    void format(LogMsg& msg) override {
        msg << msg.getFunction();
    }
};

class LineFmtItem : public LogFormatter::FmtItem {
 public:
    explicit LineFmtItem(const std::string& str = "") {}
    void format(LogMsg& msg) override {
        msg << msg.getLine();
    }
};

class TableFmtItem : public LogFormatter::FmtItem {
 public:
    explicit TableFmtItem(const std::string& str = "") {}
    void format(LogMsg& msg) override {
        msg << "\t";
    }
};

class StringFmtItem : public LogFormatter::FmtItem {
 public:
    explicit StringFmtItem(const std::string& str):str_(str) {}
    void format(LogMsg& msg) override {
        msg << str_;
    }
 private:
    std::string str_;
};

class CoroutineIdFmtItem : public LogFormatter::FmtItem {
public:
    explicit CoroutineIdFmtItem(const std::string& str = "") {}
    void format(LogMsg& msg) override {
        msg << reyao::Coroutine::GetCoroutineId();
    }
};

void LogFormatter::init() {   
    items_.clear();
    std::vector<std::pair<std::string, int>> vec;
    std::string normalStr;
    for (size_t i = 0; i < pattern_.size(); i++) {
        if (pattern_[i] != '%') {
            // normal char.
            normalStr.append(1, pattern_[i]);
            continue;
        }
        
        // match %

        // %%
        if (i + 1 < pattern_.size() && pattern_[i + 1] == '%') {
            normalStr.append(1, '%');
            ++i;
            continue;
        }

        size_t n = i + 1;
        std::string fmtStr;
        while (n < pattern_.size()) {
            if (!isalpha(pattern_[n])) {
                // add format type until meet no alpha char.
                fmtStr = pattern_.substr(i + 1, n - i - 1);
                break;
            }       
            ++n;
            if (n == pattern_.size()) {
                if (fmtStr.empty()) {
                    fmtStr = pattern_.substr(i + 1);
                }
            }
        }

        if (!normalStr.empty()) {
            vec.push_back({normalStr, 0});
            normalStr.clear();
        }
        vec.push_back({fmtStr, 1});
        i = n - 1;
    }

    if (!normalStr.empty()) {
        // pattern has no % , normal_str == pattern_
        vec.push_back({normalStr, 0});
    }

    static std::map<std::string, std::function<FmtItem::SPtr()>> format2Item = {
#define XX(c, Item) \
        {#c, [] () { return FmtItem::SPtr(std::make_shared<Item>());} }

        XX(level, LevelFmtItem),
        XX(time, DateTimeFmtItem),
        XX(tid, ThreadIdFmtItem),
        XX(file, FileNameFmtItem),
        XX(func, FunctionFmtItem),
        XX(line, LineFmtItem),
        XX(t, TableFmtItem),
        XX(cid, CoroutineIdFmtItem),

#undef XX
    };

    for (auto& i : vec) {
        if (i.second == 0) {
            items_.push_back(FmtItem::SPtr(std::make_shared<StringFmtItem>(i.first)));
        } else {
            auto iter = format2Item.find(i.first);
            if (iter == format2Item.end()) {
                items_.push_back(FmtItem::SPtr(std::make_shared<StringFmtItem>("<<error format %" + i.first + ">>")));
            } else {
                items_.push_back(iter->second());
            }
        }
    }
}

} // namespace qlog