#include "System.h"

mg_serve_http_opts HttpServer::s_server_option;
std::string HttpServer::s_web_dir = "../web";
std::unordered_map<std::string, ReqHandler> HttpServer::s_handler_map;

int main(int argc, char const *argv[])
{
    System DetectSystem;
    DetectSystem.init();
    DetectSystem.run();
    return 0;
}