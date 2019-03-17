
#ifndef DETECT_UTILS_H_
#define DETECT_UTILS_H_


#include <iostream>
#include <chrono>
#include <fstream>
#include <vector>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "opencv2/imgproc/imgproc.hpp"
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

#define CUT_VERTICAL_HEAD   1
#define CUT_VERTICAL_TAIL   -1
#define CUT_HORIZON_LEFT    2
#define CUT_HORIZON_RIGHT   -2

// 腐蚀
void Erosion(const Mat& src, Mat& dst, int erosion_elem, int erosion_size );

// 膨胀
void Dilation( const Mat& src, Mat& dst, int dilation_elem, int dilation_size );

// 获取偏差
int getDiff(const Mat & src,Point & point,const int direction,const int first,const int second);

// 找到图中边界
void findLinePoint(vector<Point>& points,const Mat& src,int direction,int threshold, int diff=60);

// 画出边线
void drawLine(cv::Mat &image, double theta, double rho, cv::Scalar color);

typedef vector<Point2f> LINE;

// 得到两直线交点
void CrossPoint(Point2f & pt,const LINE & line1, const LINE & line2);

// 图像变换
void image_transfrom1(Mat & src, int du=0,int dv=0);

// 图像变换
void image_transfrom(const Mat & src,Mat & dst, int du=0,int dv=0);

// 得到偏差最小图像
void image_min(vector<Mat>& mats, Mat& templ);

// 得到共同区域
Mat getCommon(Mat origin_img, Mat img, int step);

// 移除小区域
void removeSmallRegion(Mat& src, Mat& dst,int areaLimit, int checkMode, int neighborMode);

// 快速傅里叶变换
void myDft(Mat& src, Mat& dst, Mat& show_dst);

// 傅里叶反变换
void myIdft(Mat& src, Mat& dst);

// Sobel边缘检测
void getSobel(Mat src_gray, Mat & grad, int order);

// 调整大小和第二个图一致
void adjustSize(Mat &img1, Mat img2);

void cutRatio(Mat &img, float ratio, int direction = CUT_VERTICAL_HEAD);

void cutPx(Mat &img, int px, int direction = CUT_VERTICAL_HEAD);

Mat getPaper(Mat src_img, Mat& roi_img);

#endif
