#include <iostream>
#include <memory>
#include <sstream>
#include <queue>
#include <json/json.h>
#include "http_server.h"
#include "utils.h"
#include "Camera.h"
#include "Detector.h"
#include "http_client.h"
#include "Serial.h"


// 初始化HttpServer静态类成员
mg_serve_http_opts HttpServer::s_server_option;
std::string HttpServer::s_web_dir = "../web";
std::unordered_map<std::string, ReqHandler> HttpServer::s_handler_map;
// 批次信息
std::deque<std::string> work_name_list;
std::deque<int64_t> work_count_list;
std::deque<std::string> batch_origin_list;
std::deque<int64_t> batch_count_list;

static std::string client_host;
static int server_port;
static int client_port;

/*
	* 下载文件
    * 返回值int, 0表示成功,1表示超时, 其它表示失败
*/
int downloadFileList(std::vector<string> file_list){
	int nCode = -1;
	std::string sIP = client_host;
	unsigned int nPort = client_port;
	std::string sUser = "";   //可为空
	std::string sPwd = "";	  //可为空	

	std::cout<< sIP << ":" << nPort<<endl;
 
	//这边用智能指针更好
	CurlClient* pCurlClient = new CurlClient(sIP, nPort, sUser, sPwd);
	if(NULL == pCurlClient)
	{
		//创建Curl对象失败
		printf("new object failure!!!!!\n");
		return -1;
	}
    
    //curl初始化
    nCode = pCurlClient->initCurlResource();
    if(0 != nCode)
    {
        printf("curl init failure!!!!!\n");
        delete pCurlClient;
        pCurlClient = NULL;
        return -1;
    }
	
	//设置路径
	// TO DO: 修改请求url
	std::string sUrlPath = "";
	pCurlClient->setUrlPath(sUrlPath);
	
	//下载文件名
	for(uint i; i < file_list.size(); i++){
		std::string sFileName = "../images/templates/";
		sFileName.append(file_list[i]);
		int nFormat = 0;
		std::cout<<sFileName<<std::endl;
		nCode = pCurlClient->downloadFile(sFileName, nFormat);
		printf("%d\n", nCode);
	}
		
	delete pCurlClient;
    
    return nCode;

}

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
	// std::cout << "url: " << url << std::endl;
	// std::cout << "body: " << body << std::endl;

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
	int return_state = 0;
	std::vector<std::string> download_list;
	if (parseFromStream(builder, ss, &root, &errs)){
		int work_size = root["printwork"].size();
		if(work_size > 0){
			for(int i = 0; i < work_size; i++){
				int work_id = root["printwork"][i]["printJobID"].asInt();
				int64_t work_count = root["printwork"][i]["printQuantity"].asInt64();
				// 判断文件夹是否存在
				std::string origin_dir = "../images/templates/";
				origin_dir.append(to_string(work_id));
				if(isDirExist(origin_dir) == -1){
					// 下载所有原图，返回错误
					return_state = -1;
					mkdir(origin_dir.c_str(),S_IRWXU|S_IRWXG|S_IRWXO);
				}
				
				if(root["printwork"][i]["pictureLink"].isString()){
					// 图像是string 所有都是这个图
					batch_count_list.push_back(work_count);
					batch_origin_list.push_back(root["printwork"][i]["pictureLink"].asString());
					if(return_state == -1)
						download_list.push_back(root["printwork"][i]["pictureLink"].asString());
					std::cout << "set batch num: " << work_count << std::endl;
					std::cout << "set origin image url" << root["printwork"][i]["pictureLink"].asString() <<std::endl;
					std::string work_name = "print_";
					work_name.append(to_string(work_id));
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
							work_name_list.push_back(work_name);
							work_count_list.push_back(1);
							std::cout << "set work name:" << work_name <<std::endl;
							std::cout << "set work count:" << 1 << std::endl;
						}
						batch_count_list.push_back(1);
						batch_origin_list.push_back(root["printwork"][i]["pictureLink"][j].asString());
						if(return_state == -1){
							std::string down_filename = to_string(work_id);
							down_filename.append("/");
							down_filename.append(root["printwork"][i]["pictureLink"][j].asString());
							download_list.push_back(down_filename);
						}
						std::cout << "set batch num: " << 1 << std::endl;
						std::cout << "set origin image url" << root["printwork"][i]["pictureLink"][j].asString() <<std::endl;
					}
				}
				// printf("batch size %d\n", batch_size);
			}
		}else{
			printf("[ERROR] no work array");
			return_state = -2;
		}
	}else{
		std::cout << "[ERROR] Jsoncpp error: " << errs << std::endl;
	}
	
	rsp_callback(c, to_string(return_state));

	if(return_state == -1){
		sleep(1);
		std::cout << "start download" << std::endl;
		downloadFileList(download_list);
	}


	return true;
}



