/*
    * curl的封装的具体实现，有以下操作：
    * 1.设置服务器参数
    * 2.发送信息：get、post、put、delete
    * 3.上传文件处理
    * 4.下载文件
*/
 
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include "http_client.h"
 
bool CurlClient::s_bGlobalInitStatus = 0;
pthread_mutex_t CurlClient::s_MutexLock;
 
/*
    * 默认构造函数
*/
CurlClient::CurlClient():m_pCurl(NULL),
            			   m_pHeaders(NULL),
                           m_sIP(),
                           m_nPort(),
                           m_sUser(),
                           m_sPwd(),
                           m_contect_timeout(0),
                           m_timeout(0)
{
 
}
 
 

/*
    * 构造函数，进行成员变量初始化
    * 参数1：请求的ip地址
    * 参数2：请求ip地址的端口
    * 参数3：对方验证用户名
    * 参数4：对方验证密码
*/
CurlClient::CurlClient(const std::string& sIP,
                         const unsigned int nPort,
                         const std::string& sUser,
                         const std::string& sPwd
                         )
                        
                        :m_pCurl(NULL),
                         m_pHeaders(NULL),
                         m_sIP(sIP),
                         m_nPort(nPort),
                         m_sUser(sUser),
                         m_sPwd(sPwd),
                         m_contect_timeout(0),
                         m_timeout(0)
{

}

/*
    * 构造函数，进行成员变量初始化
    * 参数1：请求的ip地址
    * 参数2：请求ip地址的端口
    * 参数3：对方验证用户名
    * 参数4：对方验证密码
*/
CurlClient::CurlClient(const std::string& sIP,
                         const unsigned int nPort,
                         const std::string& sUser,
                         const std::string& sPwd,
                         pthread_mutex_t* result_mutex,
                         Json::Value* result_root
                         )
                        
                        :m_pCurl(NULL),
                         m_pHeaders(NULL),
                         m_sIP(sIP),
                         m_nPort(nPort),
                         m_sUser(sUser),
                         m_sPwd(sPwd),
                         m_contect_timeout(0),
                         m_timeout(0)
{
    result_mutex_ = result_mutex;
    result_root_ = result_root;
}
 
 
/*
    * 析构函数，进行释放资源
*/
CurlClient::~CurlClient()
{
    try
    {
        if(NULL != this->m_pHeaders)
        {
            curl_slist_free_all(this->m_pHeaders);
            this->m_pHeaders = NULL;
        }
        
        if(NULL != this->m_pCurl)
        {
            releaseCurlResource();
        }
      
    }
    catch(...)    //吞下异常，防止异常逃离析构函数
    {
        printf("~CurlClient api exception error(%d) \n", errno);
    }
 
}
 
 
/*
    * 进行所有CURL开始之前的，全局变量初始化，放在主线程中
    * 返回值为int, 0表示成功，其他表示失败
*/
int CurlClient::globalInit()
{
    int nCode = -1;
    int nLockJudge = -1;
    int nUnlockJudge = -1;
 
    try
    {
        nLockJudge = pthread_mutex_lock(&s_MutexLock);
        nCode = nLockJudge;
        if(0 != nCode)
        {
            printf("globalInit mutex lock error(%d)\n", errno);
            return nCode;
        }
 
        if(0 == s_bGlobalInitStatus)
        {
 
            if(CURLE_OK == curl_global_init(CURL_GLOBAL_ALL))
            {
                //返回成功
                s_bGlobalInitStatus = 1;
            }
            else
            {
                printf("globalInit error(%d)\n", errno);
                nCode = -1;      //CURL全局资源初始化失败
            }
 
        }

        nUnlockJudge = pthread_mutex_unlock(&s_MutexLock);
 
        return nCode;
 
    }
    catch(...)
    {
        if(0 == nLockJudge && 0 != nUnlockJudge)
        {
            pthread_mutex_unlock(&s_MutexLock);
        }
 
        printf("global init api exception(%d)\n", errno);
        return -1;                           //异常接口
    }
 
}
 
