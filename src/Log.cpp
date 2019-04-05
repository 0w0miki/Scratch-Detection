#include "Log.hpp"

/**
 * @brief 快速计算tm类型的日期，避免使用localtime_r的时间开销
 * @ref   https://www.cnblogs.com/westfly/p/5139645.html
 * 
 * @param unix_sec      timeval格式时间
 * @param tm            目标要转过去的日期数据
 * @param time_zone     时区
 * @return int          return 0
 */
static int FastSecondToDate(const timeval &unix_sec, struct tm *tm, int time_zone)
{
    static const int kHoursInDay = 24;
    static const int kMinutesInHour = 60;
    static const int kDaysFromUnixTime = 2472632;
    static const int kDaysFromYear = 153;
    static const int kMagicUnkonwnFirst = 146097;
    static const int kMagicUnkonwnSec = 1461;
    tm->tm_sec  =  unix_sec.tv_sec % kMinutesInHour;
    int i      = (unix_sec.tv_sec/kMinutesInHour);
    tm->tm_min  = i % kMinutesInHour; //nn
    i /= kMinutesInHour;
    tm->tm_hour = (i + time_zone) % kHoursInDay; // hh
    tm->tm_mday = (i + time_zone) / kHoursInDay;
    int a = tm->tm_mday + kDaysFromUnixTime;
    int b = (a*4  + 3)/kMagicUnkonwnFirst;
    int c = (-b*kMagicUnkonwnFirst)/4 + a;
    int d =((c*4 + 3) / kMagicUnkonwnSec);
    int e = -d * kMagicUnkonwnSec;
    e = e/4 + c;
    int m = (5*e + 2)/kDaysFromYear;
    tm->tm_mday = -(kDaysFromYear * m + 2)/5 + e + 1;
    tm->tm_mon = (-m/10)*12 + m + 2;
    tm->tm_year = b*100 + d  - 4800 + (m/10); // 修改为4800才对
    return 0;
}

Logger::Logger():
logfile_(NULL),
log_dir_("../log"),
log_name_("log"),
log_id_(1),
run_flag_(true)
{

}

Logger::~Logger(){
    if (logfile_ != nullptr) {
        while(!msg_queue_.empty()){    
            string msg = msg_queue_.front();
            fprintf(logfile_,"%s",msg.c_str());
            fflush(logfile_);
            msg_queue_.pop_front();
        }        
        fclose(logfile_);
        logfile_ = nullptr;
    }
}

Logger* Logger::getLogInstance(){
    static Logger instance;
    return &instance;
}

void Logger::logError(const char* fmt, ...){
    va_list args;
    va_start(args, fmt);
	char buff[1024];
	int ret = vsnprintf(buff, 1023 , fmt , args);
	va_end(args);

    if(log_level_ <= ERROR){
        printf("[ERROR]%s\n",buff);
        string str = buff;
        this->log( ERROR, str );
    }
}

void Logger::logWarn(const char* fmt, ...){
    va_list args;
    va_start(args, fmt);
	char buff[1024];
	int ret = vsnprintf(buff, 1023 , fmt , args);
	va_end(args);

    if(log_level_ <= WARN){
        printf("[WARN]%s\n",buff);
        string str = buff;
        this->log( WARN, str );
    }
}

void Logger::logDebug(const char* fmt, ...){
    va_list args;
    va_start(args, fmt);
	char buff[1024];
	int ret = vsnprintf(buff, 1023 , fmt , args);
	va_end(args);

    if(log_level_ <= DEBUG){
        printf("[DEBUG]%s\n",buff);
        string str = buff;
        this->log( DEBUG, str );
    }
}

void Logger::logInfo(const char* fmt, ...){
    va_list args;
    va_start(args, fmt);
	char buff[1024];
	int ret = vsnprintf(buff, 1023 , fmt , args);
	va_end(args);

    
    if(log_level_ <= INFO){
        printf("[INFO]%s\n",buff);
        string str = buff;
        this->log( INFO, str );
    }
}

/**
 * @brief 记录日志主体
 * 
 * @param str 
 * @param type 记录类型
 */
