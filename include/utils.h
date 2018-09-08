#if !defined(CLIENT_H)
#define CLIENT_H

#include <curl/curl.h>
#include <stdio.h>
#include <iostream> 
#include <string>
#include <fstream>
#include <sstream>
#include <json/json.h>

using namespace std;

struct thread_param
{
    Json::Value* root;
    pthread_mutex_t* mutex;
};


size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream);
int http_post_file(const char *url, const char *filename);
int check_file(char *filename);
int http_post_json(const char *url, const Json::Value json_value);
int http_get_json(const char *url);
// int readBatchFile(std::string filename, std::deque<std::deque<ROI>> &batch_ROI_list, std::deque<std::string> &batch_origin_list, std::deque<int64_t> &batch_count_list, std::deque<cv::Point2i> &desired_size_list);
int readBatchFile(std::string filename, std::deque<std::string> &work_name_list, std::deque<int64_t> &work_count_list, std::deque<std::string> &batch_origin_list, std::deque<int64_t> &batch_count_list);
void SplitString(const std::string& s, std::vector<std::string>& v, const std::string& c);

#endif // CLIENT_H
