#include <curl/curl.h>
#include <stdio.h>
#include <iostream> 
#include <string>
#include <fstream>
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

int http_post_json(const char *url, const Json::Value json_value){
    CURL *curl = NULL;
    CURLcode res;

    Json::StreamWriterBuilder wbuilder;
    wbuilder["indentation"] = "";
    std::string post_str = Json::writeString(wbuilder, json_value);
    // std::string post_str = json_value.toStyledString();

    struct curl_httppost *post=NULL;
    struct curl_httppost *last=NULL;
    struct curl_slist *headers=NULL;
    headers = curl_slist_append(headers, "Content-Type:application/json;charset=UTF-8");

    if(json_value == NULL || url == NULL){
        printf("<-----------no url or filename----------->\n");
        return -1;
    }

    printf("URL: %s\n", url);
    printf("post_str: %s\n", post_str.data());
    
    /* Add simple file section */
    curl = curl_easy_init();
    if(curl == NULL)
    {
        fprintf(stderr, "curl_easy_init() error.\n");
    }

    curl_easy_setopt(curl, CURLOPT_POST, 1);//设置为非0表示本次操作为POST
    curl_easy_setopt(curl, CURLOPT_HEADER, headers);
    curl_easy_setopt(curl, CURLOPT_URL, url); /*Set URL*/
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_str.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, post_str.size());
    int timeout = 5;
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
    // curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    // curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 1);

    res = curl_easy_perform(curl);
    if(res != CURLE_OK)
    {
        fprintf(stderr, "curl_easy_perform[%d] error.\n", res);
    }
    curl_easy_cleanup(curl);
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

//POST json
int main()
{

    char *url = "http://127.0.0.1:7999/api/result";
    char *filename = "/home/miki0w0/gree/results.json";
    
    //构建json
    Json::Value root;
    root["n1"]=Json::Value(1);
    root["n2"]=Json::Value(2);
    http_post_json(url, root);

    
    // if(check_file(filename) == -1){
    //     printf("file cannot open");
    //     return 1;
    // }
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