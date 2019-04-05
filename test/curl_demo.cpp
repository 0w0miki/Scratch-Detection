#include <memory>
#include "http_client.h"

// CURL *curl;
// CURLcode res;

using namespace std;

struct thread_param
{
    Json::Value* root;
    pthread_mutex_t* mutex;
};

/*
	* 通过get 、delete方式发送数据
    * 返回值int, 0表示成功,1表示超时, 其它表示失败
*/
int curl_get_message()
{
	int nCode = -1;
	std::string sIP = "127.0.0.1";
	unsigned int nPort = 7999;
	std::string sUser = "";   //可为空
	std::string sPwd = "";	  //可为空	

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
	std::string sUrlPath = "/api/start_signal";
	pCurlClient->setUrlPath(sUrlPath);
	
	//发送方式为get,数据格式为默认,内容为空
	int nMethod = METHOD_GET;
    //int nMethod = METHOD_DELETE;
	int nFormat = FORMAT_JSON;
	std::string sMsg;
	std::string sRec;
	nCode = pCurlClient->sendMsg(sMsg, nMethod, nFormat,sRec);
	printf("%d\n", nCode);
	printf("%s\n", sRec.c_str());
		
	delete pCurlClient;
	
}
 
/*
	* 通过post 、put方式发送数据
    * 返回值int, 0表示成功,1表示超时, 其它表示失败
*/
int curl_post_message()
{
	int nCode = -1;
	std::string sIP = "127.0.0.1";
	unsigned int nPort = 7999;
	std::string sUser = "";   //可为空
	std::string sPwd = "";	  //可为空	
 
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
	std::string sUrlPath = "/api/result";
	pCurlClient->setUrlPath(sUrlPath);
	
	//发送方式为post,数据格式为json,发送数据为json
	int nMethod = METHOD_POST;
    //int nMethod = METHOD_PUT;
	int nFormat = FORMAT_JSON;
    std::string sRec;
	std::string sMsg = "{";
				sMsg += "\"UserName\":\"user\",";
				sMsg += "\"UserPwd\":\"pwd\"";
				sMsg += "}";
	
	nCode = pCurlClient->sendMsg(sMsg, nMethod, nFormat,sRec);
	printf("%d\n", nCode);
		
	delete pCurlClient;
    return nCode;
	
}
 
/*
	* 上传文件
    * 返回值int, 0表示成功,1表示超时, 其它表示失败
*/
int curl_upload_file()
{
	int nCode = -1;
	std::string sIP = "127.0.0.1";
	unsigned int nPort = 7999;
	std::string sUser = "";   //可为空
	std::string sPwd = "";	  //可为空	
 
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
	std::string sUrlPath = "/api/result";
	pCurlClient->setUrlPath(sUrlPath);
	
	//上传文件名
	std::string sFileName = "../results.json";
	std::string sRec;
	nCode = pCurlClient->uploadFile(sFileName, sRec);
	printf("%d\n", nCode);
	printf("%s\n", sRec.c_str());
		
	delete pCurlClient;
    
    return nCode;
	
}
 
/*
	* 下载文件
    * 返回值int, 0表示成功,1表示超时, 其它表示失败
*/
int curl_download_file()
{
	int nCode = -1;
	std::string sIP = "img.aiezu.com/";
	unsigned int nPort = 80;
	std::string sUser = "";   //可为空
	std::string sPwd = "";	  //可为空	
 
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
	std::string sUrlPath = "avatar/000/00/00/01_avatar_mid.jpg";
	pCurlClient->setUrlPath(sUrlPath);
	
	//下载文件名
	std::string sFileName = "./test.jpg";
    int nFormat = 0;
	nCode = pCurlClient->downloadFile(sFileName, nFormat);
	printf("%d\n", nCode);
		
	delete pCurlClient;
    
    return nCode;
}

void* post_result(void *arg){
    
    thread_param* param_ptr;
    param_ptr = (struct thread_param *) arg;
    Json::Value* result_root = param_ptr->root;
    pthread_mutex_t* result_mutex = param_ptr->mutex;
    cout<<"root2: "<<result_root<<endl;
    cout<<"address2: "<< result_mutex<<endl;
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
        }
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 10 * 1000;
        select(0,NULL,NULL,NULL,&tv);
    }
}

//回调函数
size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream) 
{
    string data((const char*) ptr, (size_t) size * nmemb);

    *((stringstream*) stream) << data << endl;

    return size * nmemb;
}

