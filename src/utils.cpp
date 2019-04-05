#include "utils.h"

//回调函数
size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream){
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

int check_file(char *filename){
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

int http_get_json(const char *url){
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
    curl_easy_cleanup(curl);
    //curl_global_cleanup();
    return 0;
}



int readBatchFile(std::string filename, 
    std::deque<std::string> &work_name_list, 
    std::deque<int64_t> &work_count_list, 
    std::deque<std::string> &batch_origin_list, 
    std::deque<int64_t> &batch_count_list)
{
    std::ifstream batch_file;
    std::string file_dir = "../";
    std::string file = file_dir + filename;
    batch_file.open(file , std::ios::in);

    if(!batch_file){
        printf("[ERROR] failed to open batch file\n");
        return -1;
    }else{
        Json::CharReaderBuilder builder;
	    Json::Value root;
        builder["collectComments"] = false;
        JSONCPP_STRING errs;
        if (parseFromStream(builder, batch_file, &root, &errs)){
            int work_size = root["printwork"].size();
            if(work_size > 0){
                for(int i = 0; i < work_size; i++){
                    int work_id = root["printwork"][i]["printJobID"].asInt();
                    int64_t work_count = root["printwork"][i]["printQuantity"].asInt64();
                    if(root["printwork"][i]["pictureLink"].isString()){
                        // 图像是string所有都是这个图
                        batch_count_list.push_back(work_count);
                        batch_origin_list.push_back(root["printwork"][i]["pictureLink"].asString());
                        std::string work_name = "print_";
                        work_name.append(to_string(work_id));
                        work_name_list.push_back(work_name);
                        work_count_list.push_back(work_count);
                    }else{
                        // 列表
                        int link_size = root["printwork"][i]["pictureLink"].size();
                        if(root["printwork"][i]["printRedoFlag"].asBool() == false){
                            // 不是重打 只有原图需要改变
                            std::string work_name = "print_";
                            work_name.append(to_string(work_id));
                            work_name_list.push_back(work_name);
                            work_count_list.push_back(work_count);
                        }
                        for(int j = 0; j < link_size; j++)
                        {
                            batch_count_list.push_back(1);
                            batch_origin_list.push_back(root["printwork"][i]["pictureLink"][j].asString());
                            if(root["printwork"][i]["printRedoFlag"].asBool() == true){
                                // 重打 文件名需要改变 work_count是1
                                int reprint_id = root["printwork"][i]["printWorkNumber"][j].asInt();
                                std::string work_name = "reprint_";
                                work_name.append(to_string(work_id));
                                work_name.append("_");
                                work_name.append(to_string(reprint_id));
                                work_name_list.push_back(work_name);
                                work_count_list.push_back(1);
                            }
                        }
                    }
                    // printf("batch size %d\n", batch_size);
                }
            }else{
                printf("[ERROR] no work array");
                batch_file.close();
                return -2;
            }
        }else{
            cout << "[ERROR] Jsoncpp error: " << errs << endl;
        }
        batch_file.close();
    }
    return 0;
}

/** 
 * @brief           分词
 * @param           string s  字符串
 * @param           vector<>v 结果
 * @param           string c  分割字符
 */
void SplitString(const std::string& s, std::vector<std::string>& v, const std::string& c)
{
    std::string::size_type pos1, pos2;
    pos2 = s.find(c);
    pos1 = 0;
    while(std::string::npos != pos2)
    {
        v.push_back(s.substr(pos1, pos2-pos1));

        pos1 = pos2 + c.size();
        pos2 = s.find(c, pos1);
    }
    if(pos1 != s.length())
        v.push_back(s.substr(pos1));
}

/**
 * @brief           检验文件夹是否存在
 * 
 * @param dir_path  文件夹路径
 * @return int      1-存在, -1不存在
 */
int isDirExist(const std::string dir_path){
    if(dir_path.empty())
        return -1;
    if(NULL == opendir(dir_path.c_str()))
        return -1;
    return 1;
}

/**
 * @brief           等待一段时间
 * 
 * @param           时长单位ns 
 */
void wait(int time_ns){
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = time_ns * 1000;
    select(0,NULL,NULL,NULL,&tv);
}