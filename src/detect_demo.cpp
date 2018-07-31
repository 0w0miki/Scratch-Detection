#include "Detector.h"

int main ( int argc, char** argv )
{
    auto t0 = chrono::system_clock::now();
    int ret = 0;
    cv::Mat image;
    // 灰度图像
    cv::Mat image_gray;
    // 模板
    cv::Mat templ;
    // 图像
    cv::Mat match_templ;

    Detector detector;
    ret = detector.setParam();

    string dir = "../images/test/IMG_";
    string suffix = ".bmp";
    string Input_Path = dir;
    templ = cv::imread("../images/templates/template.JPG",0);

    detector.setOriginImg(templ);
    
    for (size_t filei = 1; filei < 2; filei++) {
        Input_Path += to_string(filei);
        // Input_Path += to_string(list[filei]);
        Input_Path += suffix;
        cout<<Input_Path;
        image = cv::imread ( Input_Path ); 
        image_gray= cv::imread( Input_Path, 0 );
        // namedWindow("image",WINDOW_NORMAL);
        // imshow("image",image_gray);
        // waitKey();
        match_templ= Mat::zeros(templ.rows,templ.cols,CV_8U);
        cout<<image_gray.size()<<endl;
        
        detector.setImg(image_gray);
        detector.setThresh();
        detector.detect();
    
    }
    auto t_end = chrono::system_clock::now();
    auto d = chrono::duration_cast<chrono::milliseconds>(t_end-t0);
    cout << "run time = "<< d.count() << "ms\n";
    cv::destroyAllWindows();
    return 0;
}
