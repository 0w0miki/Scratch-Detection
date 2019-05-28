#if !defined(DETECTOR_H)
#define DETECTOR_H

#include "DetectUtils.h"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <queue>
#include "utils.h"
#include "Log.hpp"
#include "Serial.h"
#include "Camera.h"
#include "json/json.h"

// enum detectType{
//     DETECTA4 = 1,
//     DETECTEACH = 0xfffe,
//     SECDETECT = 2,
//     FIRSTDETECT = 0xfffd
// };

// #define DETECTEACH 0
#define DETECTA4 1
#define SECDETECT 2

#define STATE_OK 0
#define FINDLINEERR -1

class Detector
{
enum DETECTOR_STATE{
    DETECTOR_STOP,
    DETECTOR_RUN,
    DETECTOR_READY_TO_PAUSE,
    DETECTOR_PAUSE
};

private:
    // 图
    cv::Mat template_img_;                  // 模板图像
    cv::Mat template_label_;                // 模板图像中的标签
    cv::Mat template_img_resize_;           // 【废弃】更改尺寸的模板图像
    cv::Mat img_gray_;                      // 拍摄图像
    cv::Mat img_resize_;                    // 【废弃】更改尺寸的拍摄图像
    cv::Mat label_;                         // 图像中的标签
    std::vector<Point2f> img_points_;       // 【废弃】图像边角点
    
    // 线程
    pthread_mutex_t* mutex_;                // 待处理队列 锁
    pthread_mutex_t* result_mutex_;         // 结果的json 锁
    pthread_t detection_thread_;            // pthread线程id
    std::queue<string>* unsolved_list_;     // 待处理队列

    // 参数
    float k_pos_;                           // 计算位置问题阈值的比例因子
    float k_scratch_;                       // 计算瑕疵问题阈值的比例因子
    float k_bigpro_;                        // 计算划痕问题阈值的比例因子
    string template_dir_;                   // 模板图片路径
    string img_dir_;                        // 拍摄图片路径
    int ROI_y_;                             // A4纸图像y轴上边沿坐标
    int ROI_height_;                        // A4纸区域图像高
    int left_cut_px_;                       // 左侧切除像素
    int right_cut_px_;                      // 右侧切除像素
    int scratch_pixel_num_;                 // 移动的像素范围

    // 计数器
    int count_;                             // 原图
    int64_t id_count_;                      // work id

    // 类型 0-检测A4 1-检测单张
    int8_t input_type_;

    // 期望尺寸
    float d_width_;
    float d_height_;

    // log
    ofstream result_log_;

    // 阈值
    uint8_t bin_thresh_;
    int bigpro_thresh_;
    int scratch_thresh_;
    int pos_thresh_;
    float size_thresh_;

    // 开关
    bool save_img_switch_;
    bool save_result_switch_;
    bool show_time_switch_;
    bool log_switch_;
    bool start_detect_;

    // 原图是否已设置的标志位
    bool origin_flag;

    // 批次信息
    std::deque<std::string>* work_name_list_;               // 作业的文件名前缀
    std::deque<int>* work_count_list_;                      // 作业的数量
    std::deque<string>* batch_origin_list_;                 // 原图文件名
    std::deque<int>* batch_count_list_;                     // 一张原图对应的照片数量
    std::vector<cv::Point2i>* desired_size_list_;           // 【废弃】
    std::vector<cv::Point2i>::iterator desired_size_iter_;  // 【废弃】这里不能用vector迭代器 会有失效的问题

    // 返回信息
    Json::Value* result_root_;

    // 相机指针
    Camera* camera_;

    // 串口指针
    Serial* serial_;

    // remap消畸变
    cv::Mat remap_x_, remap_y_;

    // 
    DETECTOR_STATE state_ = DETECTOR_STOP;

protected:
    int findLabel(cv::Mat image_gray, cv::Mat &match_templ, std::vector<cv::Point2f> & points);
    int findPaper(cv::Mat image_gray, cv::Mat &det_img, std::vector<cv::Point2f> & points);
    cv::Mat getLabelImg(Mat img);
    void getROI(cv::Mat image, cv::Mat &dst);
    cv::Mat LOG(Mat img);
    void highConstract(Mat img, Mat & dst, int r, int type);
    cv::Mat search(Mat img, Mat templ);
    void saveImg(string pre, Mat img);
    int checkSize();
    int checkPos();
    int checkScratch();
    int checkBigProblem();
    void ProcDetect();
    static void* detector_pth(void* args);
    void writeResJson(int8_t result);
    // void crossIntegral(cv::Mat M1, cv::Mat M2, cv::Mat& crosssum);
    // cv::Mat getMerge(Mat src,int m,int n,int width,int height,int style);
    // cv::Mat getSum(Mat src,int m,int n,int width,int height);
    // cv::Mat getNCC(Mat origin_img,Mat img,Mat ointegral, Mat iintegral, int m, int n,int width,int height);

public:
    Detector();
    Detector(pthread_mutex_t* mutex, std::queue<string>* list);
    Detector(pthread_mutex_t* mutex, 
            std::queue<string>* list, 
            std::deque<string>* batch_origin_list, 
            std::deque<int>* batch_count_list, 
            std::vector<cv::Point2i>* desired_size_list, 
            pthread_t threadId = 2);

    Detector(pthread_mutex_t* mutex, 
            pthread_mutex_t* result_mutex, 
            Json::Value* root, 
            std::queue<string>* list, 
            std::deque<std::string>* work_name_list_, 
            std::deque<int>* work_count_list_, 
            std::deque<string>* batch_origin_list, 
            std::deque<int>* batch_count_list, 
            int detectionId = 0, 
            pthread_t threadId = 2);
    ~Detector();
    int setParam();
    int detect();
    int launchThread();
    int setOriginImg(string filename);
    int setOriginImg(Mat img);
    void setDesiredSize(cv::Point2i desired_size);
    int setImg(string filename);
    int setImg(Mat img);
    void setCameraPtr(Camera* camera);
    void setSerialPtr(Serial* serial);
    void setThresh();
    int stopThread();
    int sendMsg();
    int getCount();
    int64_t getIdCount();
    void setCount(int num){count_ = num;}
    void setIdCount(int64_t id){id_count_ = id;}
    inline void callPause(){state_ = DETECTOR_READY_TO_PAUSE;}
    inline void restart(){state_ = DETECTOR_RUN;}
    inline bool isPause(){return DETECTOR_PAUSE == state_;}
};


#endif // DETECTOR_H
