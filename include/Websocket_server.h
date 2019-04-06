#if !defined(WEBSOCKET_SERVER_H_)
#define WEBSOCKET_SERVER_H_

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include "mongoose.h"

// 定义http返回callback
typedef void OnRspCallback(mg_connection *c, std::string);
// 定义http请求handler
using ReqHandler = std::function<bool (std::string, std::string, mg_connection *c, OnRspCallback)>;

class WebsocketServer
{
private:
    // 静态事件响应函数
	static void OnEvent(mg_connection *connection, int event_type, void *event_data);
	static void HandleHttpEvent(mg_connection *connection, http_message *http_req);
	static void SendRsp(mg_connection *connection, std::string rsp);
	std::vector<std::string> m_local_res_list; // 存储本地静态资源列表

	std::string m_port;    // 端口
	mg_mgr m_mgr;          // 连接管理器
    mg_connection m_con;   // 连接

public:

    enum{
        REQ_ERR,
        REQ_GET,
        REQ_POST,
        REQ_PUT,
        REQ_DELETE
    };

    WebsocketServer();
    ~WebsocketServer();

    void Init(const std::string &port); // 初始化设置
	bool Start(); // 启动httpserver
	bool Close(); // 关闭
	void AddHttpHandler(const std::string &url, ReqHandler req_handler); // 注册事件处理函数
	void RemoveHttpHandler(const std::string &url); // 移除时间处理函数
    int SendWebsocketMsg(const std::string &msg);
	static std::string s_websocket_dir; // 网页根目录
	static mg_serve_http_opts s_websocket_option; // web服务器选项
	static std::unordered_map<std::string, ReqHandler> s_http_handler_map; // 回调函数映射表
};


#endif // WEBSOCKET_SERVER_H_
