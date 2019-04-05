#include "Camera.h"
#include "Detector.h"
#include "http_client.h"

int readBatchFile(std::string filename, std::vector<std::vector<ROI>> &batch_ROI_list, std::deque<std::string> &batch_origin_list, std::deque<int64_t> &batch_count_list, std::vector<cv::Point2i> &desired_size_list){
    std::ifstream batch_file;
    std::string file_dir = "../";
    std::string file = file_dir + filename;
    batch_file.open(file , std::ios::in);

    if(!batch_file){
        printf("[ERROR] failed to open batch file\n");
        return -1;
    }else{
        std::string lineStr;
        while(std::getline(batch_file, lineStr)){
            std::stringstream ss(lineStr);
            int index = 0;
            std::string str;
            std::vector<ROI> rois;
            struct ROI a;
            cv::Point2i desired_size;
            while (getline(ss, str, ',')){
                if(index == 0)
                {
                    // 设置批次数量列表
                    int64_t batch_num;
                    std::istringstream is(str);
                    is >> batch_num;
                    batch_count_list.push_back(batch_num);
                    printf("set batch number: %ld\n",batch_num);
                }
                if(index == 1){
                    // 设置原图列表
                    batch_origin_list.push_back(str);
                    printf("set origin image: %s\n",str.c_str());
                }
                if(index == 2){
                    int desired_width;
                    std::istringstream is(str);
                    is >> desired_width;
                    desired_size.x = desired_width;
                    printf("set desired width: %d\n",desired_width);
                }
                if(index == 3){
                    int desired_height;
                    std::istringstream is(str);
                    is >> desired_height;
                    desired_size.y = desired_height;
                    printf("set desired height: %d\n",desired_height);
                    cout<<desired_size<<endl;
                    desired_size_list.push_back(desired_size);
                }
                // if(index > 3){
                //     if((index - 4) % 4 == 0){
                //         int x;
                //         std::istringstream is(str);
                //         is >> x;
                //         a.x = x;
                //         printf("set roi x: %d\n",x);
                //     }else if((index - 4) % 4 == 1){
                //         int y;
                //         std::istringstream is(str);
                //         is >> y;
                //         a.y = y;
                //         printf("set roi y: %d\n",y);
                //     }else if((index - 4) % 4 == 2){
                //         int w;
                //         std::istringstream is(str);
                //         is >> w;
                //         a.w = w;
                //         printf("set roi w: %d\n",w);
                //     }else if((index - 4) % 4 == 3){
                //         int h;
                //         std::istringstream is(str);
                //         is >> h;
                //         a.h = h;
                //         rois.push_back(a);
                //         printf("set roi h: %d\n",h);
                //     }
                // }
                index++;
            }
            batch_ROI_list.push_back(rois);
        }
    }
    return 0;
}

void* post_result(void *arg){
    
    thread_param* param_ptr;
    param_ptr = (struct thread_param *) arg;
    Json::Value* result_root = param_ptr->root;
    pthread_mutex_t* result_mutex = param_ptr->mutex;
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

int main(int argc, char const *argv[])
{
    pthread_t post_thread_id = 3;
    pthread_mutex_t slv_list_mutex;
    pthread_mutex_init(&slv_list_mutex, NULL);
    pthread_mutex_t result_mutex;
    pthread_mutex_init(&result_mutex, NULL);

    std::queue<string> unsolved_list;
    // 批次信息
    std::deque<std::string> work_name_list;
    std::deque<int64_t> work_count_list;
    std::deque<string> batch_origin_list;
    std::deque<int64_t> batch_count_list;
    // std::vector<cv::Point2i> desired_size_list;

    Json::Value* result_root = new Json::Value();
    CurlClient::globalInit();

    thread_param post_thread_param;
    post_thread_param.root = result_root;
    post_thread_param.mutex = &result_mutex;
    // 错误回报线程
    pthread_create(&post_thread_id, NULL, post_result, (void*)&post_thread_param);

    // 读取批次信息
    int ret = readBatchFile("batch_info.json", work_name_list, work_count_list, batch_origin_list, batch_count_list);
    if(ret != 0){
        ;
    }
    Camera camera(&slv_list_mutex, &unsolved_list, &work_name_list, &work_count_list);
    Detector detector(&slv_list_mutex, &result_mutex, result_root, &unsolved_list, &work_name_list, &work_count_list, &batch_origin_list, &batch_count_list);

    camera.init();
    camera.setTrigger();
    camera.applyParam();

    detector.setParam();

    // 收到开车指令


    // 设置第一个批次的原图和ROI
    detector.setOriginImg(batch_origin_list[0]);
    // camera.setROI(batch_ROI_list[0]);

    //发送开采命令
    ret = camera.start();
    if(ret != 0) printf("[WARN] failed to start camera");

    ret = detector.launchThread();
    if(ret != 0) printf("[WARN] failed to launch detection");

    printf("====================Menu================\n");
    printf("[X or x]:Exit\n");
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
                break;
            case 'c':
            case 'C':
                unsolved_list.push("print_1_0.ppm");
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

    // end process
    // ret = camera.stop();
    // if(ret != 0) printf("[WARN] failed to stop camera");

    cv::destroyAllWindows();
    return 0;
}