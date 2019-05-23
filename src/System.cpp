#include "System.h"
/**
 * @brief Construct a new System:: System object
 * 
 */
System::System(){
	work_list_.reserve(5000);
	origin_list_.reserve(5000);

	pthread_mutex_init(&slv_list_mutex_, NULL);
    pthread_mutex_init(&result_mutex_, NULL);
	pthread_mutex_init(&work_list_mutex_, NULL);

	sLog->init("../log","detection");
	readNetParam();

	result_root_ = new Json::Value();
	server_ = std::shared_ptr<WebsocketServer>(new WebsocketServer);
	
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

/**
 * @brief Construct a new System:: System object
 * 
 * @param firstflag 彩色检测低2位置1
 */
System::System(int firstflag){
	work_list_.reserve(5000);
	origin_list_.reserve(5000);

	pthread_mutex_init(&slv_list_mutex_, NULL);
    pthread_mutex_init(&result_mutex_, NULL);
	pthread_mutex_init(&work_list_mutex_, NULL);

	sLog->init("../log","detection");

	readNetParam();
	result_root_ = new Json::Value();
	server_ = std::shared_ptr<WebsocketServer>(new WebsocketServer);
	
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
						firstflag);
}

/**
 * @brief Destroy the System:: System object
 * 
 */
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

/**
 * @brief 系统初始化
 * 
 */