/*
    * 进行所有CURL开始结束的，全局变量初始化，放在主线程中
    * 返回值为int, 0表示成功，其他表示失败
*/
int CurlClient::globalCleanup()
{
    int nLockJudge = -1;
    int nUnlockJudge = -1;
    int nCode = -1;
 
    try
    {
        nLockJudge = pthread_mutex_lock(&s_MutexLock);;
        nCode = nLockJudge;
        if(0 != nCode)
        {
            printf("globalCleanup mutex lock error(%d)\n", errno);
            return nCode;
        }
 
        if(1 == s_bGlobalInitStatus)
        {
            curl_global_cleanup();
            s_bGlobalInitStatus = 0;
        }
 
        nUnlockJudge = pthread_mutex_unlock(&s_MutexLock);;
        nCode = nUnlockJudge;
 
        return nCode;
 
    }
    catch(...)
    {
        if(0 == nLockJudge && 0 != nUnlockJudge)
        {
            pthread_mutex_unlock(&s_MutexLock);
        }
        printf("globalCleanup api exception(%d)\n", errno);
        return -1;                           //异常接口
    }
 
}
 
/*
    * 进行单个线程CURL简单资源进行初始化
    * 返回值int, 0表示成功,　其它表示失败
*/
int CurlClient::initCurlResource()
{
    this->m_pCurl = curl_easy_init();
    if(NULL == this->m_pCurl)
    {
        printf("curl easy init failure \n");
        return -1;
    }
    else
    {
        return 0;
    }
    
}
 
 
/*
    * 进行单个线程CURL简单资源进行释放
    * 返回值为int, 0表示成功，其他表示失败
*/
int CurlClient::releaseCurlResource()
{
    //判断参数的合法性
    if(NULL == this->m_pCurl)
    {
        printf("releaseCurlResource curl ptr is null \n");
        return -1;       //CURL指针为NULL
    }
    
    //释放curl指针
    curl_easy_cleanup(this->m_pCurl);
    this->m_pCurl = NULL;
 
    return 0;
}
 
 
/*
    * 设置传输的用户和密码验证
    * 参数1：curl指针，通过此指针进行设置用户密码操作
    * 返回值int, 0表示成功，其他表示失败
*/
int CurlClient::setUserPwd()
{
    if(NULL == this->m_pCurl)
    {
        printf("setUserPwd curl ptr is null \n");
        return -1;       //CURL指针为NULL
    }
 
    std::string sUserPwd;
    if((!(this->m_sUser.empty())) && (!(this->m_sPwd.empty())))
    {
        sUserPwd = this->m_sUser + ":" + this->m_sPwd;
        curl_easy_setopt(this->m_pCurl, CURLOPT_USERPWD, (char*)sUserPwd.c_str());
    }
    return 0;
}
 
 
/*
    * 设置等待连接最长时间
    * 参数1:最长时间秒数
*/
void CurlClient::setConnectTimeout(unsigned int nTime)
{
    this->m_contect_timeout = nTime;
}
 
/*
    * 设置传输最长时间
    * 参数1:最长时间秒数
*/
void CurlClient::setTimeout(unsigned int nTime)
{
    this->m_timeout = nTime;
}
 
 
/*
    * 宏定义数据格式字符串
*/
//XML格式
#define  XML_FORMAT_STRING        "Content-Type: application/xml;charset=UTF-8"
//JSON格式
#define  JSON_FORMAT_STRING       "Content-Type: application/json;charset=UTF-8"
 
 
/*
    * 设置数据格式
    * 参数1：CURL指针
    * 参数2：发送数据格式,0默认,1为xml,2为json
    * 返回值int,0表示成功,其它表示失败
*/
int CurlClient::setDataFormat(const int nFormat)
{
    if(NULL == this->m_pCurl)
    {
        printf("setDataFormat curl ptr is null \n");
        return -1;       //CURL指针为NULL
    }
 
    std::string sFormatStr;
    if(FORMAT_XML == nFormat)
    {
        sFormatStr =  XML_FORMAT_STRING;
    }
    else if(FORMAT_JSON == nFormat)
    {
        sFormatStr =  JSON_FORMAT_STRING;
    }
    
    if(!sFormatStr.empty())
    {
        //非空设置相应格式,空则不设置
        this->m_pHeaders = curl_slist_append(NULL, (char*)sFormatStr.c_str());
        if(NULL == this->m_pHeaders)
        {
            printf("setDataFormat set format error(%d) \n", errno);
            return -1;
        }
        curl_easy_setopt(this->m_pCurl, CURLOPT_HTTPHEADER, this->m_pHeaders);
    }
    
    return 0;
}
 
 
/*
    * 设置服务器参数
    * 参数1：服务器ip
    * 参数2：服务器ip相关端口
    * 参数3：服务器用户名
    * 参数4：服务器密码
*/
void CurlClient::setServer(const std::string& sIP,
                            const unsigned int nPort,
                            const std::string& sUser,
                            const std::string& sPwd)
{
    this->m_sIP = sIP;
    this->m_nPort = nPort;
    this->m_sUser = sUser;
    this->m_sPwd = sPwd;
}
 
