#if !defined(UTILS_H_)
#define UTILS_H_

#include <curl/curl.h>
#include <stdio.h>
#include <iostream>
#include <sys/types.h>
#include <dirent.h>
#include <string>
#include <fstream>
#include <sstream>
#include <json/json.h>

using namespace std;

struct thread_param
{
    Json::Value* root;
    pthread_mutex_t* mutex;
    std::string host;
    std::string url;
    unsigned int port;
};

int check_file(char *filename);
// int readBatchFile(std::string filename, std::deque<std::deque<ROI>> &batch_ROI_list, std::deque<std::string> &batch_origin_list, std::deque<int64_t> &batch_count_list, std::deque<cv::Point2i> &desired_size_list);
int readBatchFile(std::string filename, std::deque<std::string> &work_name_list, std::deque<int> &work_count_list, std::deque<std::string> &batch_origin_list, std::deque<int> &batch_count_list);
// 分割字符串
void SplitString(const std::string& s, std::vector<std::string>& v, const std::string& c);
// 检测路径是否存在
int isDirExist(const std::string dir_path);
// 等待ms
void wait(int time_ns);

#endif // UTILS_H_
