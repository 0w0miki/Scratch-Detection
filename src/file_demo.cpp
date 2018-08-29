#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include "json/json.h"
#include "Camera.h"


int readBatchFile(std::string filename, std::vector<std::vector<ROI>> &batch_ROI_list, std::vector<std::string> &batch_origin_list, std::vector<long> &batch_count_list){
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
            while (getline(ss, str, ',')){
                struct ROI a;
                if(index == 0)
                {
                    // 设置批次数量
                    long batch_num;
                    std::istringstream is(str);
                    is >> batch_num;
                    batch_count_list.push_back(batch_num);
                    printf("set batch number: %ld\n",batch_num);
                }
                if(index == 1){
                    // 设置原图
                    batch_origin_list.push_back(str);
                    printf("set batch number: %s\n",str.c_str());
                }
                if(index > 1){
                    if((index - 2) % 4 == 0){
                        int x;
                        std::istringstream is(str);
                        is >> x;
                        a.x = x;
                        printf("set roi x: %d\n",x);
                    }else if((index - 2) % 4 == 1){
                        int y;
                        std::istringstream is(str);
                        is >> y;
                        a.y = y;
                        printf("set roi y: %d\n",y);
                    }else if((index - 2) % 4 == 2){
                        int w;
                        std::istringstream is(str);
                        is >> w;
                        a.w = w;
                        printf("set roi w: %d\n",w);
                    }else if((index - 2) % 4 == 3){
                        int h;
                        std::istringstream is(str);
                        is >> h;
                        a.h = h;
                        rois.push_back(a);
                        printf("set roi h: %d\n",h);
                    }
                }
                index++;
            }
            batch_ROI_list.push_back(rois);
        }
    }
}

void writeResJson(){
    std::fstream resFile;
	resFile.open("../results.json", std::ios::in | std::ios::out);
    if(!resFile){
        printf("[ERROR] failed to open results.json\n");
        // return -1;
    }else{
        if(resFile.get() == EOF){
            resFile.close();
            // 文件为空 写入
            std::ofstream file;
	        file.open("../results.json",std::ios::out);
            std::cout<<"[WARN]File empty"<<std::endl;
            Json::Value root;
            Json::Value batch_item;
            Json::Value Id_item;
            Json::StreamWriterBuilder wbuilder;
            wbuilder["indentation"] = "";
            std::unique_ptr<Json::StreamWriter> writer(wbuilder.newStreamWriter());

            batch_item["batch id"] = 1;
            Id_item["id"] = 1;
            batch_item["image state"].append(Id_item);
            Id_item.clear();
            Id_item["id"] = 2;
            batch_item["image state"].append(Id_item);
            root.append(batch_item);
            batch_item.clear();
            std::cout << "'" << Json::writeString(wbuilder, root) << "'" << std::endl;
            writer->write(root, &file);
            resFile.close();
            return;
        }
        // 文件非空 
        Json::CharReaderBuilder builder;
        Json::StreamWriterBuilder wbuilder;
        builder["collectComments"] = false;
        Json::Value root;
        JSONCPP_STRING errs;
        if (parseFromStream(builder, resFile, &root, &errs)){
            // root[""];
            if(root.empty()){
                Json::Value batch_item;
                Json::Value Id_item;
                
                batch_item["batch id"] = 3;
                Id_item["id"] = 1;
                batch_item["image state"].append(Id_item);
                Id_item.clear();
                Id_item["id"] = 2;
                batch_item["image state"].append(Id_item);
                root.append(batch_item);
                
                root.append(batch_item);
                batch_item.clear();
                std::cout << "'" << Json::writeString(wbuilder, root) << "'" << std::endl;
                // std::cout<<root.toStyledString()<<std::endl;
                resFile<<root.toStyledString();
                // std::string document = Json::writeString(value, root);
                std::cout<< "nothing in the json file" <<std::endl;
            }
        }else{
            std::cout << "[ERROR] Jsoncpp error: " << errs << std::endl;
        }
        resFile.close();
    }
    // return 0;
}

int main(int argc, char const *argv[])
{
    // std::vector<std::vector<ROI>> batch_ROI_list;
    // std::vector<std::string> batch_origin_list;
    // std::vector<long> batch_count_list;
    // readBatchFile("batch_info.csv", batch_ROI_list, batch_origin_list, batch_count_list);
    writeResJson();
    return 0;
}