/*
    * 设置URL路径
    * 参数1：URL路径
*/
void CurlClient::setUrlPath(const std::string& sUrlPath)
{
    this->m_sUrlPath = sUrlPath;
}
 
/*
    * 回调函数，处理返回的数据
    * 参数1：缓存存放
    * 参数2：缓存块数
    * 参数3：缓存每块大小
    * 参数4：WRITEDATA对应的数据流
    * 返回值, 数据所占字节数
*/
size_t CurlClient::httpDataWriter(void* buffer, size_t size, size_t nmemb, void* content)
{
    long totalSize = size*nmemb;
    std::string* symbolBuffer = (std::string*)content;
    if(symbolBuffer)
    {
        symbolBuffer->append((char *)buffer, ((char*)buffer)+totalSize);
        return totalSize;
    }
    else
    {
        printf("CurlClient httpDataWriter data error(%d) \n", errno);
        return 0;
    }
 
}
 
/*
    * 回调函数，上传文件处理，读文件
    * 参数1：缓存存放
    * 参数2：缓存块数
    * 参数3：缓存每块大小
    * 参数4：READDATA对应的数据流
    * 返回值，数据所占字节数
*/
size_t CurlClient::httpReadFile(void* buffer, size_t size, size_t nmemb, void* file)
{
    return fread(buffer, size, nmemb, (FILE *)file);
}
 
/*
    * 回调函数，下载文件处理，写文件
    * 参数1：缓存存放
    * 参数2：缓存块数
    * 参数3：缓存每块大小
    * 参数4：WRITEDATA对应的数据流
    * 返回值，数据所占字节数
*/
size_t CurlClient::httpWriteFile(void* buffer, size_t size, size_t nmemb, void* file)
{
    return fwrite(buffer, size, nmemb, (FILE *)file);
}
 
/*
    * 数据发送方式
    * 参数1：要发送的数据
    * 参数2：发送的方法：0使用GET, 1使用POST, 2使用PUT, 3使用DELETE
    * 参数3：数据头格式,0表示默认,1表示xml,2为json
    * 参数4：返回的数据信息
    * 返回值int, 0表示成功, 1表示超时,其他表示失败
*/
int CurlClient::sendMsg(const std::string& sMsg,
                         const int nMethod,
                         const int nFormat,
                         std::string& sRec)
{
    int nCode = -1;
    sRec = "";  //清空数据
    switch(nMethod)
    {
        case METHOD_GET:        //使用GET方法传送数据
        {
            nCode = getMsg(sMsg, nFormat, sRec);
            return nCode;
        }
        case METHOD_POST:       //使用POST方法传送数据
        {
            nCode = postMsg(sMsg, nFormat, sRec);
            return nCode;
        }
        case METHOD_PUT:        //使用PUT方法传送数据
        {
            nCode = putMsg(sMsg, nFormat, sRec);
            return nCode;
        }
        case METHOD_DELETE:     //使用DELETE方法传送数据
        {
            nCode = deleteMsg(sMsg, nFormat, sRec);
            return nCode;
        }
        default:
        {
            printf("sendMsg method error\n");
            return -1;
        }
    }
}

/* 
    * 发送
*/
int CurlClient::sendResult(std::string &sRec){
    pthread_mutex_lock(result_mutex_);
    if(!result_root_->empty()){
        Json::StreamWriterBuilder wbuilder;
        wbuilder["indentation"] = "";
        std::string post_str = Json::writeString(wbuilder, *result_root_);
        sRec = "";
        sendMsg(post_str, METHOD_POST, FORMAT_JSON, sRec);

        // response 检查
        std::stringstream ss(sRec);
        Json::CharReaderBuilder builder;
        Json::Value root;
        builder["collectComments"] = false;
        JSONCPP_STRING errs;
        if (parseFromStream(builder, ss, &root, &errs)){
            if(!root.empty()){
                if(root["StatusCode"].empty()){
                    printf("[ERROR] no StatusCode term");
                }else if(root["StatusCode"].isString()){
                    std::string response = root["StatusCode"].asString();
                    if(!response.empty()){
                        // 收到请求
                        result_root_->clear();
                    }
                }else{
                    result_root_->clear();
                    printf("[ERROR] response is not string");
                }
            }else{
                printf("[ERROR] nothing in response");
            }
        }else{
            std::cout << "[ERROR] Jsoncpp error: " << errs << std::endl;
        }
    }
    pthread_mutex_unlock(result_mutex_);
}

