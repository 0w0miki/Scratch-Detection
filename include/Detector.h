#if !defined(DETECTOR_H)
#define DETECTOR_H

#include "DetectUtils.h"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <queue>
#include "json/json.h"

#define DETECTA4 0
#define DETECTEACH 1

class Detector
{
private:
    cv::Mat template_img_;
    cv::Mat template_label_;
    cv::Mat template_img_resize_;
    cv::Mat img_gray_;
    cv::Mat img_resize_;
    cv::Mat label_;
    std::vector<Point2f> img_points_;
    
    // 线程
    pthread_mutex_t* mutex_;
    pthread_t detection_thread_; 
    std::queue<string>* unsolved_list_;

    // 参数
    float k_pos_;
    float k_scratch_;
    float k_bigpro_;
    string template_dir_;
    string img_dir_;

    // id
    int64_t count_;
    int64_t id_;

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

    // 批次信息
    std::vector<std::string>* work_id_list_;
    std::vector<std::string>::iterator work_id_iter_;
    std::vector<int64_t>* work_count_list_;
    std::vector<int64_t>::iterator work_count_iter_;
    std::vector<string>* batch_origin_list_;
    std::vector<string>::iterator batch_origin_iter_;
    std::vector<int64_t>* batch_count_list_;
    std::vector<int64_t>::iterator batch_count_iter_;
    std::vector<cv::Point2i>* desired_size_list_;
    std::vector<cv::Point2i>::iterator desired_size_iter_;


protected:
    void findLabel(cv::Mat image_gray, cv::Mat &match_templ, std::vector<cv::Point2f> & points);
    cv::Mat getLabelImg(Mat img);
    void getROI(cv::Mat image, cv::Mat &dst);
    cv::Mat LOG(Mat img);
    void highConstract(Mat img, Mat & dst, int r);
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
    Detector(pthread_mutex_t* mutex, std::queue<string>* list, std::vector<string>* batch_origin_list, std::vector<int64_t>* batch_count_list, std::vector<cv::Point2i>* desired_size_list);
    Detector(pthread_mutex_t* mutex, std::queue<string>* list, std::vector<std::string>* work_id_list_, std::vector<int64_t>* work_count_list_, std::vector<string>* batch_origin_list, std::vector<int64_t>* batch_count_list);
    ~Detector();
    int setParam();
    int detect();
    int launchThread();
    void setOriginImg(string filename);
    void setOriginImg(Mat img);
    void setDesiredSize(cv::Point2i desired_size);
    void setImg(string filename);
    void setImg(Mat img);
    void setThresh();
    int sendMsg();
    int64_t getCount();
};


#endif // DETECTOR_H
