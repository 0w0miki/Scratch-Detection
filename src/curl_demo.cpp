#include <curl/curl.h>
#include <stdio.h>
#include <iostream> 
#include <string>
#include <sstream>
#include <json/json.h>

CURL *curl;
CURLcode res;

using namespace std;

//回调函数
size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream) 
{
    string data((const char*) ptr, (size_t) size * nmemb);

    *((stringstream*) stream) << data << endl;

    return size * nmemb;
}

//POST json
int main()
{
    CURL *curl;
    CURLcode res;
    char tmp_str[256] = { 0 };
    std::stringstream out;

    //HTTP报文头  
    struct curl_slist* headers = NULL;

    char *url = "http://127.0.0.1:7999/api/fun1";

    curl = curl_easy_init();

    if(curl)
    {
        //构建json
        Json::Value item;
        item["n1"]=Json::Value(1);
        item["n2"]=Json::Value(2);
        // item["type"]=Json::Value("Libcurl HTTP POST Json串");
        // item["authList"]=Json::Value("weidong0925@126.com");
        std::string jsonout = item.toStyledString();

        // jsonout = AsciiToUtf8(jsonout);

        //设置url
        curl_easy_setopt(curl, CURLOPT_URL, url);

        //设置http发送的内容类型为JSON
        //构建HTTP报文头  
        // sprintf_s(tmp_str, "Content-Length: %s", jsonout.c_str());
        snprintf(tmp_str,sizeof(tmp_str),"Content-Length: %s", jsonout.c_str());
        headers=curl_slist_append(headers, "Content-Type:application/json;charset=UTF-8");
        //headers=curl_slist_append(headers, tmp_str);//在请求头中设置长度,请求会失败,还没找到原因

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        //curl_easy_setopt(curl,  CURLOPT_CUSTOMREQUEST, "POST");//自定义请求方式
        curl_easy_setopt(curl, CURLOPT_POST, 1);//设置为非0表示本次操作为POST

        // 设置要POST的JSON数据
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonout.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, jsonout.size());//设置上传json串长度,这个设置可以忽略

        // 设置接收数据的处理函数和存放变量
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);//设置回调函数
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &out);//设置写数据
        res = curl_easy_perform(curl);//执行

        curl_slist_free_all(headers); /* free the list again */

        string str_json = out.str();//返回请求值 
        printf("%s",str_json.c_str()); 

        /* always cleanup */ 
        curl_easy_cleanup(curl);
    } 

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