int http_post_file(const char *url, const char *filename){
    CURL *curl = NULL;
    CURLcode res;

    struct curl_httppost *post=NULL;
    struct curl_httppost *last=NULL;
    curl_slist *headers=NULL;
    bool out = false;
    if(filename == NULL || url == NULL){
        printf("<-----------no url or filename----------->\n");
        return -1;
    }

    printf("URL: %s\n", url);
    printf("filename: %s\n", filename);
    headers = curl_slist_append(headers, "Content-Type: application/json");
    /* Add simple file section */
    if( curl_formadd(&post, &last, CURLFORM_COPYNAME, "result",
               CURLFORM_FILECONTENT, filename, 
               CURLFORM_CONTENTHEADER, headers,
               CURLFORM_END) != 0)
    {
        fprintf(stderr, "curl_formadd error.\n");
        out = true;
    }
    if(out == false){
    //curl_global_init(CURL_GLOBAL_ALL);
        curl = curl_easy_init();
        if(curl == NULL)
        {
            fprintf(stderr, "curl_easy_init() error.\n");
            out = true;
        }
    }

    if(out == false){
        // curl_easy_setopt(curl, CURLOPT_HEADER, headers);
        curl_easy_setopt(curl, CURLOPT_URL, url); /*Set URL*/
        curl_easy_setopt(curl, CURLOPT_HTTPPOST, post);
        int timeout = 5;
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
        // curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
        // curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 1);

        res = curl_easy_perform(curl);
        if(res != CURLE_OK)
        {
            fprintf(stderr, "curl_easy_perform[%d] error.\n", res);
            out = true;
        }
    }

    if(out == false){
        curl_easy_cleanup(curl);
    }
    curl_formfree(post);
    curl_slist_free_all(headers);
    //curl_global_cleanup();
    return 0;
}

int check_file(char *filename)
{
    ifstream fd;
    fd.open(filename,std::ios::in);
    // fd = open(filename, O_RDONLY);
    if(!fd)
    {
        perror("can not open file");
        return -1;
    }
    fd.seekg(0,ios::end);
    int file_len = fd.tellg();
    printf("file length: %d",file_len);
    fd.close();
    return file_len;
}

int http_post_json(const char *url, const Json::Value json_value){
    CURL *curl = NULL;
    CURLcode res;

    Json::StreamWriterBuilder wbuilder;
    wbuilder["indentation"] = "";
    std::string post_str = Json::writeString(wbuilder, json_value);
    post_str = post_str;
    // std::string post_str = json_value.toStyledString();

    if(json_value == NULL || url == NULL){
        printf("<-----------no url or filename----------->\n");
        return -1;
    }

    printf("URL: %s\n", url);
    printf("post_str: %s\n", post_str.c_str());
    
    /* Add simple file section */
    curl = curl_easy_init();
    if(curl == NULL)
    {
        fprintf(stderr, "curl_easy_init() error.\n");
    }

    stringstream out;
    int response = 0;
    curl_slist *headers = curl_slist_append(NULL, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HEADER, 0);

    curl_easy_setopt(curl, CURLOPT_POST, 1);//设置为非0表示本次操作为POST
    curl_easy_setopt(curl, CURLOPT_URL, url); /*Set URL*/
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_str.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, post_str.size());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &out);
    int timeout = 1;
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
    // curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    // curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 1);

    res = curl_easy_perform(curl);
    if(res != CURLE_OK)
    {
        fprintf(stderr, "curl_easy_perform[%d] error.\n", res);
    }

    std::cout<<"===============test=============="<<std::endl;
    std::cout<<out.str()<<std::endl;
    Json::CharReaderBuilder builder;
    Json::Value root;
    builder["collectComments"] = false;
    JSONCPP_STRING errs;
    if (parseFromStream(builder, out, &root, &errs)){
        std::cout<<root.toStyledString()<<std::endl;
        if(root.empty()){
            response = -4;
        }else{
            if(root["statusCode"].empty()){
                response = -5;
            }else if(root["statusCode"].isInt()){
                response = root["statusCode"].asInt();
            }else{
                response = -102;
            }
        }
    }else{
        std::cout<<root.size();
        std::cout<<"parse failed"<<std::endl;
        response = -3;
    }

    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
    //curl_global_cleanup();
    return response;
}

