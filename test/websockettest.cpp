#include "Websocket_server.h"
#include <memory>

// 初始化WebsocketServer静态类成员
mg_serve_http_opts WebsocketServer::s_websocket_option;
std::string WebsocketServer::s_websocket_dir = "../web";
std::unordered_map<std::string, ReqHandler> WebsocketServer::s_http_handler_map;
std::function<bool()> WebsocketServer::s_ws_connect_handler;

void* startServer(void* server){
	std::shared_ptr<WebsocketServer> http_server = *(std::shared_ptr<WebsocketServer> *) server;
	http_server->Start();
}

int main(int argc, char const *argv[])
{
    std::shared_ptr<WebsocketServer> server = std::shared_ptr<WebsocketServer>(new WebsocketServer);

    pthread_t server_thread_id = 1;
    pthread_create(&server_thread_id, NULL, startServer, (void*)&server);
	pthread_detach(server_thread_id);

    server->Init("7999");
    
    bool run_flag = true;
    while(run_flag){
        char ch = getchar();
        switch (ch)
        {
            case 27:
            case 'x':
            case 'X':
                server->Close();
                run_flag = false;
                break;
            case 's':
            case 'S':
                server->SendWebsocketMsg("test send");
                break;
            default:
                break;
        }
    }
    

    return 0;
}
