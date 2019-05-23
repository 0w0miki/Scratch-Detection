#include "Websocket_server.h"

WebsocketServer::WebsocketServer()
{
}

WebsocketServer::~WebsocketServer()
{
    
}

static int isWebsocket(const struct mg_connection *nc) {
    return nc->flags & MG_F_IS_WEBSOCKET;
}

static int route_check(http_message *http_msg, const char *route_prefix)
{
	if (mg_vcmp(&http_msg->uri, route_prefix) == 0)
		if (mg_vcmp(&http_msg->method, "GET"))
			return WebsocketServer::REQ_GET;
		else if (mg_vcmp(&http_msg->method, "POST"))
			return WebsocketServer::REQ_POST;
		else if (mg_vcmp(&http_msg->method, "PUT"))
			return WebsocketServer::REQ_PUT;
		else if (mg_vcmp(&http_msg->method, "DELETE"))
			return WebsocketServer::REQ_DELETE;
		else
			return WebsocketServer::REQ_ERR;
	else
		return WebsocketServer::REQ_ERR;
}

void WebsocketServer::Init(const std::string &port){
    m_port = port;
    s_websocket_option.enable_directory_listing = "yes";
    s_websocket_option.document_root = s_websocket_dir.c_str();
}

bool WebsocketServer::Start(){
    mg_mgr_init(&m_mgr, NULL);
	mg_connection *connection = mg_bind(&m_mgr, m_port.c_str(), OnEvent);
	if (connection == NULL)
		return false;
	mg_set_protocol_http_websocket(connection);

	sLog->logInfo("starting websocket server at port: %s", m_port.c_str());
	// loop
	while (true)
		mg_mgr_poll(&m_mgr, 200); // ms

	return true;
}

void WebsocketServer::AddHttpHandler(const std::string &url, ReqHandler req_handler){
    if(s_http_handler_map.find(url) != s_http_handler_map.end())
        return;
    
    s_http_handler_map.insert(std::make_pair(url,req_handler));
}

void WebsocketServer::setWebsocketConnectHandler(std::function<bool ()> fun){
	s_ws_connect_handler = fun;
}

void WebsocketServer::RemoveHttpHandler(const std::string &url){
    auto iter = s_http_handler_map.find(url);
    if(iter != s_http_handler_map.end())
        s_http_handler_map.erase(iter);
}

void WebsocketServer::OnEvent(mg_connection *connection, int event_type, void *event_data){
    switch (event_type)
    {
        case MG_EV_WEBSOCKET_HANDSHAKE_DONE:{
            sLog->logInfo("New websocket connection:%p\n", connection);
            std::string msg("Hello Websocket Client");
            mg_send_websocket_frame(connection, WEBSOCKET_OP_TEXT, msg.c_str(), msg.size());
			s_ws_connect_handler();
            break;
        }
        case MG_EV_WEBSOCKET_FRAME:{
            struct websocket_message *wm = (struct websocket_message *)event_data;
            sLog->logInfo("receive websocket msg: %s\n", wm->data);
            break;
        }
        case MG_EV_CLOSE:{
            if(isWebsocket(connection))
                sLog->logInfo("Websocket connection:%p closed\n", connection);
            break;
        }
        case MG_EV_HTTP_REQUEST:{
            http_message *http_req = (http_message *)event_data;
            HandleHttpEvent(connection, http_req);
            break;
        }
        default:
            break;
    }
}

void WebsocketServer::HandleHttpEvent(mg_connection *connection, http_message *http_req)
{
	std::string req_str = std::string(http_req->message.p, http_req->message.len);
	sLog->logInfo("got request: %s\n", req_str.c_str());

	// 先过滤是否已注册的函数回调
	std::string url = std::string(http_req->uri.p, http_req->uri.len);
	std::string body = std::string(http_req->body.p, http_req->body.len);
	auto it = s_http_handler_map.find(url);
	if (it != s_http_handler_map.end())
	{
		ReqHandler handle_func = it->second;
		handle_func(url, body, connection, SendRsp);
	}

	// 其他请求 NOTE "先这样试试，之后改成验证是否存在静态页面"
	if (route_check(http_req, "/")) // index page
		mg_serve_http(connection, http_req, s_websocket_option);
	else if (route_check(http_req, "/mystyle.css")) // css
		mg_serve_http(connection, http_req, s_websocket_option);
	else if (route_check(http_req, "/updatedata.js")) // js
		mg_serve_http(connection, http_req, s_websocket_option);
	else if (route_check(http_req, "/api/hello")) 
	{
		// 直接回传
		SendRsp(connection, "welcome to httpserver");
	}
	else
	{
		mg_serve_http(connection, http_req, s_websocket_option);
	}
}

int WebsocketServer::SendWebsocketMsg(const std::string &msg){
    struct mg_connection *c;
    for (c = mg_next(&m_mgr, NULL); c != NULL; c = mg_next(&m_mgr, c)) {
        if(isWebsocket(c)){
            mg_send_websocket_frame(c, WEBSOCKET_OP_TEXT, msg.c_str(), msg.size());
        }
    }
    return 0;
}

void WebsocketServer::SendRsp(mg_connection *connection, std::string rsp)
{
	sLog->logInfo("send response %s", rsp.c_str());
	// 必须先发送header
	mg_printf(connection, "%s", "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");
	// 以json形式返回
	// mg_printf_http_chunk(connection, "{ \"StatusCode\": %s }", rsp.c_str());
	mg_printf_http_chunk(connection, "%s", rsp.c_str());
	// 发送空白字符快，结束当前响应
	mg_send_http_chunk(connection, "", 0);
}

bool WebsocketServer::Close(){
    mg_mgr_free(&m_mgr);
	return true;
}