void System::init(){
	CurlClient::globalInit();

	std::string port = to_string(server_port_);
	server_->Init(port);

	// NOTE "std::function 不能直接用成员函数替换"
	std::function<bool (std::string, std::string, mg_connection *c, OnRspCallback)> f1 = bind(&System::handle_signal,this,std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
	std::function<bool (std::string, std::string, mg_connection *c, OnRspCallback)> f2 = bind(&System::handle_result,this,std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
	std::function<bool (std::string, std::string, mg_connection *c, OnRspCallback)> f3 = bind(&System::handle_start,this,std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
	std::function<bool (std::string, std::string, mg_connection *c, OnRspCallback)> f4 = bind(&System::handle_setTask,this,std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
	std::function<bool ()> f5 = bind(&System::handle_websocketConnect,this);
	std::function<bool (std::string, std::string, mg_connection *c, OnRspCallback)> f6 = bind(&System::handle_changeMod,this,std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
	std::function<bool (std::string, std::string, mg_connection *c, OnRspCallback)> f7 = bind(&System::handle_softTrigger,this,std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
	server_->AddHttpHandler("/api/start_signal", f1);
	server_->AddHttpHandler("/api/Print/addErrorReport", f2);
	server_->AddHttpHandler("/api/start", f3);
	server_->AddHttpHandler("/api/setTask", f4);
	server_->setWebsocketConnectHandler(f5);
	server_->AddHttpHandler("/api/changeMode", f6);
	server_->AddHttpHandler("/api/softTrigger", f7);
	
	// 设置波特率,停止位
	result_serial_.init("/dev/ttyUSB0");
	result_serial_.setBaudRate(19200);
	result_serial_.setParity(8,1,'n');

	// 相机初始化
	camera->init();
    camera->applyParam();
    camera->setTrigger(HARD_TRIGGER);
	
	// 检测初始化
    detector->setParam();
	detector->setCameraPtr(camera);
	detector->setSerialPtr(&result_serial_);
}

/**
 * @brief 启动系统
 * 
 */
void System::run(){
	// 发送开采命令
	static struct termios save_stdin, nt;
	int ret = 0;
    ret = camera->start();
    if(ret != 0) sLog->logWarn("failed to start camera");

    ret = detector->launchThread();
    if(ret != 0) sLog->logWarn("failed to launch detection");

	// 错误回报线程 
    pthread_create(&post_thread_id_, NULL, postResultWrapper, this);
	pthread_detach(post_thread_id_);

	// 本地服务器线程
	pthread_create(&server_thread_id_, NULL, startServerWrapper, this);
	pthread_detach(server_thread_id_);

    printf("====================Menu================\n");
    printf("[X or x]:Exit\n");
    printf("[S or s]:Send softtrigger command\n");
	printf("[C or c]:Push back unsolved list\n");

	// 按键控制 读取终端输入 实现非阻塞输入
	int tty_fd=open("/dev/tty",O_RDONLY);
	bool inputCtl = true;
	if(tty_fd == -1){
		sLog->logError("failed to open tty");
		inputCtl = false;
	}
	tcgetattr(0,&save_stdin);
	char key = 0, lastkey = 0;

	int flags = fcntl(tty_fd,F_GETFL);
	flags |= O_NONBLOCK;
	nt = save_stdin;
	nt.c_lflag &= ~(ICANON);//设为0所以用&
	tcsetattr(0,TCSANOW,&nt);
	if(fcntl(tty_fd,F_SETFL,flags)==-1){
		sLog->logError("failed to set tty_fd");
		inputCtl = false;
	}

	bool run = true;
    while(run == true)
    {
		updateState();
		if(inputCtl){
			// 换成非阻塞的键盘响应事件
			read(tty_fd,&key,1);
			// int c = getchar();
			switch(key)
			{
				case 10:
					// NOTE "直接读取输入容易不受控制，使用回车确认"
					switch (lastkey)
					{
						//退出程序
						case 'X':
						case 'x':
							run = false;
							detector->stopThread();
							sLog->logInfo("camera stop status: %d",camera->stop());
							tcsetattr(0,TCSANOW,&save_stdin);
							close(tty_fd);
							break;
						case 'C':
						case 'c':
							unsolved_list_.push("a.bmp");
							break;
						//发送一次软触发命令
						case 'S':
						case 's':
							ret = camera->sendSoftTrigger();
							sLog->logDebug("The return value of softtrigger command: %d", ret);
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
		}
		wait(10);
    }
	return;
}

void System::updateState(){
	if(work_name_list_.empty()){
		cur_id_ = 0;
		cur_page_ = 0;
		cur_index_ = 0;
		totla_page_ = 0;
		return;
	}
	static int last_length = 0;
	int send_flag = 0;
	pthread_mutex_lock(&work_list_mutex_);
	int m = work_name_list_.size();
	int n = work_list_.size();
	int det_num = detector->getIdCount();
	std::string cur_name = work_name_list_.front();

	std::vector<std::string> splitres;
	SplitString(cur_name, splitres, "_");
	cur_id_ = std::stoll(splitres[1]);
	if(splitres[0] == "reprint"){
		// 重打 id和编号直接由名字给出
		cur_page_ = std::stoll(splitres[2]);
	}else{
		cur_page_ = det_num;
	}
	totla_page_ = work_count_list_.front();
	// 计算index
	int index = 0;
	for(int i = 0; i < n-m; ++i){
		index += work_list_[i].reprint_flag ? 1 : work_list_[i].page;
	}
	index += det_num;
	if(cur_index_ != index){
		cur_index_ = index;
		send_flag = 1;
	}

	// 检验是否需要清除列表头部
	if( cur_index_ > 10 && n-m > 10){
		int i = 0;
		for(auto workinfo:work_list_){
			cur_index_ -= workinfo.reprint_flag ? 1 : workinfo.page;
			if(i >= n-m-10)
				break;
			++i;
		}
		work_list_.erase(work_list_.begin(), work_list_.begin()+n-m-10);
		send_flag = 2;
	}
	if(n != last_length){
		send_flag = 2;
		last_length = n;
	}
	pthread_mutex_unlock(&work_list_mutex_);

	switch (send_flag)
	{
		case 1:
			informCurState();
			break;
		case 2:
			informCurState();
			informWorkList();
			break;
		default:
			break;
	}
}

void System::informCurState(){
	string msg = "statemsg{\"index\":" + to_string(cur_index_) + ",\"id\":" + to_string(cur_id_) + ",\"page\":" + to_string(cur_page_) + ",\"totalpage\":" + to_string(totla_page_) + "}";
	sLog->logDebug("Websocket send %s", msg.c_str());
	server_->SendWebsocketMsg(msg);
}

void System::informWorkList(){
	string list = "listmsg{\"index\":" + to_string(cur_index_) + ",\"list\":" + getJsonWorkList() + ",\"origin\":" + getJsonOriginList() + "}";
	sLog->logDebug("Websocket send %s", list.c_str());
	server_->SendWebsocketMsg(list);
}

int  System::setCurWork(int index, int64_t jobid, int64_t curpage){
	if(index == cur_index_ && jobid == cur_id_ && curpage == cur_page_){

		return 0;
	}

	// 校验要设置成的内容是否正确
	int remain = index;
	int64_t id = -1;
	int16_t page = 0;
	int id_index = 0;
	
	for(auto &work_info:work_list_){
		int num = work_info.reprint_flag ? 1 : work_info.page;
		if (remain > num){
			remain -= num;
		}else{
			id = work_info.id;
			page = remain;
		}
		id_index++;
	}
	if(jobid != id || curpage != page){
		sLog->logError("Set work can not match with list.");
		return -1;
	}
	
	detector->callPause();
	camera->callPause();

	while(!detector->isPause() && !camera->isPause()){
		// 阻塞等待detector和camera进入暂停状态
		sLog->logDebug("wait detector and camera pause");
		wait(100);
	}
	// 进入暂停状态后,可以随意更改资源 不需要加锁
	pthread_mutex_lock(&work_list_mutex_);
	int16_t cur_id_num = detector->getIdCount();
	int cur_origin_num = detector->getCount();
	if(index > cur_index_){
		// 往后设置, pop掉若干个 设置id
		int remain = index - cur_index_;

		// ID
		int16_t first_num = work_count_list_.front() - cur_id_num + 1;
		if(remain < first_num){
			camera->setCount(cur_id_num + remain);
			detector->setIdCount(cur_id_num + remain);
			remain = -1;
		}else{
			work_name_list_.pop_front();
			work_count_list_.pop_front();
			remain -= first_num;
		}
		while(remain>0){
			int16_t id_num;
			id_num = work_count_list_.front();
			if(remain > id_num){
				// 不是当前id
				work_name_list_.pop_front();
				work_count_list_.pop_front();
				remain -= id_num;
			}else{
				camera->setCount(remain + 1);
				detector->setIdCount(remain + 1);
				break;
			}
		}

		// origin image
		remain = index - cur_index_;
		first_num = batch_count_list_.front() - cur_origin_num + 1;
		if(remain < first_num){
			detector->setCount(cur_origin_num + remain);
			remain = -1;
		}else{
			batch_origin_list_.pop_front();
			batch_count_list_.pop_front();
			remain -= first_num;
		}
		while(remain > 0){
			int16_t origin_num;
			origin_num = batch_count_list_.front();
			if(remain > origin_num){
				// 不是当前原图
				batch_origin_list_.pop_front();
				batch_count_list_.pop_front();
				remain -= origin_num;
			}else{
				detector->setCount(remain);
				break;
			}
		}
	}else{
		// 回溯, push front若干个 设置id
		int remain = cur_index_ - index;
		int id_index = work_list_.size() - work_name_list_.size();

		// id
		int16_t first_num = cur_id_num;
		if(remain < first_num){
			camera->setCount(cur_id_num - remain);
			detector->setIdCount(cur_id_num - remain);
			remain = -1;
		}else{
			string filename = "";
			if( !work_list_[id_index].reprint_flag )
				filename = "print_" + to_string(work_list_[id_index].id) + "_";
			else
				filename = "reprint_" + to_string(work_list_[id_index].id) + "_" + to_string(work_list_[id_index].page);
			work_name_list_.push_front(filename);
			work_count_list_.push_front(work_list_[id_index].reprint_flag ? 1 : work_list_[id_index].page);
			remain -= first_num;
			--id_index;
		}
		while(remain >= 0){
			int16_t id_num;
			id_num = work_list_[id_index].reprint_flag ? 1 : work_list_[id_index].page;
			if(remain > id_num){
				// 不是当前id
				string filename = "";
				if( !work_list_[id_index].reprint_flag )
					filename = "print_" + to_string(work_list_[id_index].id) + "_";
				else
					filename = "reprint_" + to_string(work_list_[id_index].id) + "_" + to_string(work_list_[id_index].page);
				work_name_list_.push_front(filename);
				work_count_list_.push_front(id_num);
				remain -= id_num;
				--id_index;
			}else{
				camera->setCount(work_count_list_.front() - remain);
				detector->setIdCount(work_count_list_.front() - remain);
				break;
			}
		}

		// origin image
		remain = cur_index_ - index;
		int origin_index = origin_list_.size() - batch_origin_list_.size();
		first_num = cur_origin_num;
		if(remain < first_num){
			detector->setCount(cur_origin_num-remain);
		}else{
			batch_origin_list_.push_front(origin_list_[origin_index].location);
			batch_count_list_.push_front(origin_list_[origin_index].quantity);
			remain -= first_num;
			--origin_index;
		}
		while(remain >= 0){
			int16_t origin_num;
			origin_num = batch_count_list_.front();
			if(remain > origin_num){
				// 不是当前原图
				batch_origin_list_.push_front(origin_list_[origin_index].location);
				batch_count_list_.push_front(origin_list_[origin_index].quantity);
				remain -= origin_num;
				--origin_index;
			}else{
				detector->setCount(batch_count_list_.front()-remain);
				break;
			}
		}
	}
	cur_index_ = index;	cur_id_ = jobid; cur_page_ = curpage;
	camera->resetBias();
	std::queue<string> empty_queue;
	swap(empty_queue, unsolved_list_);
	detector->restart();
	camera->restart();
	informCurState();
	sLog->logDebug("set task finish");
	pthread_mutex_unlock(&work_list_mutex_);
	return 0;
}

// SECTION util functions
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
		id = std::stoll(split_name[1]);
        page = std::stoi(split_name[2]);
    }else{
        // 新打
        id  = std::stoll(split_name[1]);
    }
	return page;
}

std::string System::getJsonWorkList(){
	Json::Value res;
	if(work_list_.empty()){
		sLog->logDebug("%s",res.toStyledString().c_str());
		return res.toStyledString();
	}
	Json::StreamWriterBuilder wbuilder;
    wbuilder["indentation"] = "";
	std::unique_ptr<Json::StreamWriter> writer(wbuilder.newStreamWriter());
	Json::Value element;
	for(auto work:work_list_){
		element["id"] = work.id;
		element["is_reprint"] = work.reprint_flag;
		element["num"] = work.reprint_flag ? 1 : work.page;
		res.append(element);
	}
	std::string s = Json::writeString(wbuilder, res);
	sLog->logDebug("Get json worklist: %s", s.c_str());
	return s;
}

std::string System::getJsonOriginList(){
	Json::Value res;
	if(origin_list_.empty()){
		sLog->logDebug("%s",res.toStyledString().c_str());
		return res.toStyledString();
	}
	Json::StreamWriterBuilder wbuilder;
    wbuilder["indentation"] = "";
	std::unique_ptr<Json::StreamWriter> writer(wbuilder.newStreamWriter());
	Json::Value element;
	for(auto work:origin_list_){
		element["pos"] = work.location;
		element["num"] = work.quantity;
		res.append(element);
	}
	std::string s = Json::writeString(wbuilder, res);
	sLog->logDebug("Get json origin list: %s", s.c_str());
	return s;
}

int System::readNetParam(){
	std::ifstream paramfile;
	paramfile.open("../settings.json", std::ios::binary);
    if(!paramfile){
        sLog->logError("failed to open settings.json");
        return -1;
    }else{
        Json::CharReaderBuilder builder;
		Json::Value root;
        builder["collectComments"] = false;
        JSONCPP_STRING errs;
        if (parseFromStream(builder, paramfile, &root, &errs)){
			client_host_ = root["communication"]["client_host"].empty() ? "127.0.0.1" : root["communication"]["client_host"].asString();
			server_port_ = root["communication"]["server_port"].empty() ? 7999 : root["communication"]["server_port"].asInt();
			client_port_ = root["communication"]["client_port"].empty() ? 7999 : root["communication"]["client_port"].asInt();
			result_url_  = root["communication"]["result_url"].empty() ? "/api/Print/addErrorReport" : root["communication"]["result_url"].asString();
			int log_level = root["log"]["level"].empty() ? 0 : root["log"]["level"].asInt();
			sLog->setLevel(log_level);
			sLog->logInfo("client host %s:%d,%d",client_host_.c_str(),client_port_,server_port_);
			// cout << client_host_<<":"<<client_port_<<","<<server_port_ << endl;
        }else{
			sLog->logError("Jsoncpp error: %s", errs.c_str());
            // cout << "[ERROR] Jsoncpp error: " << errs << endl;
        }
        paramfile.close();
    }
	return 0;
}
// !SECTION 

// SECTION Client functions
int System::downloadFileList(std::vector<string> file_list){
	int nCode = -1;
	std::string sIP = client_host_;
	unsigned int nPort = client_port_;
	std::string sUser = "";   //可为空
	std::string sPwd = "";	  //可为空	

	sLog->logInfo("web client %s:%u", sIP.c_str(), nPort);

	//这边用智能指针更好
	CurlClient* pCurlClient = new CurlClient(sIP, nPort, sUser, sPwd);
	if(NULL == pCurlClient)
	{
		//创建Curl对象失败
		sLog->logError("curl: new object failure!!!!!");
		return -1;
	}
    
    //curl初始化
    nCode = pCurlClient->initCurlResource();
    if(0 != nCode)
    {
        sLog->logError("curl init failure!!!!!");
        delete pCurlClient;
        pCurlClient = NULL;
        return -1;
    }
	
	//下载文件名
	for(auto &file_url:file_list){
		std::string sUrlPath = "/" + file_url;

		sLog->logInfo("download url path: %s",file_url.c_str());

		pCurlClient->setUrlPath(sUrlPath);
		std::string sFileName = "../images/templates/";
		sFileName.append(file_url);
		int nFormat = 0;

		std::string sLocalFile = "../images/templates/";
		std::vector<std::string> substring;
		SplitString(file_url,substring,"/");
		int sublen = substring.size();
		if( sublen < 2 ){
			sLog->logError("cannot split download filename");
		}else{
			sLocalFile.append(substring[sublen-2]);
			sLocalFile.append("/");
			sLocalFile.append(substring[sublen-1]);
			nCode = pCurlClient->downloadFile(sFileName, sLocalFile, nFormat);
			// nCode = pCurlClient->downloadFile(sFileName, nFormat);
			sLog->logDebug("%s download status %d", sFileName.c_str(), nCode);
		}
	}
		
	delete pCurlClient;
    
    return nCode;

}
// !SECTION 

// SECTION HTTP Server functions
bool System::handle_signal(std::string url, std::string body, mg_connection *c, OnRspCallback rsp_callback)
{
	// do sth
	sLog->logInfo("handle start signal");
	sLog->logDebug("body: %s", body.c_str());

    int ret = readBatchFile("batch_info.json", work_name_list_, work_count_list_, batch_origin_list_, batch_count_list_);
	rsp_callback(c, "recived");
	return true;
}

bool System::handle_result(std::string url, std::string body, mg_connection *c, OnRspCallback rsp_callback){
	sLog->logInfo("handle result");

	rsp_callback(c, "{ \"StatusCode\": 1 }");

	return true;
}

bool System::handle_setTask(std::string url, std::string body, mg_connection *c, OnRspCallback rsp_callback){
	sLog->logInfo("handle refresh");
	sLog->logDebug("body: %s", body.c_str());

	std::stringstream ss(body);
	Json::CharReaderBuilder builder;
	Json::Value root;
	JSONCPP_STRING errs;
	builder["collectComments"] = false;
	if (parseFromStream(builder, ss, &root, &errs)){
		int index = root["index"].asInt();
		int id = root["id"].asInt64();
		int page = root["page"].asInt();
		setCurWork(index+1, id, page);
	}else{
		sLog->logError("Jsoncpp error: %s", errs.c_str());
	}
	

	rsp_callback(c, "done");
	return true;
}

bool System::handle_start(std::string url, std::string body, mg_connection *c, OnRspCallback rsp_callback){
	sLog->logInfo("handle start");
	sLog->logDebug("body: %s", body.c_str());

	int return_state = 0;
	std::vector<std::string> download_list;

	std::stringstream ss(body);
	Json::CharReaderBuilder builder;
	Json::Value root;
	JSONCPP_STRING errs;
	builder["collectComments"] = false;
	if (parseFromStream(builder, ss, &root, &errs)){
		int work_size = root["printwork"].size();
		if(work_size > 0){
			for(int i = 0; i < work_size; i++){
				int64_t work_id = root["printwork"][i]["printJobID"].asInt64();
				int work_count = root["printwork"][i]["printQuantity"].asInt();
				// 判断文件夹是否存在
				std::string origin_dir = "../images/templates/";
				std::string dir = "../images/templates/" + to_string(work_id);
				// origin_dir.append(to_string(work_id));
				if(isDirExist(dir) == -1){
					// 下载所有原图，返回错误
					return_state = -1;
					mkdir(dir.c_str(),S_IRWXU|S_IRWXG|S_IRWXO);
				}
				
				if(root["printwork"][i]["pictureLink"].isString()){
					// 图像是string 所有都是这个图

					std::string file_url = root["printwork"][i]["pictureLink"].asString();
					
					std::string localfile="";
					std::vector<std::string> substring;
					SplitString(file_url,substring,"/");
					int sublen = substring.size();
					if( sublen < 2 ){
						sLog->logError("Cannot split download filename");
					}else{
						localfile.append(substring[sublen-2]);
						localfile.append("/");
						localfile.append(substring[sublen-1]);
					}
					
					OriginInfo oinfo{localfile,work_count};
					origin_list_.push_back(oinfo);

					batch_count_list_.push_back(work_count);
					batch_origin_list_.push_back(localfile);

					if(return_state == -1)
						download_list.push_back(root["printwork"][i]["pictureLink"].asString());
					else{
						std::string filename = origin_dir + localfile;
						if(access(filename.c_str(),F_OK) == -1){
							return_state = -3;
							download_list.push_back(root["printwork"][i]["pictureLink"].asString());
							sLog->logWarn("%s does not exist, try to download", filename.c_str());
						}
					}
					sLog->logInfo("set batch num: %d", work_count);
					sLog->logInfo("set origin image url %s", root["printwork"][i]["pictureLink"].asString().c_str());
					std::string work_name = "print_";
					work_name.append(to_string(work_id));
					work_name_list_.push_back(work_name);
					work_count_list_.push_back(work_count);
					
					WorkInfo winfo;
					winfo.id = work_id;
					winfo.page = work_count;
					work_list_.push_back(winfo);
					
					sLog->logInfo("set work name: %s", work_name.c_str());
					sLog->logInfo("set work count: %d", work_count);
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
						winfo.page = work_count;
						work_list_.push_back(winfo);
						sLog->logInfo("set work name: %s", work_name.c_str());
						sLog->logInfo("set work count: %d", work_count);
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
							winfo.page = reprint_id;
							winfo.reprint_flag=true;
							work_list_.push_back(winfo);
							sLog->logInfo("set work name: %s", work_name.c_str());
							sLog->logInfo("set work count: %d", 1);
						}

						std::string file_url = root["printwork"][i]["pictureLink"][j].asString();
						std::string localfile="";

						std::vector<std::string> substring;
						SplitString(file_url,substring,"/");
						int sublen = substring.size();
						if( sublen < 2 ){
							sLog->logError("Cannot split download filename");
						}else{
							localfile.append(substring[sublen-2]);
							localfile.append("/");
							localfile.append(substring[sublen-1]);
						}
						
						OriginInfo oinfo{localfile,1};
						origin_list_.push_back(oinfo);

						batch_count_list_.push_back(1);
						batch_origin_list_.push_back(localfile);
						if(return_state == -1){
							std::string down_filename;
							// down_filename.append("/");
							down_filename = root["printwork"][i]["pictureLink"][j].asString();
							download_list.push_back(down_filename);
						}else{
							std::string filename = origin_dir + localfile;
							if(access(filename.c_str(),F_OK) == -1){
								return_state = -3;
								download_list.push_back(root["printwork"][i]["pictureLink"][j].asString());
								sLog->logWarn("%s does not exist, try to download", filename.c_str());
							}
						}
						sLog->logInfo("set batch num: %d", 1 );
						sLog->logInfo("set origin image url %s", root["printwork"][i]["pictureLink"][j].asString().c_str());
					}
				}
			}
		}else{
			sLog->logError("no work array");
			return_state = -2;
		}
	}else{
		sLog->logError("Jsoncpp error: %s", errs.c_str());
	}
	
	std::string response = "{ \"StatusCode\": " + to_string(return_state) + "}";
	rsp_callback(c, response);

	if(return_state == -1 || return_state == -3){
		wait(1);
		sLog->logInfo("start download");
		downloadFileList(download_list);
	}


	return true;
}

bool System::handle_changeMod(std::string url, std::string body, mg_connection *c, OnRspCallback rsp_callback){
	sLog->logInfo("handle change mode");
	sLog->logDebug("body: %s", body.c_str());
	int ret = 0;
	if(body == "hard"){
		ret = camera->setTrigger(HARD_TRIGGER);
		sLog->logInfo("set hardware trigger: %d", ret);
	}else if(body == "soft"){
		ret = camera->setTrigger(SOFT_TRIGGER);
		sLog->logInfo("set software trigger: %d", ret);
	}else{
		rsp_callback(c, "wrong command");
		return false;
	}
	if(ret == 0)
		rsp_callback(c, "done");
	else
		rsp_callback(c, "failed");
	return true;
}

bool System::handle_softTrigger(std::string url, std::string body, mg_connection *c, OnRspCallback rsp_callback){
	int ret = camera->sendSoftTrigger();
	sLog->logDebug("The return value of softtrigger command: %d", ret);
	return true;
}

bool System::handle_websocketConnect(){
	informWorkList();
	informCurState();
	string msg = "trigger:";
	if(camera->isSoftTrigger())
		msg += "1";
	else
		msg += "0";
	server_->SendWebsocketMsg(msg);
	return true;
}

void* System::postResultWrapper(void * arg){
	System * thisptr= (System *) arg;
	thisptr->postResult();
	return nullptr;
}

void System::postResult(){
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
        sLog->logError("new object failure!!!!!");
    }
    
    //curl初始化
    nCode = pCurlClient->initCurlResource();
    if(0 != nCode)
    {
        sLog->logError("curl init failure!!!!!");
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
            sLog->logDebug("send result %s", res.c_str());
        }
		wait(10);
    }
}

void* System::startServerWrapper(void *arg){
	System* this_ptr= (System *) arg;
	this_ptr->startServer();
	return nullptr;
}

void System::startServer(){
	server_->Start();
}
