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

    const int64_t MEM_LIMIT = (1u * 1024 * 1024 * 1024);   // 1G

    Logger();
    ~Logger();
    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;

    void Time(char* buffer);                               // 获取当前时间
    void log(LOG_TYPE type, string str);                   // log本体

    bool checkFile();                                      // 校验文件是否存在, 大小是否超出*
    void openNewFile();                                    // 开新文件

    // thread
    static void* logThreadWrapper(void* args);
    void run();

    string log_dir_;                                        // 路径
    string log_name_;                                       // 文件名
    FILE *logfile_;                                         // 文件指针
    deque<string> msg_queue_;                               // 记录队列 线程每次写入队首记录
    int log_level_;                                         // 日志等级
    bool run_flag_;                                         // 运行标志位
    int log_id_;                                            // 同一天中的第几个文件
    int year_, mon_, day_;                                  // 年月日
    int cyear_, cmon_, cday_;                               // 当前年月日

    pthread_t threadId_;
    pthread_mutex_t mutex_;

public:

    static Logger* getLogInstance();                        // 内部静态对象方法

    void logError(const char* fmt, ...);
    void logDebug(const char* fmt, ...);
    void logInfo(const char* fmt, ...);
    void logWarn(const char* fmt, ...);

    inline void setLevel(int level){                        // 设置日志等级
        log_level_ = level;
    };

    void init(const char* dir, const char* name, int level = 0); // 初始化
};

#define sLog Logger::getLogInstance()                       // 宏定义取得实例指针