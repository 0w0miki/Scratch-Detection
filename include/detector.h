#if !defined(DETECTOR_H)
#define DETECTOR_H

#include "DetectUtils.h"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include "json/json.h"

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
    
    // 参数
    float k_pos_;
    float k_scratch_;
    float k_bigpro_;

    // id
    int64_t id_;
    
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
    // void crossIntegral(cv::Mat M1, cv::Mat M2, cv::Mat& crosssum);
    // cv::Mat getMerge(Mat src,int m,int n,int width,int height,int style);
    // cv::Mat getSum(Mat src,int m,int n,int width,int height);
    // cv::Mat getNCC(Mat origin_img,Mat img,Mat ointegral, Mat iintegral, int m, int n,int width,int height);

public:
    Detector();
    Detector(char* filename);
    ~Detector();
    int setParam();
    int detect();
    void setOriginImg(Mat img);
    void setImg(Mat img);
    void setThresh();
};


#endif // DETECTOR_H
