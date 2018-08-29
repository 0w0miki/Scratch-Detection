#include "Camera.h"
#include "Detector.h"

int readBatchFile(std::string filename, std::vector<std::vector<ROI>> &batch_ROI_list, std::vector<std::string> &batch_origin_list, std::vector<int64_t> &batch_count_list, std::vector<cv::Point2i> &desired_size_list){
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

int readBatchFile(std::string filename, std::vector<std::string> &work_id_list, std::vector<int64_t> &work_count_list, std::vector<std::string> &batch_origin_list, std::vector<int64_t> &batch_count_list){
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
            int work_size = root["work"].size();
            if(work_size > 0){
                for(int i = 0; i < work_size; i++){
                    std::string work_id = root["work"][i]["id"].asString();
                    int64_t work_count = root["work"][i]["total"].asInt64();
                    work_id_list.push_back(work_id);
                    work_count_list.push_back(work_count);
                    int batch_size = root["work"][i]["batch"].size();
                    printf("batch size %d\n", batch_size);
                    if(batch_size > 0){
                        Json::Value batch = root["work"][i]["batch"];
                        for(int j = 0; j < batch_size; j++){
                            std::string img_url = batch[j]["img url"].asString();
                            batch_origin_list.push_back(img_url);
                            std::cout << "set origin image url " << img_url << std::endl;
                            int64_t batch_count = batch[j]["total"].asInt64();
                            batch_count_list.push_back(batch_count);
                            std::cout << "set detection batch "<< batch_count << std::endl;
                        }
                    }else{
                        printf("[ERROR] no batch array");
                        batch_file.close();
                        return -3;
                    }
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

int main(int argc, char const *argv[])
{
    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, NULL);

    std::queue<string> unsolved_list;
    // 批次信息
    std::vector<std::string> work_id_list;
    std::vector<int64_t> work_count_list;
    std::vector<string> batch_origin_list;
    std::vector<int64_t> batch_count_list;
    // std::vector<cv::Point2i> desired_size_list;

    // 读取批次信息
    int ret = readBatchFile("batch_info.json", work_id_list, work_count_list, batch_origin_list, batch_count_list);
    if(ret == -1){
        ;
    }
    Camera camera(&mutex, &unsolved_list, &work_id_list, &work_count_list);
    Detector detector(&mutex, &unsolved_list, &work_id_list, &work_count_list, &batch_origin_list, &batch_count_list);

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
        int64_t detect_count = detector.getCount();
        int64_t catch_count = camera.getCount();
        
        int c = getchar();
        switch(c)
        {
            //退出程序
            case 'X':
            case 'x':
                run = false;
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