#if !defined(DETECTOR_H)
#define DETECTOR_H

#include "DetectUtils.h"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/opencv.hpp>

class Detector
{
private:
    cv::Mat template_img_;
    cv::Mat template_label_;
    cv::Mat template_img_resize_;
    cv::Mat img_;
    cv::Mat img_resize_;
    cv::Mat label_;
    std::vector<Point2f> img_points_;
protected:
    void findLabel(cv::Mat image_gray, cv::Mat &match_templ, std::vector<cv::Point2f> & points);
    void getROI(cv::Mat image, cv::Mat &dst);
    cv::Mat LOG(Mat img);
    void highConstract(Mat img, Mat & dst, int r);
    
    cv::Mat search(Mat img, Mat templ);
    void crossIntegral(cv::Mat M1, cv::Mat M2, cv::Mat& crosssum);
    cv::Mat getMerge(Mat src,int m,int n,int width,int height,int style);
    cv::Mat getSum(Mat src,int m,int n,int width,int height);
    cv::Mat getNCC(Mat origin_img,Mat img,Mat ointegral, Mat iintegral, int m, int n,int width,int height);

public:
    Detector();
    ~Detector();
    void checkSize(std::vector<Point2f> & points, float d_width, float d_height, float size_thresh);
    int checkPos(Mat & adjust_match);
    int checkScratch(Mat & diff);
    int checkBigProblem(Mat &bpres);
    cv::Mat getLabelImg(Mat img, int width, int height);
    void setOriginImg(Mat img);
    void setImg(Mat img);
};


#endif // DETECTOR_H
