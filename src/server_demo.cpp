#include <iostream>
#include <memory>
#include <sstream>
#include <queue>
#include <json/json.h>
#include "http_server.h"
#include "utils.h"
#include "Camera.h"
#include "Detector.h"
#include "client.h"

// 初始化HttpServer静态类成员
mg_serve_http_opts HttpServer::s_server_option;
std::string HttpServer::s_web_dir = "./web";
std::unordered_map<std::string, ReqHandler> HttpServer::s_handler_map;
// 批次信息
std::deque<std::string> work_name_list;
std::deque<int64_t> work_count_list;
std::deque<std::string> batch_origin_list;
std::deque<int64_t> batch_count_list;

bool handle_signal(std::string url, std::string body, mg_connection *c, OnRspCallback rsp_callback)
{
	// do sth
	std::cout << "handle start signal" << std::endl;
	std::cout << "url: " << url << std::endl;
	std::cout << "body: " << body << std::endl;

    int ret = readBatchFile("batch_info.json", work_name_list, work_count_list, batch_origin_list, batch_count_list);
	rsp_callback(c, "recived");
	return true;
}

bool handle_result(std::string url, std::string body, mg_connection *c, OnRspCallback rsp_callback){
	std::cout << "handle result" << std::endl;
	std::cout << "url: " << url << std::endl;
	std::cout << "body: " << body << std::endl;

	// int l = body.length();
	// for(size_t i=0;i<l;i++){
	// 	printf("char %x", body.c_str()[i]);
	// }

	rsp_callback(c, "1");

	return true;
}

bool handle_start(std::string url, std::string body, mg_connection *c, OnRspCallback rsp_callback){
	std::cout << "handle start" << std::endl;
	std::cout << "url: " << url << std::endl;
	std::cout << "body: " << body << std::endl;

	std::stringstream ss(body);
	Json::CharReaderBuilder builder;
	Json::Value root;
	builder["collectComments"] = false;
	JSONCPP_STRING errs;
	if (parseFromStream(builder, ss, &root, &errs)){
		int work_size = root["printwork"].size();
		if(work_size > 0){
			for(int i = 0; i < work_size; i++){
				int work_id = root["printwork"][i]["printJobID"].asInt();
				int64_t work_count = root["printwork"][i]["printQuantity"].asInt64();
				if(root["printwork"][i]["pictureLink"].isString()){
					// 图像是string 所有都是这个图
					batch_count_list.push_back(work_count);
					batch_origin_list.push_back(root["printwork"][i]["pictureLink"].asString());
					std::cout << "set batch num: " << work_count << std::endl;
					std::cout << "set origin image url" << root["printwork"][i]["pictureLink"].asString() <<std::endl;
					std::string work_name = "print_";
					work_name.append(to_string(work_id));
					work_name.append("_");
					work_name_list.push_back(work_name);
					work_count_list.push_back(work_count);
					std::cout << "set work name:" << work_name << std::endl;
					std::cout << "set work count:" << work_count << std::endl;
				}else{
					// 列表
					int link_size = root["printwork"][i]["pictureLink"].size();
					if(root["printwork"][i]["printRedoFlag"].asBool() == false){
						// 不是重打
						std::string work_name = "print_";
						work_name.append(to_string(work_id));
						work_name.append("_");
						work_name_list.push_back(work_name);
						work_count_list.push_back(work_count);
						std::cout << "set work name:" << work_name <<std::endl;
						std::cout << "set work count:" << work_count << std::endl;
					}
					for(int j = 0; j < link_size; j++)
					{
						if(root["printwork"][i]["printRedoFlag"].asBool() == true){
							// 重打
							int reprint_id = root["printwork"][i]["printWorkNumber"][j].asInt();
							std::string work_name = "reprint_";
							work_name.append(to_string(work_id));
							work_name.append("_");
							work_name.append(to_string(reprint_id));
							work_name.append("_");
							work_name_list.push_back(work_name);
							work_count_list.push_back(1);
							std::cout << "set work name:" << work_name <<std::endl;
							std::cout << "set work count:" << 1 << std::endl;
						}
						batch_count_list.push_back(1);
						batch_origin_list.push_back(root["printwork"][i]["pictureLink"][j].asString());
						std::cout << "set batch num: " << 1 << std::endl;
						std::cout << "set origin image url" << root["printwork"][i]["pictureLink"][j].asString() <<std::endl;
					}
				}
				// printf("batch size %d\n", batch_size);
			}
		}else{
			printf("[ERROR] no work array");
			return -2;
		}
	}else{
		std::cout << "[ERROR] Jsoncpp error: " << errs << std::endl;
	}
	
	rsp_callback(c, "0");

	return true;
}

