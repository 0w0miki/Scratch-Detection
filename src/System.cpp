#include "System.h"

System::System()
{
	pthread_mutex_init(&slv_list_mutex_, NULL);
    pthread_mutex_init(&result_mutex_, NULL);

	readNetParam();
	result_root_ = new Json::Value();
	http_server_ = std::shared_ptr<HttpServer>(new HttpServer);
	
	camera = new Camera(&slv_list_mutex_, 
					&unsolved_list_, 
					&work_name_list_, 
					&work_count_list_);

    detector = new Detector(&slv_list_mutex_,
						&result_mutex_, 
						result_root_, 
						&unsolved_list_, 
						&work_name_list_, 
						&work_count_list_, 
						&batch_origin_list_,
						&batch_count_list_,
						SECDETECT);
}

System::~System()
{
	if(nullptr != camera){
		delete camera;
		camera = nullptr;
	}
	if(nullptr != detector){
		delete detector;
		detector = nullptr;
	}
	if(nullptr != result_root_){
		delete result_root_;
		result_root_ = nullptr;
	}
}

void System::init(){
	CurlClient::globalInit();

	std::string port = to_string(server_port_);
	http_server_->Init(port);

	// add handlerhandle_refresh
	// NOTE std::function 不能直接用成员函数替换
	std::function<bool (std::string, std::string, mg_connection *c, OnRspCallback)> f1 = bind(&System::handle_signal,this,std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
	std::function<bool (std::string, std::string, mg_connection *c, OnRspCallback)> f2 = bind(&System::handle_result,this,std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
	std::function<bool (std::string, std::string, mg_connection *c, OnRspCallback)> f3 = bind(&System::handle_start,this,std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
	std::function<bool (std::string, std::string, mg_connection *c, OnRspCallback)> f4 = bind(&System::handle_refresh,this,std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
	http_server_->AddHandler("/api/start_signal", f1);
	http_server_->AddHandler("/api/Print/addErrorReport", f2);
	http_server_->AddHandler("/api/start", f3);
	http_server_->AddHandler("/api/refreshdata", f4);
	
	result_serial_.init("/dev/ttyUSB0");
	result_serial_.setBaudRate(115200);
	result_serial_.setParity(8,1,'e');

	camera->init();
    camera->applyParam();
    camera->setTrigger(HARD_TRIGGER);
	
    detector->setParam();
	detector->setCameraPtr(camera);
	detector->setSerialPtr(&result_serial_);
}

void System::run(){
	// 发送开采命令
	static struct termios save_stdin, nt;
	int ret = 0;
    ret = camera->start();
    if(ret != 0) printf("[WARN] failed to start camera");

    ret = detector->launchThread();
    if(ret != 0) printf("[WARN] failed to launch detection");

	// 错误回报线程 
    pthread_create(&post_thread_id_, NULL, post_result_wraper, this);
	pthread_detach(post_thread_id_);

	pthread_create(&server_thread_id_, NULL, startServer_wraper, this);
	pthread_detach(server_thread_id_);

	// std::cout << "test file dir: " << isDirExist("") << std::endl;
    printf("====================Menu================\n");
    printf("[X or x]:Exit\n");
    printf("[S or s]:Send softtrigger command\n");
	printf("[C or c]:Push back unsolved list\n");

	// 按键输入后会持续 容易不受控
	int tty_fd=open("/dev/tty",O_RDONLY);
	if(tty_fd == -1)
		printf("failed to open tty\n");
	tcgetattr(0,&save_stdin);
	char key = 0, lastkey = 0;

	int flags = fcntl(tty_fd,F_GETFL);
	flags |= O_NONBLOCK;
	nt = save_stdin;
	nt.c_lflag &= ~(ICANON);//设为0所以用&
	tcsetattr(0,TCSANOW,&nt);
	if(fcntl(tty_fd,F_SETFL,flags)==-1){
		printf("failed to set tty_fd");
		return;
	}

	bool run = true;
    while(run == true)
    {
		updateState();
		// 换成非阻塞的键盘响应事件
		read(tty_fd,&key,1);
		// int c = getchar();
        switch(key)
        {
			case 10:
				// 回车确认
				switch (lastkey)
				{
					//退出程序
					case 'X':
					case 'x':
						run = false;
						detector->stopThread();
						std::cout<<"camera stop status: "<<camera->stop()<<std::endl;
						tcsetattr(0,TCSANOW,&save_stdin);
						close(tty_fd);
						break;
					case 'C':
					case 'c':
						unsolved_list_.push("print_1_0_test.ppm");
						break;
					//发送一次软触发命令
					case 'S':
					case 's':
						ret = camera->sendSoftTrigger();
						printf("<The return value of softtrigger command: %d>\n", ret);
						break;
					default:
						break;
				}
				lastkey = 0;
				break;
            default:
				lastkey = key;
                break;
		}
		wait(10);
    }
	return;
}

void System::updateState(){
	// TODO this part may not be thread safe
	int m = work_name_list_.size();
	int n = work_list_.size();
	int detid = detector->getCount();
	std::string cur_name = work_name_list_.front();
	
	if(work_name_list_.empty()){
		cur_id_ = 0;
		cur_page_ = 0;
		cur_index_ = 0;
		return;
	}
	std::vector<std::string> splitres;
	SplitString(cur_name, splitres, "_");
	cur_id_ = std::stoll(splitres[1]);
	if(splitres[0] == "reprint"){
		// 重打 id和编号直接由名字给出
		cur_page_ = std::stoll(splitres[2]);
	}else{
		cur_page_ = detid;
	}
	cur_index_ = n - m + detid;
}

int  System::setCurWork(int index, int64_t jobid, int64_t curpage){
	if(index == cur_index_ && jobid == cur_id_ && curpage == cur_page_)
		return 0;
	if(index > cur_index_){}
	// 校验要设置成的内容是否正确
	int remain = index;
	int64_t id = -1;
	int diff_count = 0;
	for(auto &work_info:work_list_){
		int num = work_info.count;
		if (remain > num){
			remain -= num;
		}else{
			id = work_info.id;
		}
		diff_count++;
	}
	
	batch_count_list_.front();

	// if(jobid == id && (curpage == remain || curpage == )){
	// 	if(index > cur_index_){
	// 		// 往后设置, pop掉若干个 设置id
	// 		while(remain>0){
	// 			auto id_num = work_count_list_.front();
	// 			if(remain > id_num){

	// 			}else{

	// 			}
	// 		}
	// 		work_name_list_.erase(work_name_list_.begin(),work_name_list_.begin() + pop_num);
	// 		work_count_list_.erase(work_name_list_.begin(),work_name_list_.begin() + pop_num);
			
	// 		batch_count_list_.erase(work_name_list_.begin(),work_name_list_.begin() + pop_num);
	// 		batch_origin_list_.erase(work_name_list_.begin(),work_name_list_.begin() + pop_num);
			
	// 	}else{
	// 		// 回溯, push front若干个 设置id
	// 		int num = cur_index_ - index;
	// 		cur_id_ = id;
	// 		cur_page_ = curpage;
	// 		cur_index_ = index;
			
	// 	}
	// }
	return -1;
}

// ANCHOR util functions

int System::getCurInfo(std::string filename, int64_t &id){
	int page = 0;
	std::vector<std::string> split_name;
	if(unsolved_list_.empty()){
		return -1;
	}
	std::string unsolved_filename = unsolved_list_.front();
	SplitString(unsolved_filename, split_name, "_");
	if(split_name[0] == "reprint"){
        // 重打
		id = std::stoi(split_name[1]);
        page = std::stoi(split_name[2]);
    }else{
        // 新打
        id  = std::stoi(split_name[1]);
    }
	return page;
}

Json::Value System::getJsonWorkList(){
	Json::Value res;
	if(work_list_.empty()){
		return res;
	}
	Json::StreamWriterBuilder wbuilder;
    wbuilder["indentation"] = "";
	std::unique_ptr<Json::StreamWriter> writer(wbuilder.newStreamWriter());
	Json::Value element;
	for(auto work:work_list_){
		element["id"] = work.id;
		element["num"] = work.count;
		res.append(element);
	}
	auto s = res.toStyledString();
	return res;
}

int System::readNetParam(){
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
			client_host_ = root["communication"]["client_host"].empty() ? "127.0.0.1" : root["communication"]["client_host_"].asString();
			server_port_ = root["communication"]["server_port"].empty() ? 7999 : root["communication"]["server_port"].asInt();
			client_port_ = root["communication"]["client_port"].empty() ? 7999 : root["communication"]["client_port"].asInt();
			result_url_  = root["communication"]["result_url"].empty() ? "/api/Print/addErrorReport" : root["communication"]["result_url"].asString();
			cout << client_host_<<":"<<client_port_<<","<<server_port_ << endl;
        }else{
            cout << "[ERROR] Jsoncpp error: " << errs << endl;
        }
        paramfile.close();
    }
	return 0;
}


// ANCHOR Client functions
int System::downloadFileList(std::vector<string> file_list){
	int nCode = -1;
	std::string sIP = client_host_;
	unsigned int nPort = client_port_;
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
	
	//下载文件名
	for(auto &file_url:file_list){
		//设置路径
		std::string sUrlPath = "/" + file_url;

		cout<<"url path"<<file_url<<endl;

		pCurlClient->setUrlPath(sUrlPath);
		std::string sFileName = "../images/templates/";
		sFileName.append(file_url);
		int nFormat = 0;
		std::cout<<sFileName<<std::endl;
		nCode = pCurlClient->downloadFile(sFileName, nFormat);
		printf("%d\n", nCode);
	}
		
	delete pCurlClient;
    
    return nCode;

}

// ANCHOR HTTP Server functions
bool System::handle_signal(std::string url, std::string body, mg_connection *c, OnRspCallback rsp_callback)
{
	// do sth
	std::cout << "handle start signal" << std::endl;
	std::cout << "url: " << url << std::endl;
	std::cout << "body: " << body << std::endl;

    int ret = readBatchFile("batch_info.json", work_name_list_, work_count_list_, batch_origin_list_, batch_count_list_);
	rsp_callback(c, "recived");
	return true;
}

bool System::handle_result(std::string url, std::string body, mg_connection *c, OnRspCallback rsp_callback){
	std::cout << "handle result" << std::endl;

	rsp_callback(c, "{ \"StatusCode\": 1 }");

	return true;
}

bool System::handle_refresh(std::string url, std::string body, mg_connection *c, OnRspCallback rsp_callback){
	std::cout << "handle refresh" << std::endl;
	std::cout << "url: " << url << std::endl;
	std::cout << "body: " << body << std::endl;

	Json::Value response;
	Json::StreamWriterBuilder wbuilder;
    wbuilder["indentation"] = "";
	std::unique_ptr<Json::StreamWriter> writer(wbuilder.newStreamWriter());

	response["CurJobId"] = cur_id_;
	response["CurPage"] = cur_page_;
	response["CurIndex"] = cur_index_;

	rsp_callback(c, response.toStyledString());
	return true;
}

bool System::handle_start(std::string url, std::string body, mg_connection *c, OnRspCallback rsp_callback){
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
				// origin_dir.append(to_string(work_id));
				if(isDirExist(origin_dir) == -1){
					// 下载所有原图，返回错误
					return_state = -1;
					mkdir(origin_dir.c_str(),S_IRWXU|S_IRWXG|S_IRWXO);
				}
				
				if(root["printwork"][i]["pictureLink"].isString()){
					// 图像是string 所有都是这个图
					if(return_state == -1)
						download_list.push_back(root["printwork"][i]["pictureLink"].asString());
					else{
						std::string filename = origin_dir + root["printwork"][i]["pictureLink"].asString();
						if(access(filename.c_str(),F_OK) == -1){
							return_state = -3;
							download_list.push_back(root["printwork"][i]["pictureLink"].asString());
							std::cout << "[WARN] " << filename << " does not exist, try to download" << endl;
						}
					}
					std::cout << "set batch num: " << work_count << std::endl;
					std::cout << "set origin image url" << root["printwork"][i]["pictureLink"].asString() <<std::endl;
					std::string work_name = "print_";
					work_name.append(to_string(work_id));
					work_name_list_.push_back(work_name);
					work_count_list_.push_back(work_count);
					batch_count_list_.push_back(work_count);
					batch_origin_list_.push_back(root["printwork"][i]["pictureLink"].asString());
					WorkInfo winfo;
					winfo.id = work_id;
					winfo.count = work_count;
					// ANCHOR now work at here
					work_list_.push_back(winfo);
					
					std::cout << "set work name:" << work_name << std::endl;
					std::cout << "set work count:" << work_count << std::endl;
				}else{
					// 列表
					int link_size = root["printwork"][i]["pictureLink"].size();
					if(root["printwork"][i]["printRedoFlag"].asBool() == false){
						// 不是重打
						std::string work_name = "print_";
						work_name.append(to_string(work_id));
						work_name_list_.push_back(work_name);
						work_count_list_.push_back(work_count);
						WorkInfo winfo;
						winfo.id = work_id;
						winfo.count = work_count;
						work_list_.push_back(winfo);
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
							work_name_list_.push_back(work_name);
							work_count_list_.push_back(1);
							WorkInfo winfo;
							winfo.id = work_id;
							winfo.count = 1;
							work_list_.push_back(winfo);
							std::cout << "set work name:" << work_name <<std::endl;
							std::cout << "set work count:" << 1 << std::endl;
						}
						batch_count_list_.push_back(1);
						batch_origin_list_.push_back(root["printwork"][i]["pictureLink"][j].asString());
						if(return_state == -1){
							std::string down_filename;
							// down_filename.append("/");
							down_filename = root["printwork"][i]["pictureLink"][j].asString();
							download_list.push_back(down_filename);
						}else{
							std::string filename = origin_dir + root["printwork"][i]["pictureLink"][j].asString();
							if(access(filename.c_str(),F_OK) == -1){
								return_state = -3;
								download_list.push_back(root["printwork"][i]["pictureLink"][j].asString());
								std::cout << "[WARN]" << filename << "does not exist, try to download" << endl;
							}
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
	
	std::string response = "{ \"StatusCode\": \"" + to_string(return_state) + "\" }";
	rsp_callback(c, response);

	if(return_state == -1 || return_state == -3){
		sleep(1);
		std::cout << "start download" << std::endl;
		downloadFileList(download_list);
	}


	return true;
}

void* System::post_result_wraper(void * arg){
	System * thisptr= (System *) arg;
	thisptr->post_result();
}

void System::post_result(){
    int nCode = -1;
    std::string sIP = client_host_;
    unsigned int nPort = client_port_;
    std::string sUser = "";   //可为空
    std::string sPwd = "";	  //可为空	

	std::cout<< sIP << ":" << nPort<<endl;

    //这边用智能指针更好
	pthread_mutex_t* mutex_ptr = &result_mutex_;
    CurlClient* pCurlClient = new CurlClient(sIP, nPort, sUser, sPwd, mutex_ptr, result_root_);
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
    std::string sUrlPath = result_url_;
    pCurlClient->setUrlPath(sUrlPath);
    
    std::string res="";
    bool run_flag = true;
    while(run_flag){
        if(!result_root_->empty()){
            pCurlClient->sendResult(res);
            printf("%s\n", res.c_str());
        }
		wait(10);
    }
}

void* System::startServer_wraper(void *arg){
	System* this_ptr= (System *) arg;
	this_ptr->startServer();
}

void System::startServer(){
	http_server_->Start();
}

// int main(int argc, char *argv[]) 
// {
	
//     CurlClient::globalInit();


	
    

	

// 	return 0;
// }