void Logger::log(LOG_TYPE type, string str){
    if(nullptr == logfile_)
        return;
    pthread_mutex_lock(&mutex_);
    string msg = "";
    // time
    tm cur_time_tm;
    timeval cur_unix_sec;
    gettimeofday(&cur_unix_sec, NULL);
    FastSecondToDate(cur_unix_sec, &cur_time_tm, 8);
    msg += "[" + to_string(cur_time_tm.tm_year) + "-"
            + to_string(cur_time_tm.tm_mon+1) + "-"
            + to_string(cur_time_tm.tm_mday) + " "
            + to_string(cur_time_tm.tm_hour) + ":"
            + to_string(cur_time_tm.tm_min) + ":"
            + to_string(cur_time_tm.tm_sec) + ":"
            + to_string((cur_unix_sec.tv_usec/1000)%1000)
            + "]";
    // update date
    if(cur_time_tm.tm_mday != cday_){
        cday_ = cur_time_tm.tm_mday;
        cmon_ = cur_time_tm.tm_mon;
        cyear_ = cur_time_tm.tm_year;
    }

    // type
    switch (type)
    {
        case ERROR:
            msg += "[ERROR]";
            break;
        case WARN:
            msg += "[WARN]";
            break;
        case DEBUG:
            msg += "[DEBUG]";
            break;
        case INFO:
            msg += "[INFO]";
            break;
        default:
            break;
    }
    // push into queue
    msg += str;
    msg += "\n";
    msg_queue_.push_back(msg);
    pthread_mutex_unlock(&mutex_);
}

// Thread
void* Logger::logThreadWrapper(void* args){
    Logger::getLogInstance()->run();
}

void Logger::run(){
    while(run_flag_){
        // wait for 0.01ms
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 10 * 1000;
        select(0,NULL,NULL,NULL,&tv);

        if(msg_queue_.empty()){
            continue;
        }
        if(!checkFile()){
            continue;
        }
        string msg = msg_queue_.front();
        fprintf(logfile_,"%s",msg.c_str());
        fflush(logfile_);
        msg_queue_.pop_front();
    }
    
}

void Logger::openNewFile(){
    char log_path[1024] = {};
    sprintf(log_path, "%s/%s%d%02d%02d_%d.log", log_dir_.c_str(), log_name_.c_str(), year_, mon_, day_, log_id_);
    logfile_ = fopen(log_path,"a");
}

/**
 * @brief 判断文件是不是对(日期,大小等)并更新文件指针
 * 
 * @return true     文件成功打开
 * @return false    文件打开失败
 */
bool Logger::checkFile(){
    if(nullptr == logfile_){
        log_id_ = 1;
        openNewFile();
    }
    else if(day_ != cday_)
    {
        fclose(logfile_);
        log_id_ = 1;
        openNewFile();
    }
    else if( ftell(logfile_) >= MEM_LIMIT )
    {
        fclose(logfile_);
        log_id_ += 1;
        openNewFile();
    }
    return nullptr != logfile_;
}


/**
 * @brief 初始化日志,设置路径主文件名和记录等级
 * 
 * @param dir   路径
 * @param name  文件名
 * @param level 等级,默认为0,输出所有日志
 */
void Logger::init(const char* dir, const char* name, int level){
    log_dir_ = dir;
    log_name_ = name;
    log_level_ = level;

    pthread_mutex_init(&mutex_, NULL);

    // update time
    tm cur_time_tm;
    timeval cur_unix_sec;
    gettimeofday(&cur_unix_sec, NULL);
    FastSecondToDate(cur_unix_sec, &cur_time_tm, 8);

    year_ = cur_time_tm.tm_year; mon_ = cur_time_tm.tm_mon+1; day_= cur_time_tm.tm_mday;
    cyear_ = year_; cmon_ = mon_; cday_= day_;

    openNewFile();
    if(nullptr == logfile_){
        printf("[FATAL] Failed to open log file!!!!\n");
        return;
    }
    pthread_create(&threadId_, NULL, logThreadWrapper, NULL);
}