/*
    * CURL公共操作
    * 参数1：curl指针，通过此指针进行相关设置
    * 返回值int，0表示成功，其他表示失败
*/
int CurlClient::messagePublicMethod(const int nFormat)
{
    int nCode = -1;
    try
    {
        if(NULL == this->m_pCurl)
        {
            printf("messagePublicMethod curl ptr is null\n");
            return -1;       //CURL指针为NULL
        }
		
		//参数合法性检测
		if(0 > nFormat)
		{
			printf("messagePublicMethod params error, nFormat=%d \n", nFormat);
            return -1;       //CURL指针为NULL
		}
 
        //指定url
        if(this->m_sIP.empty())
        {
            printf("messagePublicMethod ip is empty\n");
            return -1;
        }
        std::string sUrl;
        sUrl = sUrl + "http://" + this->m_sIP + this->m_sUrlPath;
        printf("sUrl: %s\n", sUrl.c_str());
        curl_easy_setopt(this->m_pCurl, CURLOPT_URL, (char*)sUrl.c_str());
        curl_easy_setopt(this->m_pCurl, CURLOPT_PORT, this->m_nPort);
 
        //设置用户名和密码
        nCode = setUserPwd();
        if(0 != nCode)
        {
            printf("messagePublicMethod setUserPwd error(%d) \n", errno);
            return -1;
        }
 
        //设置数据格式
        nCode = setDataFormat(nFormat);
        if(0 != nCode)
        {
            printf("messagePublicMethod setDataFormat error(%d) \n", errno);
            return -1;
        }
 
        //禁用掉alarm这种超时
        curl_easy_setopt(this->m_pCurl, CURLOPT_NOSIGNAL, 1);
 
        //设置超时时间
        if(0 >= this->m_contect_timeout)
        {
            this->m_contect_timeout = CONNECT_DEFAULT_TIMEOUT;
        }
        curl_easy_setopt(this->m_pCurl, CURLOPT_CONNECTTIMEOUT, this->m_contect_timeout);
 
        if(0 >= this->m_timeout)
        {
            this->m_timeout = DEFAULT_TIMEOUT;
        }
        curl_easy_setopt(this->m_pCurl, CURLOPT_TIMEOUT, this->m_timeout);
 
        /*
        第一:默认情况下libcurl完成一个任务以后，出于重用连接的考虑不会马上关闭,如果每次目标主机不一样，这里禁止重连接
        第二:每次执行完curl_easy_perform，libcurl会继续保持与服务器的连接。接下来的请求可以使用这个连接而不必创建新的连接,如果目标主机是同一个的话。
        这样可以减少网络开销。
        */
        curl_easy_setopt(this->m_pCurl, CURLOPT_FORBID_REUSE, 1);
 
        return 0;
 
    }
    catch(...)
    {
        printf("messagePublicMethod api exception(%d)\n", errno);
        return -1;     //发送信息公共接口异常
    }
}
 
 
/*
	* curl返回值处理
	* 参数1: curl返回码
	* 返回值int, 0表示成功, 1表示超时, 其他表示失败
*/
int CurlClient::dealResCode(const CURLcode res)
{
	//输出返回码代表的意思
	int nCode = 0;
    const char* pRes = NULL;
    pRes = curl_easy_strerror(res);
    printf("%s\n",pRes);
 
    //http返回码
    long lResCode = 0;
    curl_easy_getinfo(this->m_pCurl, CURLINFO_RESPONSE_CODE, &lResCode);
 
    if(CURLE_OK != res || 200 != lResCode)
    {
        //curl传送失败
        if(CURLE_OPERATION_TIMEOUTED == res)
        {
            nCode = 1;   //超时返回1
        }
        else
        {
            nCode = -1;    //其它错误返回-1
        }
        printf("curl send msg error: pRes=%s, lResCode=%ld \n", pRes, lResCode);
    }
	
	return nCode;
}
 
 
/*
    * 通过put的方式操作
    * 参数1：要发送的数据
    * 参数2: 数据头格式,0为默认,1为xml,2为json
    * 参数3：返回的数据
    * 返回值int, 0表示成功, 1表示超时,其他表示失败
*/
int CurlClient::putMsg(const std::string& sMsg, const int nFormat, std::string& sRec)
{
    CURLcode res = CURLE_OK;
    int nCode = -1;
 
    try
    {
	    if(NULL == this->m_pCurl)
        {
            printf("putMsg curl ptr is null\n");
            return -1;       //CURL指针为NULL
        }
		
        //发送数据,以及发送方式
        curl_easy_setopt(this->m_pCurl, CURLOPT_CUSTOMREQUEST, "PUT");
        if(sMsg.empty())
        {
            curl_easy_setopt(this->m_pCurl, CURLOPT_POSTFIELDS, "");
        }
        else
        {
            curl_easy_setopt(this->m_pCurl, CURLOPT_POSTFIELDS, (char*)sMsg.c_str());
        }
 
        //CURL公共操作方式
        nCode = messagePublicMethod(nFormat);
        if(0 != nCode)
        {
            printf("putMsg call messagePublicMethod error(%d) \n", errno);
            return -1;
        }
 
        // 设置回调函数
        curl_easy_setopt(this->m_pCurl, CURLOPT_WRITEFUNCTION, httpDataWriter);
        curl_easy_setopt(this->m_pCurl, CURLOPT_WRITEDATA, (void*)&sRec);
        res = curl_easy_perform(this->m_pCurl);
 
        //处理curl返回值
		nCode = dealResCode(res);
		if(0 > nCode)
		{
			printf("deal response code error \n");
		}
		
		return nCode;
 
    }
    catch(...)
    {
        printf("putMsg api exception(%d)\n", errno);
        return -1;         // 接口异常
    }
}
 