int http_get(const char *url){
    CURL *curl = NULL;
    CURLcode res;

    if(url == NULL){
        printf("<-----------no url----------->\n");
        return -1;
    }

    printf("URL: %s\n", url);
    
    /* Add simple file section */
    curl = curl_easy_init();
    if(curl == NULL)
    {
        fprintf(stderr, "curl_easy_init() error.\n");
        return -2;
    }
    std::stringstream out;

    curl_easy_setopt(curl, CURLOPT_URL, url); /*Set URL*/
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &out);
    int timeout = 5;
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
    // curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    // curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 1);

    res = curl_easy_perform(curl);
    if(res != CURLE_OK)
    {
        fprintf(stderr, "curl_easy_perform[%d] error.\n", res);
    }
    std::cout<<"===============out=============="<<std::endl;
    std::cout<< out.str()<<std::endl;
    curl_easy_cleanup(curl);
    //curl_global_cleanup();
    return 0;
}

//POST json
int main()
{
    pthread_t post_thread_id = 3;
    pthread_mutex_t result_mutex;
    pthread_mutex_init(&result_mutex, NULL);
    Json::Value* result_root = new Json::Value();
    cout<<"root1: "<<result_root<<endl;
    cout<<"address1: "<< &result_mutex<<endl;
    CurlClient::globalInit();

    thread_param post_thread_param;
    post_thread_param.root = result_root;
    post_thread_param.mutex = &result_mutex;
    
    pthread_create(&post_thread_id, NULL, post_result, (void*)&post_thread_param);
    
    bool run = true;
    while(run){
        int c = getchar();
        switch(c){
            case 'a':
            case 'A':
            {
                curl_post_message();
                // char *url = "http://127.0.0.1:7999/api/result";
                // char *filename = "/home/miki0w0/gree/results.json";
                // std::ifstream batch_file;
                // batch_file.open(filename , std::ios::in);
                // if(!batch_file){
                //     printf("[ERROR] failed to open batch file\n");
                //     return -1;
                // }else{
                //     Json::CharReaderBuilder builder;
                //     Json::Value root;
                //     builder["collectComments"] = false;
                //     JSONCPP_STRING errs;
                //     if (parseFromStream(builder, batch_file, &root, &errs)){
                //         std::cout<<root.toStyledString()<<std::endl;
                //         int ret = http_post_json(url, root);
                //         switch(ret){
                //             case 1:
                //                 // 收到
                //                 std::cout << "result is received." << std::endl;
                //                 break;
                //             case -101:
                //                 // 没收到
                //                 std::cout << "does not receive the result." << std::endl;
                //                 break;
                //             case -102:
                //                 std::cout << "response Stype is wrong." << std::endl;
                //                 break;
                //             default:
                //                 // 不在列表中的解析值
                //                 std::cout << "response not in list." << std::endl;
                //                 break;
                //         }
                //     }
                // }
                break;
            }
            case 's':
            case 'S':
            {
                char* url = "http://127.0.0.1:7999/api/start_signal";
                // http_get(url);
                curl_get_message();
                break;
            }
            case 'd':
            case 'D':
            {
                // char *url = "http://127.0.0.1:7999/api/result";
                // char *filename = "/home/miki0w0/gree/results.json";
                // http_post_file(url,filename);
                // curl_upload_file();
                curl_download_file();
                break;
            }
            case 'q':
            case 'Q':
            {
                Json::Value batch_item;
                Json::StreamWriterBuilder wbuilder;
                wbuilder["indentation"] = "";
                std::unique_ptr<Json::StreamWriter> writer(wbuilder.newStreamWriter());

                batch_item["work"] = 1;
                batch_item["id"] = 1;
                batch_item["state"] = "fault";
                batch_item["fault type"] = 1001;
                result_root->append(batch_item);
                std::cout << "'" << Json::writeString(wbuilder, *(result_root)) << "'" << std::endl;
                batch_item.clear();
                break;
            }
            case 'w':
            case 'W':
            {
                char *url = "http://127.0.0.1:7999/api/start";
                // char *url = "http://192.168.31.18:7999/api/start";
                char *filename = "/home/miki0w0/gree/batch_info.json";
                std::ifstream batch_file;
                batch_file.open(filename , std::ios::in);
                if(!batch_file){
                    printf("[ERROR] failed to open batch file\n");
                    return -1;
                }else{
                    Json::CharReaderBuilder builder;
                    Json::Value root;
                    builder["collectComments"] = false;
                    JSONCPP_STRING errs;
                    if (parseFromStream(builder, batch_file, &root, &errs)){
                        std::cout<<root.toStyledString()<<std::endl;
                        int ret = http_post_json(url, root);
                        switch(ret){
                            case 0:
                                // 收到
                                std::cout << "result is received." << std::endl;
                                break;
                            case -101:
                                // 没收到
                                std::cout << "does not receive the result." << std::endl;
                                break;
                            case -102:
                                std::cout << "response Stype is wrong." << std::endl;
                                break;
                            default:
                                // 不在列表中的解析值
                                std::cout << "response not in list." << std::endl;
                                break;
                        }
                    }
                }
                break;
            }
            case 'x':
            case 'X':
            case 27:
                run = false;
                break;
        }
    }

    // //构建json
    // std::ifstream batch_file;
    // batch_file.open(filename , std::ios::in);
    // if(!batch_file){
    //     printf("[ERROR] failed to open batch file\n");
    //     return -1;
    // }else{
    //     Json::CharReaderBuilder builder;
	//     Json::Value root;
    //     builder["collectComments"] = false;
    //     JSONCPP_STRING errs;
    //     if (parseFromStream(builder, batch_file, &root, &errs)){
    //         http_post_json(url, root);
    //     }
    // }
    
    // if(check_file(filename) == -1){
    //     printf("file cannot open");
    //     return 1;
    // }
    // http_post_file(url, filename);

    // curl = curl_easy_init();

    // if(curl)
    // {
        
        // item["type"]=Json::Value("Libcurl HTTP POST Json串");
        // item["authList"]=Json::Value("weidong0925@126.com");
        // std::string jsonout = item.toStyledString();

    //     // jsonout = AsciiToUtf8(jsonout);

    //     //设置url
    //     curl_easy_setopt(curl, CURLOPT_URL, url);

    //     //设置http发送的内容类型为JSON
    //     //构建HTTP报文头  
    //     // sprintf_s(tmp_str, "Content-Length: %s", jsonout.c_str());
        // snprintf(tmp_str,sizeof(tmp_str),"Content-Length: %s", jsonout.c_str());
        // headers=curl_slist_append(headers, "Content-Type:application/json;charset=UTF-8");
    //     //headers=curl_slist_append(headers, tmp_str);//在请求头中设置长度,请求会失败,还没找到原因

    //     curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    //     //curl_easy_setopt(curl,  CURLOPT_CUSTOMREQUEST, "POST");//自定义请求方式
    //     curl_easy_setopt(curl, CURLOPT_POST, 1);//设置为非0表示本次操作为POST

    //     // 设置要POST的JSON数据
    //     curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonout.c_str());
    //     curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, jsonout.size());//设置上传json串长度,这个设置可以忽略

    //     // 设置接收数据的处理函数和存放变量
    //     curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);//设置回调函数
    //     curl_easy_setopt(curl, CURLOPT_WRITEDATA, &out);//设置写数据
    //     res = curl_easy_perform(curl);//执行

    //     curl_slist_free_all(headers); /* free the list again */

    //     string str_json = out.str();//返回请求值 
    //     printf("%s",str_json.c_str()); 

    //     /* always cleanup */ 
    //     curl_easy_cleanup(curl);
    // } 

    return 0;
 }

// int main(int argc, char *argv[])
// {
    
//     // 基于当前系统的当前日期/时间
//     time_t now = time(0);
//     char* dt = ctime(&now);
//     cout << dt << "-------------------------------------" << endl;

//     /*HTTP GET json data*/
//     std::stringstream out;
//     void* curl = curl_easy_init();
//     // 设置URL
//     curl_easy_setopt(curl, CURLOPT_URL, "http://aiezu.com/test.php?en=aiezu&cn=爱E族");
//     // 设置接收数据的处理函数和存放变量
//     curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
//     curl_easy_setopt(curl, CURLOPT_WRITEDATA, &out);

//     // 执行HTTP GET操作
//     CURLcode res = curl_easy_perform(curl);
//     if (res != CURLE_OK) {
//         fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
//     }

//     // 接受数据存放在out中，输出之
//     //cout << out.str() << endl;
//     string str_json = out.str();


//     printf("%s",str_json.c_str());
//     curl_easy_cleanup(curl);

//     return 0;
// }