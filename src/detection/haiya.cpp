#include <iostream>
#include <map>
#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include <string.h>
//#include "opencv2/ml.hpp"
#include <time.h>

using namespace std;
using namespace cv;

#define bin_thresh 50	//70
#define roi_thresh 90	//40
#define img_dir "images/new/6.png"

void image_init(Mat src_img, Mat& roi_img)
{
	Mat gray_img, bin_img;
	vector<vector<Point>> contours;
	// 灰度化
	cvtColor(src_img, gray_img, CV_BGR2GRAY);
	// 去噪
	// blur(gray_img, gray_img, Size(6, 6));
	// 二值化
	threshold(gray_img, bin_img, bin_thresh, 255, CV_THRESH_BINARY);
	// 腐蚀膨胀
	// Mat element = getStructuringElement(0, Size(3, 3));
	// erode(bin_img, bin_img, element);
	// dilate(bin_img, bin_img, element);
	// 寻找轮廓
	findContours(bin_img, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
	roi_img = bin_img.clone();
	for (int i = 0; i < contours.size(); i++)
	{
		//drawContours(bin_img, contours, i, Scalar(255), 1);
		drawContours(roi_img, contours, i, Scalar(255), -1);
	}
}

Mat roi_get(Mat src_img, Mat& roi_img)
{
	vector<vector<Point>> roi_contours;
	RotatedRect roi_rect;
	Mat roi;
	Size image_size(src_img.size());

	findContours(roi_img, roi_contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
	for (int i = 0; i < roi_contours.size(); i++)
	{
		roi_rect = minAreaRect(roi_contours[i]);
		if (roi_rect.size.width > 1000 && roi_rect.size.height > 500)
		{
			Point2f vertices[4];
			roi_rect.points(vertices);
			for (int k = 0; k < 4; k++) {
				line(roi_img, vertices[k], vertices[(k + 1) % 4], Scalar(255), 3);
			}
			// 旋转使矩形摆正
			Mat rotation = getRotationMatrix2D(roi_rect.center, roi_rect.angle, 1.0);;
			Mat rot_img;
			int imgx, imgy, imgwidth, imgheight;

			if (roi_rect.size.width > roi_rect.size.height)
			{
				imgx = 0;
				imgy = roi_rect.center.y - (roi_rect.size.height / 2.0) > 0 ? roi_rect.center.y - (roi_rect.size.height / 2.0) : 0;
				imgwidth = roi_img.cols;
				imgheight = roi_rect.center.y + (roi_rect.size.height / 2.0) < roi_img.rows ? roi_rect.size.height : roi_img.rows - imgy;
			}
			else
			{
				imgx = roi_rect.center.x - (roi_rect.size.width / 2.0) > 0 ? roi_rect.center.x - (roi_rect.size.width / 2.0) : 0;
				imgy = 0;
				imgwidth = roi_rect.center.x + (roi_rect.size.width / 2.0) < roi_img.cols ? roi_rect.size.width : roi_img.cols - imgy;
				imgheight = roi_img.rows;
			}
				
			warpAffine(src_img, rot_img, rotation, image_size);
			//namedWindow("rot_img", WINDOW_NORMAL);
			//imshow("rot_img", rot_img);

			// 旋转矩形摆正后的坐标
			roi = rot_img(Rect(imgx, imgy, imgwidth, imgheight));

			// 统一方向
			
			if (roi_rect.size.width < roi_rect.size.height)
			{
				transpose(roi, roi);
				flip(roi, roi, 0);
			}
			// 统一形状
			resize(roi, roi, image_size);
		}
	}
	return roi;
}

// 如果摄像头提前标定，消除摄像头畸变，不需要这一步
void roi_init(Mat roi)
{
	Mat gray_roi, bin_roi;
	Mat temp_roi;
	// 灰度化
	cvtColor(roi, gray_roi, CV_BGR2GRAY);
	// 去噪
	//blur(gray_roi, gray_roi, Size(6, 6));
	// 二值化
	threshold(gray_roi, bin_roi, bin_thresh, 255, CV_THRESH_BINARY);
	// 腐蚀膨胀
	Mat element = getStructuringElement(0, Size(3, 3));
	//erode(bin_roi, bin_roi, element);
	//dilate(bin_roi, bin_roi, element);

	// 角点检测
	// pass;

	namedWindow("bin_roi", WINDOW_NORMAL);
	imshow("bin_roi", bin_roi);
}

void edge_detect(Mat roi)
{
	Mat gray_roi, bin_roi;
	Mat temp_roi;
	vector<vector<Point>> roi_contours;
	vector<Vec4i> hierarchy;
	// 灰度化
	cvtColor(roi, gray_roi, CV_BGR2GRAY);
	// 去噪
	blur(gray_roi, gray_roi, Size(3, 3));

	// sobel
	
	Mat dx, dy;
	Sobel(gray_roi, dx, CV_16S, 1, 0, 3, 1, BORDER_DEFAULT);
	convertScaleAbs(dx, dx);
	Sobel(gray_roi, dy, CV_16S, 0, 1, 3, 1, BORDER_DEFAULT);
	convertScaleAbs(dy, dy);
	addWeighted(dx, 2.0, dy, 1.0, 0, gray_roi);
	
	// canny
	//Canny(gray_roi, gray_roi, 3, 9, 3);
	// 二值化
	threshold(gray_roi, bin_roi, roi_thresh, 255, CV_THRESH_BINARY);
	
	// 开闭操作
	Mat element1 = getStructuringElement(0, Size(3, 3));
	morphologyEx(bin_roi, bin_roi, MORPH_OPEN, element1);
	morphologyEx(bin_roi, bin_roi, MORPH_CLOSE, element1);
	
	// 腐蚀膨胀
	Mat element2 = getStructuringElement(0, Size(5, 5));
	dilate(bin_roi, bin_roi, element2);
	//erode(bin_roi, bin_roi, element2);
	
	findContours(bin_roi, roi_contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE);
	Mat contoursImage(bin_roi.rows, bin_roi.cols, CV_8U, Scalar(255));
	for (int i = 0; i<roi_contours.size(); i++) {
		if (hierarchy[i][3] != -1)
			drawContours(contoursImage, roi_contours, i, Scalar(0), 2);
	}

	namedWindow("gray_roi", WINDOW_NORMAL);
	imshow("gray_roi", bin_roi);
}

int main()
{
	// 定义变量
	Mat src_img = imread(img_dir);
	Mat roi_img, roi;
	// 调用函数
	image_init(src_img, roi_img);
	roi = roi_get(src_img, roi_img);
	//roi_init(roi);
	edge_detect(roi);

	// 显示图像
	namedWindow("roi_img", WINDOW_NORMAL);
	imshow("roi_img", roi_img);
	if (!roi.empty())
	{
		namedWindow("roi", WINDOW_NORMAL);
		imshow("roi", roi);
	}

	waitKey(0);
	return 0;
}