/*
    * 通过delete的方式操作
    * 参数1：要发送的数据,此方式可空
    * 参数2:数据头格式,0为默认,1为xml,2为json
    * 参数3：数据头格式,0表示默认,1表示xml,2为json
    * 参数4：返回的数据
    * 返回值int, 0表示成功, 1表示超时,其他表示失败
*/
int CurlClient::deleteMsg(const std::string& sMsg, const int nFormat, std::string& sRec)
{
    CURLcode res = CURLE_OK;
    int nCode = -1;
 
    try
    {
        if(NULL == this->m_pCurl)
        {
            printf("deleteMsg curl ptr is null\n");
            return -1;       //CURL指针为NULL
        }
 
        //发送数据,以及发送方式
        curl_easy_setopt(this->m_pCurl,CURLOPT_CUSTOMREQUEST,"DELETE");
 
        //CURL公共操作方式
        nCode = messagePublicMethod(nFormat);
        if(0 != nCode)
        {
            printf("deleteMsg call messagePublicMethod error(%d) \n", errno);
            return -1;
        }
 
        // 设置回调函数
        curl_easy_setopt(this->m_pCurl, CURLOPT_WRITEFUNCTION, httpDataWriter);
        curl_easy_setopt(this->m_pCurl, CURLOPT_WRITEDATA, (void*)&sRec);
        res = curl_easy_perform(this->m_pCurl);
 
        //处理curl返回值
		nCode = dealResCode(res);
		if(0 > nCode)
		{
			printf("deal response code error \n");
		}
		
		return nCode;
    }
    catch(...)
    {
        printf("deleteMsg api exception(%d)\n", errno);
        return -1;         //接口异常
    }
 
}
 
/*
    * 通过post的方式操作
    * 参数1：要发送的数据
    * 参数2:数据头格式,0为默认,1为xml,2为json
    * 参数3：数据头格式,0表示默认,1表示xml,2为json
    * 参数4：返回的数据
    * 返回值int, 0表示成功, 1表示超时,其他表示失败
*/
int CurlClient::postMsg(const std::string& sMsg, const int nFormat, std::string& sRec)
{
    CURLcode res = CURLE_OK;
    int nCode = -1;
 
    try
    {
        if(NULL == this->m_pCurl)
        {
            printf("postMsg curl ptr is null\n");
            return -1;       //CURL指针为NULL
        }
 
        //发送数据,以及发送方式
        curl_easy_setopt(this->m_pCurl, CURLOPT_POST, 1);
        if(sMsg.empty())
        {
            curl_easy_setopt(this->m_pCurl, CURLOPT_POSTFIELDS, "");
        }
        else
        {
            curl_easy_setopt(this->m_pCurl, CURLOPT_POSTFIELDS, (char*)sMsg.c_str());
        }
 
        //CURL公共操作方式
        nCode = messagePublicMethod(nFormat);
        if(0 != nCode)
        {
            printf("postMsg call messagePublicMethod error(%d) \n", errno);
            return -1;
        }
 
        // 设置回调函数
        curl_easy_setopt(this->m_pCurl, CURLOPT_WRITEFUNCTION, httpDataWriter);
        curl_easy_setopt(this->m_pCurl, CURLOPT_WRITEDATA, (void*)&sRec);
        res = curl_easy_perform(this->m_pCurl);
 
	    //处理curl返回值
		nCode = dealResCode(res);
		if(0 > nCode)
		{
			printf("deal response code error \n");
		}
		
		return nCode;
 
    }
    catch(...)
    {
        printf("postMsg api exception(%d)\n", errno);
        return -1;         // 接口异常
    }
 
}
 
