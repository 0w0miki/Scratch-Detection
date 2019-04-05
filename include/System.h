#include <iostream>
#include <memory>
#include <sstream>
#include <queue>
#include <functional>

#include "json/json.h"
#include "http_server.h"
#include "utils.h"
#include "Camera.h"
#include "Detector.h"
#include "http_client.h"
#include "Serial.h"

// struct thread_param
// {
//     Json::Value* root;
//     pthread_mutex_t* mutex;
//     std::string host;
//     std::string url;
//     unsigned int port;
// };

class System
{

struct WorkInfo
{
    int64_t id;
    int64_t count;
    std::vector<std::string> origin_name;
};


private:
    // 批次信息
    std::deque<std::string>     work_name_list_;
    std::deque<int64_t>         work_count_list_;
    std::deque<std::string>     batch_origin_list_;
    std::deque<int64_t>         batch_count_list_;

    std::deque<WorkInfo>        work_list_;

    std::queue<string> unsolved_list_;

    // 服务器
    std::string client_host_;
    int         server_port_;
    int         client_port_;
    std::string result_url_;

    // 当前作业信息
    int64_t cur_id_;        // 当前的JobId
    int64_t cur_page_;      // 当前所在页
    int64_t cur_index_;     // 当前的列表下标

    // 线程
    pthread_t post_thread_id_ = 3;
	pthread_t server_thread_id_ = 4;
    pthread_mutex_t slv_list_mutex_;
    pthread_mutex_t result_mutex_;
    
    Json::Value* result_root_;

    Serial result_serial_;
    Camera* camera;
    Detector* detector;
    std::shared_ptr<HttpServer> http_server_;

    int readNetParam();

    int downloadFileList(std::vector<string> file_list);

    Json::Value getJsonWorkList();

    void post_result();
    void startServer();
    
    static void* post_result_wraper(void *arg);
    static void* startServer_wraper(void *arg);
    bool handle_signal(std::string url, std::string body, mg_connection *c, OnRspCallback rsp_callback);
    bool handle_result(std::string url, std::string body, mg_connection *c, OnRspCallback rsp_callback);
    bool handle_refresh(std::string url, std::string body, mg_connection *c, OnRspCallback rsp_callback);
    bool handle_start(std::string url, std::string body, mg_connection *c, OnRspCallback rsp_callback);

    void updateState();                                             // 主线程每个运行周期更新当前页数和当前检测作业在列表中的位置
    void popWorkList();                                             // 丢弃过久的列表元素
    int  setCurWork(int index, int64_t jobid, int64_t curpage);     // 修改当前作业
    void onChangeWorkList();                                        // 当列表发生变化时进行处理,通知网页

public:
    System(/* args */);
    ~System();

    void init();
    void run();
    
    int getCurInfo(std::string filename, int64_t &id);

};