void* post_result(void *arg){
    
    thread_param* param_ptr;
    param_ptr = (struct thread_param *) arg;
    Json::Value* result_root = param_ptr->root;
    pthread_mutex_t* result_mutex = param_ptr->mutex;
    int nCode = -1;
    std::string sIP = "127.0.0.1";
    unsigned int nPort = 7999;
    std::string sUser = "";   //可为空
    std::string sPwd = "";	  //可为空	

    //这边用智能指针更好
    CurlClient* pCurlClient = new CurlClient(sIP, nPort, sUser, sPwd, result_mutex, result_root);
    if(NULL == pCurlClient)
    {
        //创建Curl对象失败
        printf("new object failure!!!!!\n");
    }
    
    //curl初始化
    nCode = pCurlClient->initCurlResource();
    if(0 != nCode)
    {
        printf("curl init failure!!!!!\n");
        delete pCurlClient;
        pCurlClient = NULL;
    }
    
    //设置路径
    std::string sUrlPath = "/api/result";
    pCurlClient->setUrlPath(sUrlPath);
    
    std::string res="";
    bool run_flag = true;
    while(run_flag){
        if(!result_root->empty()){
            pCurlClient->sendResult(res);
            printf("%s\n", res.c_str());
			struct timeval tv;
			tv.tv_sec = 0;
			tv.tv_usec = 10 * 1000;
			select(0,NULL,NULL,NULL,&tv);
        }
    }
}

void* startServer(void* server){
	std::shared_ptr<HttpServer> http_server = *(std::shared_ptr<HttpServer> *) server;
	http_server->Start();
}

int main(int argc, char *argv[]) 
{
	pthread_t post_thread_id = 3;
	pthread_t server_thread_id = 4;
    pthread_mutex_t slv_list_mutex;
    pthread_mutex_init(&slv_list_mutex, NULL);
    pthread_mutex_t result_mutex;
    pthread_mutex_init(&result_mutex, NULL);

    std::queue<string> unsolved_list;

	Json::Value* result_root = new Json::Value();
    CurlClient::globalInit();

    thread_param post_thread_param;
    post_thread_param.root = result_root;
    post_thread_param.mutex = &result_mutex;
	// 错误回报线程
    pthread_create(&post_thread_id, NULL, post_result, (void*)&post_thread_param);
	pthread_detach(post_thread_id);

    std::string port = "7999";
	std::shared_ptr<HttpServer> http_server = std::shared_ptr<HttpServer>(new HttpServer);
	http_server->Init(port);
	// add handler
	http_server->AddHandler("/api/start_signal", handle_signal);
	http_server->AddHandler("/api/result", handle_result);
	http_server->AddHandler("/api/start", handle_start);
	pthread_create(&server_thread_id, NULL, startServer, (void*)&http_server);
	pthread_detach(server_thread_id);

	Camera camera(&slv_list_mutex, &unsolved_list, &work_name_list, &work_count_list);
    Detector detector(&slv_list_mutex, &result_mutex, result_root, &unsolved_list, &work_name_list, &work_count_list, &batch_origin_list, &batch_count_list);

    camera.init();
    camera.setTrigger();
    camera.applyParam();

    detector.setParam();

	// 发送开采命令
    int ret = camera.start();
    if(ret != 0) printf("[WARN] failed to start camera");

    ret = detector.launchThread();
    if(ret != 0) printf("[WARN] failed to launch detection");

    printf("====================Menu================\n");
    printf("[X or x]:Exit\n");
    printf("[S or s]:Send softtrigger command\n");
	printf("[C or c]:Push back unsolved list\n");
    printf("[S or s]:Send softtrigger command\n");

	bool run = true;
    while(run == true)
    {
        int c = getchar();
        switch(c)
        {
            //退出程序
            case 'X':
            case 'x':
                run = false;
                break;
            case 'c':
            case 'C':
                unsolved_list.push("print_1_0.ppm");
                break;
            //发送一次软触发命令
            case 'S':
            case 's':
                ret = camera.sendSoftTrigger();
                printf("<The return value of softtrigger command: %d>\n", ret);
                break;

            default:
                break;
        }	
    }

	return 0;
}