void* post_result(void *arg){
    
    thread_param* param_ptr;
    param_ptr = (struct thread_param *) arg;
    Json::Value* result_root = param_ptr->root;
    pthread_mutex_t* result_mutex = param_ptr->mutex;
    int nCode = -1;
    std::string sIP = client_host;
    unsigned int nPort = client_port;
    std::string sUser = "";   //可为空
    std::string sPwd = "";	  //可为空	

	std::cout<< sIP << ":" << nPort<<endl;

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
        }
		wait(10);
    }
}

void* startServer(void* server){
	std::shared_ptr<HttpServer> http_server = *(std::shared_ptr<HttpServer> *) server;
	http_server->Start();
}

int readNetParam(){
	std::ifstream paramfile;
	paramfile.open("../settings.json", std::ios::binary);
    if(!paramfile){
        printf("[ERROR] failed to open settings.json\n");
        return -1;
    }else{
        Json::CharReaderBuilder builder;
	    Json::Value root;
        builder["collectComments"] = false;
        JSONCPP_STRING errs;
        if (parseFromStream(builder, paramfile, &root, &errs)){
			client_host = root["communication"]["client_host"].empty() ? "127.0.0.1" : root["communication"]["client_host"].asString();
			server_port = root["communication"]["server_port"].empty() ? 7999 : root["communication"]["server_port"].asInt();
			client_port = root["communication"]["client_port"].empty() ? 7999 : root["communication"]["client_port"].asInt();
			cout << client_host<<":"<<client_port<<","<<server_port << endl;
        }else{
            cout << "[ERROR] Jsoncpp error: " << errs << endl;
        }
        paramfile.close();
    }
	return 0;
}

int main(int argc, char *argv[]) 
{
	pthread_t post_thread_id = 3;
	pthread_t server_thread_id = 4;
	// pthread_t first_thread_id = 5;
    pthread_mutex_t slv_list_mutex;
    pthread_mutex_init(&slv_list_mutex, NULL);
    pthread_mutex_t result_mutex;
    pthread_mutex_init(&result_mutex, NULL);
	pthread_mutex_t fst_slv_list_mutex;
	pthread_mutex_init(&fst_slv_list_mutex, NULL);

    std::queue<string> unsolved_list;

	Json::Value* result_root = new Json::Value();
	// Json::Value* fst_result_root = new Json::Value();
	
	readNetParam();
    CurlClient::globalInit();
    thread_param post_thread_param;
    post_thread_param.root = result_root;
    post_thread_param.mutex = &result_mutex;
	// 错误回报线程
    pthread_create(&post_thread_id, NULL, post_result, (void*)&post_thread_param);
	pthread_detach(post_thread_id);

    std::string port = to_string(server_port);
	std::shared_ptr<HttpServer> http_server = std::shared_ptr<HttpServer>(new HttpServer);
	http_server->Init(port);
	
	// add handler
	http_server->AddHandler("/api/start_signal", handle_signal);
	http_server->AddHandler("/api/result", handle_result);
	http_server->AddHandler("/api/start", handle_start);
	pthread_create(&server_thread_id, NULL, startServer, (void*)&http_server);
	pthread_detach(server_thread_id);

	Serial *result_serial = new Serial();
	result_serial->init("/dev/ttyUSB0");
	result_serial->setBaudRate(115200);
	result_serial->setParity(8,1,'e');

	Camera camera(	&slv_list_mutex, 
					&unsolved_list, 
					&work_name_list, 
					&work_count_list);
	// Detector firstDetector(	&fst_slv_list_mutex, 
	// 						&result_mutex, 
	// 						fst_result_root, 
	// 						&unsolved_list, 
	// 						&work_name_list, 
	// 						&work_count_list, 
	// 						&batch_origin_list, 
	// 						&batch_count_list, 
	// 						first_thread_id);
    Detector monoDetector(&slv_list_mutex, 
							&result_mutex, 
							result_root, 
							&unsolved_list, 
							&work_name_list, 
							&work_count_list, 
							&batch_origin_list,
							&batch_count_list); 
	
    camera.init();
    camera.setTrigger();
    camera.applyParam();


	// firstDetector.setParam();
	// firstDetector.setCameraPtr(&camera);
	// firstDetector.setSerialPtr(result_serial);
	
    monoDetector.setParam();
	monoDetector.setCameraPtr(&camera);
	monoDetector.setSerialPtr(result_serial);

	// 发送开采命令
	int ret = 0;
    ret = camera.start();
    if(ret != 0) printf("[WARN] failed to start camera");

	// ret = firstDetector.launchThread();
	// if(ret != 0) printf("[WARN] failed to launch detection");

    ret = monoDetector.launchThread();
    if(ret != 0) printf("[WARN] failed to launch detection");

	// std::cout << "test file dir: " << isDirExist("") << std::endl;
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
				monoDetector.stopThread();
				std::cout<<"camera stop status: "<<camera.stop()<<std::endl;
                break;
            case 'c':
            case 'C':
                unsolved_list.push("print_1_0_test.ppm");
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