/*
    * 通过get的方式操作
    * 参数1：要发送的数据, 此方式可为空
    * 参数2：数据头格式,0表示默认,1表示xml,2为json
    * 参数3：返回的数据
    * 返回值int, 0表示成功, 1表示超时,其他表示失败
*/
int CurlClient::getMsg(const std::string& sMsg, const int nFormat, std::string& sRec)
{
    CURLcode res = CURLE_OK;
    int nCode = -1;
 
    try
    {
        if(NULL == this->m_pCurl)
        {
            printf("getMsg curl ptr is null\n");
            return -1;       //CURL指针为NULL
        }
 
        //设定传输方式
        curl_easy_setopt(this->m_pCurl, CURLOPT_HTTPGET, 1);
 
        //CURL公共操作方式
        nCode = messagePublicMethod(nFormat);
        if(0 != nCode)
        {
            printf("getMsg call messagePublicMethod error(%d) \n", errno);
            return -1;
        }
 
        // 设置回调函数
        curl_easy_setopt(this->m_pCurl, CURLOPT_WRITEFUNCTION, httpDataWriter);
        curl_easy_setopt(this->m_pCurl, CURLOPT_WRITEDATA, (void*)&sRec);
        res = curl_easy_perform(this->m_pCurl);
 
        //处理curl返回值
		nCode = dealResCode(res);
		if(0 > nCode)
		{
			printf("deal response code error \n");
		}
		
		return nCode;
 
    }
    catch(...)
    {
        printf("getMsg api exception(%d)\n", errno);
        return -1;                            //接口异常
    }
 
}

/*
    * 下载文件
    * 参数1：文件url请求名
    * 参数2：本地文件存放位置
    * 参数3：数据头格式,0表示默认,1表示xml,2为json
    * 返回值int, 0表示成功, 1表示超时,其他表示失败
*/
int CurlClient::downloadFile(const std::string& sFileName, const int nFormat)
{
    CURLcode res = CURLE_OK;
    FILE* pFile = NULL;
    int nCode = -1;
 
    try
    {
        if(sFileName.empty())
        {
            printf("downloadFile filename is empty\n");
            return -1;    //文件名为空
        }
        pFile = fopen((char*)sFileName.c_str(), "w");         //打开文件,返回结果用文件存储
        if (NULL == pFile)
        {
            printf("downloadFile open file error(%d), %s\n", errno, (char*)sFileName.c_str());
            return -1;      //打开文件失败
        }
 
        //设定传输方式
        curl_easy_setopt(this->m_pCurl, CURLOPT_HTTPGET, 1);
 
        //CURL公共操作方式
        nCode = messagePublicMethod(nFormat);
        if(0 != nCode)
        {
            if(NULL != pFile)
            {
                fclose(pFile);
				pFile = NULL;
            }
 
            printf("downloadFile call messagePublicMethod error(%d) \n", errno);
            return -1;
        }
 
        // 设置回调函数
        curl_easy_setopt(this->m_pCurl, CURLOPT_WRITEFUNCTION, httpWriteFile);
        curl_easy_setopt(this->m_pCurl, CURLOPT_WRITEDATA, pFile);
        res = curl_easy_perform(this->m_pCurl);
 
        //处理curl返回值
		nCode = dealResCode(res);
		if(0 > nCode)
		{
			printf("deal response code error \n");
		}
		
		//关闭文件
		fclose(pFile);
		pFile = NULL;
		
		return nCode;
    }
    catch(...)
    {
        if(NULL != pFile)
        {
            fclose(pFile);
			pFile = NULL;
        }
        printf("downloadFile api exception(%d)\n", errno);
        return -1;         //接口异常
    }
 
}

