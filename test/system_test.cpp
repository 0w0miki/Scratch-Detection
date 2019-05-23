#include "System.h"

mg_serve_http_opts WebsocketServer::s_websocket_option;
std::string WebsocketServer::s_websocket_dir = "../web";
std::unordered_map<std::string, ReqHandler> WebsocketServer::s_http_handler_map;
std::function<bool()> WebsocketServer::s_ws_connect_handler;

int main(int argc, char const *argv[])
{
    System DetectSystem(SECDETECT);
    DetectSystem.init();
    DetectSystem.run();
    return 0;
}