#include "detector.h"

int main ( int argc, char** argv )
{
    Detector detector;

    auto t0 = chrono::system_clock::now();
    cv::Mat image;
    // 灰度图像
    cv::Mat image_gray;
    // 模板
    cv::Mat templ;
    // 调整尺寸的模板
    cv::Mat templ_resize;
    // 带特征点的模板
    cv::Mat templ_with_keypoints;
    // 图像
    cv::Mat match_templ;
    // 图像
    cv::Mat adjust_match;
    // 带关键点的图像
    cv::Mat match_with_keypoints;
    // 偏差
    cv::Mat diff;
    // bpresult
    cv::Mat bpres;
    // 原图积分
    cv::Mat ImageIntegralSum;
    // 测试图积分
    cv::Mat OriginImageIntegralSum;
    // 交点
    std::vector<cv::Point2f> points(4);
    std::vector<cv::Point2f> templ_points(4);

    // 判断阈值参数
    int fault_thresh = 180000;
    int scratch_thresh = 40000;
    int bigpro_thresh = 60;
    float d_width = 350;
    float d_height = 600;
    float size_thresh = 3600;

    int i,j;
    string dir = "../images/test/IMG_";
    string suffix = ".jpg";
    // vector<int> list = {3,6,8,10,12,16};
    string Input_Path = dir;
    string Output_name;
    string Output_Path = "../Output/images/";
    ofstream outfile;
    outfile.open("../Output/result.csv",ios::out);
    outfile<<"id,value,state,scratch score,scratch state\n";
    templ = cv::imread("../images/templates/template.JPG",0);
    cv::resize(templ, templ_resize, cv::Size(templ.cols/3, templ.rows/3), (0, 0), (0, 0), cv::INTER_LINEAR);
    
    //template image
    detector.setOriginImg(templ);
    // templ = detector.getLabelImg(templ,templ.size().width,templ.size().height);
    // namedWindow("templ",WINDOW_NORMAL);
    // imshow("templ",templ);
    //cv::integral(templ_resize, OriginImageIntegralSum, CV_64F);

    for (size_t filei = 20; filei < 21; filei++) {
        Input_Path += to_string(filei);
        // Input_Path += to_string(list[filei]);
        Input_Path += suffix;
        cout<<Input_Path;
        image = cv::imread ( Input_Path ); 
        image_gray= cv::imread( Input_Path,0 );
        match_templ= Mat::zeros(templ.rows,templ.cols,CV_8U);
        cout<<image_gray.size()<<endl;
        
        // cv::resize(image_gray, image_gray, cv::Size(image_gray.cols/3, image_gray.rows/3), (0, 0), (0, 0), cv::INTER_LINEAR);
        // cout<<"图像宽为"<<image.cols<<",高为"<<image.rows<<",通道数为"<<image.channels()<<endl;
        // getROI(image_gray,match_templ);
        detector.setImg(image_gray);
       
        auto t_bp_bef = chrono::system_clock::now();
        int big_score = detector.checkBigProblem(bpres);
        auto t_bp_end = chrono::system_clock::now();
        cout<< "big problerm time" << chrono::duration_cast<chrono::microseconds>(t_bp_end-t_bp_bef).count() << "us\n";
        
        auto t_checkPos_bef = chrono::system_clock::now();
        int final_result = detector.checkPos(adjust_match);
        auto t_checkPos_end = chrono::system_clock::now();
        cout<< "check Position time" << chrono::duration_cast<chrono::microseconds>(t_checkPos_end-t_checkPos_bef).count()<< "us\n";

        auto t_scratch_bef = chrono::system_clock::now();
        int scratch_score = detector.checkScratch(diff);
        auto t_scratch_end = chrono::system_clock::now();
        cout<< "scratch time" << chrono::duration_cast<chrono::microseconds>(t_scratch_end-t_scratch_bef).count()<< "us\n";

        std::cout << filei << "final_result" << final_result << '\n';
        /*****************************************************************/
        // imshow("templ_resize", templ_resize);
        // imshow("match templ", match_templ);
        // imshow("ncc",ncc);
        // imshow("haha templ", adjust_match);
        // imshow("diff", diff);
        Output_name=Output_Path + "out" + to_string(filei) + suffix;
        // Output_name=Output_Path + "out" + to_string(list[filei]) + suffix;
        // imwrite(Output_name, ncc);
        imwrite(Output_name, 255 - adjust_match);
        Output_name=Output_Path + "scratch" + to_string(filei) + suffix;
        // Output_name=Output_Path + "scratch" + to_string(list[filei]) + suffix;
        imwrite(Output_name, diff);
        Output_name=Output_Path + "bp" + to_string(filei) + suffix;
        // Output_name=Output_Path + "bp" + to_string(list[filei]) + suffix;
        imwrite(Output_name, bpres);
        Input_Path = dir;
        outfile<<filei<<","<<final_result<<",";
        if(final_result < fault_thresh){
            outfile << ",";
            // outfile<<filei<<","<<final_result<<'\n';
        }else{
            outfile<<"fault,";
            // outfile<<filei<<","<<final_result<<","<<"fault"<<'\n';
        }
        outfile<<big_score;
        if(big_score < bigpro_thresh){
            outfile << ",,";
            // outfile<<filei<<","<<final_result<<'\n';
        }else{
            outfile<<",bigpro fault"<<',';
            // outfile<<filei<<","<<final_result<<","<<"fault"<<'\n';
        }
        outfile<<scratch_score;
        if(scratch_score < scratch_thresh){
            outfile << '\n';
            // outfile<<filei<<","<<final_result<<'\n';
        }else{
            outfile<<",scratch fault"<<'\n';
            // outfile<<filei<<","<<final_result<<","<<"fault"<<'\n';
        }
    }
    auto t_end = chrono::system_clock::now();
    auto d = chrono::duration_cast<chrono::milliseconds>(t_end-t0);
    cout << "run time = "<< d.count() << "ms\n";
    outfile<<'\n'<<"run time:," << d.count()<<"ms\n";
    outfile.close();


    //cv::waitKey ( 0 );
    // }

    cv::destroyAllWindows();
    return 0;
}
