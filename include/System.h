#include <iostream>
#include <memory>
#include <sstream>
#include <queue>
#include <functional>

#include "json/json.h"
#include "utils.h"
#include "Camera.h"
#include "Detector.h"
#include "Websocket_server.h"
#include "http_client.h"
#include "Serial.h"
#include "Log.hpp"

class System
{

// 记录作业信息推送给网页显示
struct WorkInfo
{
    int64_t id;
    int page;
    bool reprint_flag = false;
};

// 记录原图信息提供网页显示
struct OriginInfo{
    string location;
    int quantity;
};


private:
    // 批次信息
    std::deque<std::string>     work_name_list_;        // 作业名队列
    std::deque<int>         work_count_list_;           // 作业数队列
    std::deque<std::string>     batch_origin_list_;     // 原图地址队列
    std::deque<int>         batch_count_list_;          // 原图对应数量队列

    std::vector<WorkInfo>       work_list_;             // 给网页的作业信息表
    std::vector<OriginInfo>     origin_list_;           // 给网页的原图信息表

    std::queue<string> unsolved_list_;                  // 待处理图像队列

    // 服务器
    std::string client_host_;                           // 远端服务器host
    int         server_port_;                           // 本机端口
    int         client_port_;                           // 远端服务器端口
    std::string result_url_;                            // 远端服务器接收结果的url

    // 当前作业信息
    int64_t cur_id_;        // 当前的JobId
    int cur_page_;          // 当前所检测页
    int cur_index_;         // 当前的列表下标
    int totla_page_;        // 总页数

    // 线程
    pthread_t post_thread_id_ = 3;                      // 发送结果线程id
	pthread_t server_thread_id_ = 4;                    // 本地服务器线程id
    pthread_mutex_t slv_list_mutex_;                    // 待处理队列线程锁
    pthread_mutex_t result_mutex_;                      // 
    pthread_mutex_t work_list_mutex_;                   // worklist 线程锁
    
    Json::Value* result_root_;                          // 发送给远端服务器的结果json

    Serial result_serial_;                              // 485通信串口
    Camera* camera;                                     // 相机
    Detector* detector;                                 // 检测
    std::shared_ptr<WebsocketServer> server_;           // 本地服务器

    int readNetParam();                                 // 读取参数配置

    int downloadFileList(std::vector<string> file_list);// 从下载队列中下载文件

    std::string getJsonWorkList();                      // worklist->json websocket发送给网页
    std::string getJsonOriginList();                    // originlist->json websocket发送给网页

    void postResult();                                  // 发送结果，单独放在一个线程里运行
    void startServer();                                 // 开启本地服务器，单独线程
    
    static void* postResultWrapper(void *arg);          // pthread线程wrapper
    static void* startServerWrapper(void *arg);         // pthread线程wrapper
    bool handle_signal(std::string url, std::string body, mg_connection *c, OnRspCallback rsp_callback); 
    bool handle_result(std::string url, std::string body, mg_connection *c, OnRspCallback rsp_callback); 
    bool handle_setTask(std::string url, std::string body, mg_connection *c, OnRspCallback rsp_callback);       // 处理网页设置作业
    bool handle_start(std::string url, std::string body, mg_connection *c, OnRspCallback rsp_callback);         // 处理远端服务器的开始信号
    bool handle_changeMod(std::string url, std::string body, mg_connection *c, OnRspCallback rsp_callback);     // 处理网页切换触发模式
    bool handle_softTrigger(std::string url, std::string body, mg_connection *c, OnRspCallback rsp_callback);   // 处理网页触发拍摄
    bool handle_websocketConnect();

    void updateState();                                             // 主线程每个运行周期更新当前页数和当前检测作业在列表中的位置
    void popWorkList();                                             // 丢弃过久的列表元素
    void informCurState();                                          // 通知当前正在处理的作业信息
    void informWorkList();                                          // 当列表发生变化时进行处理,通知网页

public:
    System();
    System(int first);
    ~System();

    void init();
    void run();
    
    int getCurInfo(std::string filename, int64_t &id);             // 
    int setCurWork(int index, int64_t jobid, int64_t curpage);     // 修改当前作业

};