/*
    * 下载文件
    * 参数1：文件url请求名
    * 参数2：本地文件存放位置
    * 参数3：数据头格式,0表示默认,1表示xml,2为json
    * 返回值int, 0表示成功, 1表示超时,其他表示失败
*/
int CurlClient::downloadFile(const std::string& sFileName, const std::string& sLocalFile, const int nFormat)
{
    CURLcode res = CURLE_OK;
    FILE* pFile = NULL;
    int nCode = -1;
 
    try
    {
        if(sFileName.empty())
        {
            printf("downloadFile filename is empty\n");
            return -1;    //文件名为空
        }
        pFile = fopen((char*)sLocalFile.c_str(), "w");         //打开文件,返回结果用文件存储
        if (NULL == pFile)
        {
            printf("downloadFile open file error(%d), %s\n", errno, (char*)sLocalFile.c_str());
            return -1;      //打开文件失败
        }
 
        //设定传输方式
        curl_easy_setopt(this->m_pCurl, CURLOPT_HTTPGET, 1);
 
        //CURL公共操作方式
        nCode = messagePublicMethod(nFormat);
        if(0 != nCode)
        {
            if(NULL != pFile)
            {
                fclose(pFile);
				pFile = NULL;
            }
 
            printf("downloadFile call messagePublicMethod error(%d) \n", errno);
            return -1;
        }
 
        // 设置回调函数
        curl_easy_setopt(this->m_pCurl, CURLOPT_WRITEFUNCTION, httpWriteFile);
        curl_easy_setopt(this->m_pCurl, CURLOPT_WRITEDATA, pFile);
        res = curl_easy_perform(this->m_pCurl);
 
        //处理curl返回值
		nCode = dealResCode(res);
		if(0 > nCode)
		{
			printf("deal response code error \n");
		}
		
		//关闭文件
		fclose(pFile);
		pFile = NULL;
		
		return nCode;
    }
    catch(...)
    {
        if(NULL != pFile)
        {
            fclose(pFile);
			pFile = NULL;
        }
        printf("downloadFile api exception(%d)\n", errno);
        return -1;         //接口异常
    }
 
}
 
/*
    * 从文件中读取内容post出去
    * 参数1：文件名
    * 参数2：返回数据
    * 返回值int, 0表示成功, 1表示超时,其他表示失败
*/
int CurlClient::uploadFileContent(const std::string& sFileName, const int nFormat, std::string& sRec)
{
    CURLcode res = CURLE_OK;
    FILE* pFile = NULL;
    struct stat file_info;
    curl_off_t fsize;
    int nCode = 0;
 
    try
    {
        if(sFileName.empty())
        {
            printf("uploadFileContent filename is empty\n");
            return -1;    //文件名为空
        }
 
        if(stat((char*)sFileName.c_str(), &file_info))    //文件大小
        {
            printf("uploadFileContent get file info error(%d), %s\n", errno, (char*)sFileName.c_str());
            return -1;
        }
        fsize = (curl_off_t)file_info.st_size;
 
        pFile = fopen((char*)sFileName.c_str(), "rb");         //打开文件,返回结果用文件存储
        if (NULL == pFile)
        {
            printf("uploadFileContent open file error(%d), %s\n", errno, (char*)sFileName.c_str());
            return -1;      //打开文件失败
        }
 
        //设定传输方式
        curl_easy_setopt(this->m_pCurl, CURLOPT_POST, 1);
 
        //CURL公共操作方式
        nCode = messagePublicMethod(nFormat);
        if(0 != nCode)
        {
            if(NULL != pFile)
            {
                fclose(pFile);
				pFile = NULL;
            }
 
            printf("uploadFileContent call messagePublicMethod error(%d) \n", errno);
            return -1;
        }
 
        // 设置回调函数
        curl_easy_setopt(this->m_pCurl, CURLOPT_READFUNCTION, httpReadFile);
        curl_easy_setopt(this->m_pCurl, CURLOPT_READDATA, pFile);
        curl_easy_setopt(this->m_pCurl, CURLOPT_POSTFIELDSIZE, (curl_off_t)fsize);
 
        sRec = "";  //清空数据
        curl_easy_setopt(this->m_pCurl, CURLOPT_WRITEFUNCTION, httpDataWriter);
        curl_easy_setopt(this->m_pCurl, CURLOPT_WRITEDATA, (void*)&sRec);
        res = curl_easy_perform(this->m_pCurl);
        
		//处理curl返回值
		nCode = dealResCode(res);
		if(0 > nCode)
		{
			printf("deal response code error \n");
		}
		
		//关闭文件
		fclose(pFile);
		pFile = NULL;
		
		return nCode;
 
    }
    catch(...)
    {
        if(NULL != pFile)
        {
            fclose(pFile);
			pFile = NULL;
        }
        printf("uploadFileContent api exception(%d)\n", errno);
        return -1;         //接口异常
    }
 
}
 
 
 
