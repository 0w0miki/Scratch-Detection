#pragma once

#include <stdio.h>
#include <sys/time.h>
#include <sys/select.h>
#include <time.h>
#include <stdarg.h>
#include <pthread.h>
#include <string>
#include <deque>

using namespace std;


// 日志调用方法后将 时间和记录信息 推到处理队列里面 之后写入文件
class Logger
{
public:
    enum LOG_TYPE{
        ERROR,
        WARN,
        INFO,
        DEBUG
    };

private:

    const int64_t MEM_LIMIT = (1u * 1024 * 1024 * 1024); // 1G

    Logger();
    ~Logger();
    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;

    void Time(char* buffer);
    void log(LOG_TYPE type, string str);

    bool checkFile();
    void openNewFile();

    // thread
    static void* logThreadWrapper(void* args);
    void run();

    string log_dir_;
    string log_name_;
    FILE *logfile_;
    deque<string> msg_queue_;
    int log_level_;
    bool run_flag_;
    int log_id_;
    int year_, mon_, day_;
    int cyear_, cmon_, cday_;

    pthread_t threadId_;
    pthread_mutex_t mutex_;

public:

    static Logger* getLogInstance(); // 内部静态对象方法

    void logError(const char* fmt, ...);
    void logDebug(const char* fmt, ...);
    void logInfo(const char* fmt, ...);
    void logWarn(const char* fmt, ...);

    inline void setLevel(int level){
        log_level_ = level;
    };

    void init(const char* dir, const char* name, int level = 0);
};

#define sLog Logger::getLogInstance()