
#ifndef LOG_H
#define LOG_H

#include <bits/types/FILE.h>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include <string.h>
#include <sys/time.h>
#include <stdarg.h> //提供了C标准库中的宏va_start, va_end等，用于处理可变参数函数。
#include <assert.h>
#include <sys/stat.h>       //linux文件系统头文件

#include "../buffer/buffer.h"
#include "./blockqueue.h"

class Log{
public:
    void init(int level = 1, const char* path = "./log",
                const char* suffix = ".log",
                int maxQueueCapacity = 1024);
    static Log* Instance();     //单例模式
    static void FlushLogThread();

    void write(int level, const char *format, ...);
    void flush();

    int GetLevel();
    void SetLevel(int level);
    bool IsOpen()   {   return isOpen_; }
private:
    Log();
    void AppendLogLevelTitle_(int level);
    virtual ~Log();     //TODO
    void AsyncWrite_();

private:
    static const int LOG_PATH_LEN = 256;
    static const int LOG_NAME_LEN = 256;
    static const int MAX_LINES = 50000;

    const char* path_;
    const char* suffix_;

    int MAX_LINES_;

    int lineCount_;
    int toDay_;
    
    bool isOpen_;

    Buffer buff_;
    int level_;
    bool isAsync_;

    FILE* fp_;
    std::unique_ptr<BlockDeque<std::string>> deque_;
    std::unique_ptr<std::thread> writeThread_;
    std::mutex mtx_;
};

#define LOG_BASE(level, format, ...) \
    do {\
        Log* log = Log::Instance();\
        if (log->IsOpen() && log->GetLevel() <= level) {\
            log->write(level, format, ##__VA_ARGS__); \
            log->flush();\
        }\
    } while(0);

//TODO
#define LOG_DEBUG(format, ...) do {LOG_BASE(0, format, ##__VA_ARGS__)} while(0);
#define LOG_INFO(format, ...) do {LOG_BASE(1, format, ##__VA_ARGS__)} while(0);
#define LOG_WARN(format, ...) do {LOG_BASE(2, format, ##__VA_ARGS__)} while(0);
#define LOG_ERROR(format, ...) do {LOG_BASE(3, format, ##__VA_ARGS__)} while(0);

#endif