/*
    * 宏定义上传文件相关信息
*/
#define FILE_NAME_MAX_SIZE      (128)
 
 
/*
    * 从文件全路径中获取文件名指针
    * 返回文件名地址
*/
const char* CurlClient::getFileName(const char *path)
{
    if(!path)
    {
        return NULL;
    }
 
    //找最后一个斜杠/
    const char *pname = strrchr(path, '/');
    if(!pname)
    {
        //没找到斜杠,则认为path就是文件名
        return path;
    }
 
    //找到最后一个斜杠, 反回指针加1
    return (char*)(pname + 1);
}
 
 
 
/*
    * 上传文件
    * 参数1：文件名
    * 参数2：返回数据
    * 返回值int, 0表示成功, 1表示超时,其他表示失败
*/
int CurlClient::uploadFile(const std::string& sFileFullname, std::string& sRec)
{
    CURLcode res = CURLE_OK;
    int nCode = 0;
    struct curl_httppost *formpost = NULL;
    struct curl_httppost *lastptr = NULL;
    struct curl_slist *headerlist = NULL;
    static const char buf[] = "Expect:";
 
    try
    {
        if(sFileFullname.empty())
        {
            printf("uploadFile sFileFullname is empty\n");
            return -1;    //文件名为空
        }
 
        //获取文件名
        const char* pFileName = getFileName(sFileFullname.c_str());
        if(NULL == pFileName || '\0' == pFileName[0])
        {
            printf("uploadFileContent call getFileName failure, sFileFullname=%s \n", sFileFullname.c_str());
            return -1;
        }
        printf("uploadFile pFileName=%s \n", pFileName);
 
        //CURL公共操作方式
        nCode = messagePublicMethod(FORMAT_DEFAULT);
        if(0 != nCode)
        {
            printf("uploadFile call messagePublicMethod error(%d) \n", errno);
            return -1;
        }
 
        /*
        Fill in the file upload field. This makes libcurl load data from
         the given file name when curl_easy_perform() is called.
        */
        curl_formadd(&formpost,
                 &lastptr,
                 CURLFORM_COPYNAME, "sendfile",
                 CURLFORM_FILE, sFileFullname.c_str(),
                 CURLFORM_END);
 
        /* Fill in the filename field */
        curl_formadd(&formpost,
                   &lastptr,
                   CURLFORM_COPYNAME, "filename",
                   CURLFORM_COPYCONTENTS, pFileName,
                   CURLFORM_END);
 
        /* Fill in the submit field too, even if this is rarely needed */
        curl_formadd(&formpost,
                  &lastptr,
                  CURLFORM_COPYNAME, "submit",
                  CURLFORM_COPYCONTENTS, "send",
                  CURLFORM_END);
 
        headerlist = curl_slist_append(headerlist, buf);
 
        /* only disable 100-continue header if explicitly requested */
        curl_easy_setopt(this->m_pCurl, CURLOPT_HTTPHEADER, headerlist);
        curl_easy_setopt(this->m_pCurl, CURLOPT_HTTPPOST, formpost);
 
        //返回值
        sRec = "";  //清空数据
        curl_easy_setopt(this->m_pCurl, CURLOPT_WRITEFUNCTION, httpDataWriter);
        curl_easy_setopt(this->m_pCurl, CURLOPT_WRITEDATA, (void*)&sRec);
 
        /* Perform the request, res will get the return code */
        res = curl_easy_perform(this->m_pCurl);
 
        /* then cleanup the formpost chain */
        if(NULL != formpost)
        {
            curl_formfree(formpost);
            formpost = NULL;
        }
 
        /* free slist */
        if( NULL != headerlist)
        {
            curl_slist_free_all (headerlist);
            headerlist = NULL;
        }
		
		//处理curl返回值
		nCode = dealResCode(res);
		if(0 > nCode)
		{
			printf("deal response code error \n");
		}
		
        return nCode;
 
    }
    catch(...)
    {
        if(NULL != formpost)
        {
            curl_formfree(formpost);
            formpost = NULL;
        }
 
        if( NULL != headerlist)
        {
            curl_slist_free_all (headerlist);
            headerlist = NULL;
        }
 
        printf("uploadFile api exception(%d)\n", errno);
        return -1;         //接口异常
    }
 
}