#include "Detector.h"

Detector::Detector():
    bin_thresh_(40),
    k_pos_(0.01),
    k_scratch_(0.001),
    k_bigpro_(0.001),
    start_detect_(false),
    save_result_switch_(true),
    save_img_switch_(true),
    show_time_switch_(true),
    origin_flag(false),
    detection_thread_(2),
    camera_(NULL),
    template_dir_("../images/templates/"),
    img_dir_("../images/test/"),
    d_width_(400),
    d_height_(600),
    count_(1),
    id_(1)
{
    std::vector<cv::Point2f> points(4);
    img_points_ = points;
    string filename = "result.csv";
    result_log_.open(filename,ios::out);
    if(!result_log_){
        cout<< "[WARN]failed to open log file"<<endl;
    }
    result_log_<<"id,value,state,big pro score,big pro state,scratch score,scratch state\n";

    input_type_ = DETECTA4;
}

Detector::Detector( pthread_mutex_t* mutex, std::queue<string>* list ):
    bin_thresh_(40),
    k_pos_(0.01),
    k_scratch_(0.001),
    k_bigpro_(0.001),
    start_detect_(false),
    save_result_switch_(true),
    save_img_switch_(true),
    show_time_switch_(true),
    origin_flag(false),
    detection_thread_(2),
    camera_(NULL),
    template_dir_("../images/templates/"),
    img_dir_("../images/test/"),
    d_width_(400),
    d_height_(600),
    count_(1),
    id_(1)
{
    std::vector<cv::Point2f> points(4);
    img_points_ = points;
    string filename = "result.csv";
    result_log_.open(filename,ios::out);
    if(!result_log_){
        cout<< "[WARN]failed to open log file"<<endl;
    }
    result_log_<<"id,value,state,big pro score,big pro state,scratch score,scratch state\n";
    
    mutex_ = mutex;
    unsolved_list_ = list;
}

Detector::Detector(
    pthread_mutex_t* mutex, 
    std::queue<string>* list, 
    std::deque<string>* batch_origin_list, 
    std::deque<int64_t>* batch_count_list, 
    std::vector<cv::Point2i>* desired_size_list
):
    bin_thresh_(40),
    k_pos_(0.01),
    k_scratch_(0.001),
    k_bigpro_(0.001),
    start_detect_(false),
    save_result_switch_(true),
    save_img_switch_(true),
    show_time_switch_(true),
    origin_flag(false),
    detection_thread_(2),
    camera_(NULL),
    template_dir_("../images/templates/"),
    img_dir_("../images/test/"),
    d_width_(400),
    d_height_(600),
    count_(1),
    id_(1)
{
    std::vector<cv::Point2f> points(4);
    img_points_ = points;
    string filename = "result.csv";
    result_log_.open(filename,ios::out);
    if(!result_log_){
        cout<< "[WARN]failed to open log file"<<endl;
    }
    result_log_<<"id,value,state,big pro score,big pro state,scratch score,scratch state\n";
    
    mutex_ = mutex;
    unsolved_list_ = list;

    batch_origin_list_ = batch_origin_list;
    batch_count_list_ = batch_count_list;
    desired_size_list_ = desired_size_list;
    desired_size_iter_ = desired_size_list_->begin();

    input_type_ = DETECTA4;
}

Detector::Detector(
    pthread_mutex_t* mutex, 
    pthread_mutex_t* result_mutex, 
    Json::Value* root, 
    std::queue<string>* list, 
    std::deque<std::string>* work_name_list, 
    std::deque<int64_t>* work_count_list, 
    std::deque<string>* batch_origin_list, 
    std::deque<int64_t>* batch_count_list
):
    bin_thresh_(40),
    k_pos_(0.01),
    k_scratch_(0.001),
    k_bigpro_(0.001),
    start_detect_(false),
    save_result_switch_(true),
    save_img_switch_(true),
    show_time_switch_(true),
    origin_flag(false),
    detection_thread_(2),
    camera_(NULL),
    template_dir_("../images/templates/"),
    img_dir_("../images/test/"),
    d_width_(400),
    d_height_(600),
    count_(1),
    id_(1)
{
    std::vector<cv::Point2f> points(4);
    img_points_ = points;
    string filename = "result.csv";
    result_log_.open(filename,ios::out);
    if(!result_log_){
        cout<< "[WARN]failed to open log file"<<endl;
    }
    result_log_<<"id,value,state,big pro score,big pro state,scratch score,scratch state\n";
    
    mutex_ = mutex;
    result_mutex_ = result_mutex;
    unsolved_list_ = list;
    result_root_ = root;

    work_name_list_ = work_name_list;
    work_count_list_ = work_count_list;
    batch_origin_list_ = batch_origin_list;
    batch_count_list_ = batch_count_list;
    
    input_type_ = DETECTA4;
}

Detector::~Detector()
{
    start_detect_ = false;
    if(result_mutex_ != NULL){
        pthread_mutex_unlock(result_mutex_);
        result_mutex_ = NULL;
    }
    
    result_log_.close();
}

//-------------------------------------------------
/**
\brief 提取带白边的标签图片
\param [in] image_gray 拍摄图像
\param [out] match_templ 待测图
\param [out] points 四个角点
\return void
*/ 
//-------------------------------------------------
void Detector::findLabel(cv::Mat image_gray, cv::Mat &det_img, std::vector<cv::Point2f> & points) {
    cv::Mat image_bin;
    cv::Mat image_bin2;
    // 二值化
    // cv::equalizeHist(image_gray,image_gray);
    // adaptiveThreshold(image_gray, image_bin, 255, CV_ADAPTIVE_THRESH_MEAN_C, CV_THRESH_BINARY, 27, 5);
    image_bin = image_gray > bin_thresh_;
    Erosion(image_bin, image_bin,0,12);
    Dilation(image_bin, image_bin,0,12);
    
    // namedWindow("gray",CV_WINDOW_NORMAL);
    // imshow("gray", image_gray);
    // waitKey(0);
    
    std::vector<std::vector<cv::Point> > contours;
    std::vector<cv::Vec4i> hierarchy;
    cv::findContours(image_bin, contours, hierarchy, cv::RETR_CCOMP, cv::CHAIN_APPROX_SIMPLE);
        // iterate through all levels, and draw contours in random color
    int index = 0;
    for (; index>=0; index = hierarchy[index][0]) {
        cv::Scalar color(255);
        // for opencv 2
        // cv::drawContours(dstImage, contours, index, color,  CV_FILLED, 8, hierarchy);
        // for opencv 3
        cv::drawContours(image_bin, contours, index, color,  CV_FILLED, 8, hierarchy);
    }

    // namedWindow("origin",CV_WINDOW_NORMAL);
    // imshow("origin", image_bin);
    // waitKey(0);
    
    // 找四条直线
    int i,j;
    std::vector<std::vector<cv::Point2f> > linePoints(4);
    std::vector<std::vector<cv::Point> > lines(4);
    image_bin2=image_bin.clone();

    for(i=0;i<4;i++){
        cv::Vec4f line;
        findLinePoint(lines[i],image_bin,i,2000,5);
        // std::cout << lines[0]<<"," <<lines[1]<<"," <<lines[2]<<"," <<lines[3]<< std::endl;
        // 拟合直线
        cv::fitLine(lines[i], line, CV_DIST_HUBER, 0, 0.01, 0.01);
        std::cout << line[0]<<"," <<line[1]<<"," <<line[2]<<"," <<line[3]<< std::endl;

        cv::Point point0;
        point0.x = line[2];
        point0.y = line[3];
        double k = line[1] / line[0];
        cv::Point2f point1, point2;
        cv::Point2f p1,p2;
        // std:: cout <<k<< endl;
        point1.x = 0;
        point1.y = k * (0 - point0.x) + point0.y;
        point2.x = 5000;
        point2.y = k * (5000 - point0.x) + point0.y;
        p1.x=point1.y;
        p1.y=point1.x;
        p2.x=point2.y;
        p2.y=point2.x;
        linePoints[i].push_back(p1);
        linePoints[i].push_back(p2);
        // std::cout <<point1.x<<","<< point1.y<<"  ,  "<<point2.x<<","<<point2.y<< std::endl;
        cv::line(image_bin2, p1, p2, cv::Scalar(255), 10, 8, 0);
    }

    // namedWindow("with line",CV_WINDOW_NORMAL);
    // imshow("with line", image_bin2);
    // waitKey(0);
    
    // 四个交点
    for(i=0;i<4;i++){
        CrossPoint(points[i],linePoints[i],linePoints[(i+1)%4]);
        // cout << points[i].x<<","<< points[i].y << endl;
    }

    // 透视变换
    std::vector<cv::Point2f> perspective_points(4);
    perspective_points[0].x=0;
    perspective_points[0].y=0;
    perspective_points[1].x=det_img.cols;
    perspective_points[1].y=0;
    perspective_points[2].x=det_img.cols;
    perspective_points[2].y=det_img.rows;
    perspective_points[3].x=0;
    perspective_points[3].y=det_img.rows;
    cv::Mat transform;
    cv::Size size(det_img.cols,det_img.rows);
    // 得到矩阵
    transform=cv::getPerspectiveTransform(points,perspective_points);
    // std::cout <<"transform mat"<< transform << std::endl;
    // 然后变换
    cv::warpPerspective(image_gray,det_img,transform,size);
    // imshow("final", det_img);
    // waitKey();
}

//-------------------------------------------------
/**
\brief 通过黑点找到图像
\param [in] image 待测图
\param [out] dst 输出图
\return void
*/ 
//-------------------------------------------------
void Detector::getROI(cv::Mat image, cv::Mat &dst){
    Mat square_bin, paper_bin, paper_black;
    Mat mask = Mat::zeros(image.rows,image.cols,CV_8U);
    
    // find small square
    auto t_square_bef = chrono::system_clock::now();
    threshold(image,square_bin,50,255,1);
    std::vector<std::vector<cv::Point> > contours;
    std::vector<cv::Vec4i> hierarchy;
    std::vector<RotatedRect> locate_square;
    RotatedRect cur_square;
    findContours(square_bin, contours, hierarchy, CV_RETR_CCOMP, cv::CHAIN_APPROX_SIMPLE);
    for (int index = 0; index>=0; index = hierarchy[index][0]) {
        cv::Scalar color(255);
        RotatedRect minRect = minAreaRect(contours[index]);
        if(abs(minRect.size.width/minRect.size.height-1)<0.1 && abs(minRect.size.width*minRect.size.height-655)<400 
        && (minRect.size.width*minRect.size.height-400)>0){
            // minRect is left square
            //cv::drawContours(mask, contours, index, color, 2, 8, hierarchy);
            locate_square.push_back(minRect);
        }
    }
    contours.clear();
    hierarchy.clear();
    if(locate_square.empty()){
        cout<<"[ERROR] cannot locate the label"<<endl;
        return;
    }
    int square_y = image.cols;
    int square_y_i = -1;
    for(size_t i=0;i<locate_square.size();i++){
        if(locate_square[i].center.y<100){
            continue;
        }
        if(square_y > locate_square[i].center.y){
            square_y =  locate_square[i].center.y;
            square_y_i = i;
        }
    }
    if(square_y_i == -1){
        cout<<"[ERROR] no square satisfy requirement"<<endl;
        return;
    }
    cout<<square_y<<","<<square_y_i<<endl;
    cur_square = locate_square[square_y_i];
    auto t_square_end = chrono::system_clock::now();
    cout << "find square time "<< chrono::duration_cast<chrono::microseconds>(t_square_end-t_square_bef).count() << "us\n";


    // find paper
    auto t_mask_bef = chrono::system_clock::now();
    threshold(image,paper_bin,120,255,0);
    paper_black = paper_bin.clone();
    Point2f vertices[4];
    cur_square.points(vertices);
    int minx=vertices[0].x, miny = vertices[0].y;
    for(size_t i=1;i<4;i++){
        minx = (minx < vertices[i].x) ? minx : vertices[i].x;
        miny = (miny < vertices[i].y) ? miny : vertices[i].y;
    }
    // fill white part
    auto t_flood_bef = chrono::system_clock::now();
    floodFill(paper_black,Point(minx-1,miny-1),0);
    auto t_flood_end = chrono::system_clock::now();
    cout << "flood fill time "<< chrono::duration_cast<chrono::microseconds>(t_flood_end-t_flood_bef).count() << "us\n";

    // leave only paper
    paper_bin = paper_bin - paper_black;
    findContours(paper_bin, contours, hierarchy, CV_RETR_CCOMP, cv::CHAIN_APPROX_SIMPLE);
    Point2f label_corner[4];
    RotatedRect ROI;
    for(size_t i = 0; i < contours.size(); i++)
    {
        RotatedRect minRect = minAreaRect(contours[i]);
        // large part and current black square in the same line
        if((minRect.size.width * minRect.size.height) > 1000*500 
           && pointPolygonTest(contours[i],Point(minRect.center.x,cur_square.center.y),false)==1
           && pointPolygonTest(contours[i],cur_square.center,false)==-1){
            cv::drawContours(mask, contours, i, Scalar(255), 1, 8);
            minRect.points(label_corner);
            ROI = minRect;
        }
    }
    auto t_mask_end = chrono::system_clock::now();
    cout << "find mask time "<< chrono::duration_cast<chrono::microseconds>(t_mask_end-t_mask_bef).count() << "us\n";

    // perspective transformation
    auto t_findline_bef = chrono::system_clock::now();
    vector<Point2f> corners(4);
    Mat image_bin2 = mask.clone();
    std::vector<std::vector<cv::Point2f> > linePoints(4);
    std::vector<std::vector<cv::Point> > lines(4);
    for(size_t i=0;i<4;i++){
        cv::Vec4f line;
        findLinePoint(lines[i],mask,i,700,1);
        // std::cout << lines[i][0]<<"," <<lines[i][1]<<"," <<lines[i][2]<<"," <<lines[i][3]<< std::endl;
        // 拟合直线
        cv::fitLine(lines[i], line, CV_DIST_HUBER, 0, 0.01, 0.01);
        // std::cout << line[0]<<"," <<line[1]<<"," <<line[2]<<"," <<line[3]<< std::endl;

        cv::Point point0;
        point0.x = line[2];
        point0.y = line[3];
        double k = line[1] / line[0];
        cv::Point2f point1, point2;
        cv::Point2f p1,p2;
        // std:: cout <<k<< endl;
        point1.x = 0;
        point1.y = k * (0 - point0.x) + point0.y;
        point2.x = 5000;
        point2.y = k * (5000 - point0.x) + point0.y;
        p1.x=point1.y;
        p1.y=point1.x;
        p2.x=point2.y;
        p2.y=point2.x;
        linePoints[i].push_back(p1);
        linePoints[i].push_back(p2);
        // std:: cout <<point1.x<<","<< point1.y<<"  ,  "<<point2.x<<","<<point2.y<< std::endl;
        cv::line(image_bin2, p1, p2, cv::Scalar(255));
    }
    auto t_findline_end = chrono::system_clock::now();
    cout << "find line time "<< chrono::duration_cast<chrono::microseconds>(t_findline_end-t_findline_bef).count() << "us\n";
    // namedWindow("with line", WINDOW_NORMAL);
    // imshow("with line", image_bin2);
    auto t_corner_bef = chrono::system_clock::now();
    // 四个交点
    for(size_t i=0;i<4;i++){
        CrossPoint(corners[i],linePoints[i],linePoints[(i+1)%4]);
        // cout << corners[i].x<<","<< corners[i].y << endl;
    }
    auto t_corner_end = chrono::system_clock::now();
    cout << "find corner time "<< chrono::duration_cast<chrono::microseconds>(t_corner_end-t_corner_bef).count() << "us\n";

    auto t_trans_bef = chrono::system_clock::now();
    vector<Point2f> corners_trans(4);
    corners_trans[0] = Point2f(0,0);
    corners_trans[1] = Point2f(dst.cols,0);
    corners_trans[2] = Point2f(dst.cols,dst.rows);
    corners_trans[3] = Point2f(0,dst.rows);
    Mat transform = getPerspectiveTransform(corners,corners_trans);
    // cout<<transform<<endl;
    warpPerspective(image,dst,transform,dst.size());
    auto t_trans_end = chrono::system_clock::now();
    cout << "find trans time "<< chrono::duration_cast<chrono::microseconds>(t_trans_end-t_trans_bef).count() << "us\n";
    //image.copyTo(dst,mask);

    // cout<<locate_square.size()<<endl;
}

//-------------------------------------------------
/**
\brief 获取标签的图像
\param [in] image 待测图
\param [in] width 图像宽
\param [in] height 图像高
\return Mat 标签图像
*/ 
//-------------------------------------------------
cv::Mat Detector::getLabelImg(Mat img){
    float size_thresh_k = 0.6;
    Mat image_bin, dst;
    int size_thresh = size_thresh_k * d_width_ * d_height_;
    // cout<<"d_width_, d_height_" << d_width_ << "," << d_height_ << endl;
    // cout<<"thresh"<<size_thresh<<endl;
    std::vector<std::vector<cv::Point> > contours;
    std::vector<cv::Vec4i> hierarchy;
    Mat mask = Mat::zeros(img.size(),CV_8U);
    adaptiveThreshold(img, image_bin, 255, CV_ADAPTIVE_THRESH_MEAN_C, CV_THRESH_BINARY_INV, 15, 10);
    cv::findContours(image_bin, contours, hierarchy, CV_RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
    drawContours(mask, contours, -1, Scalar(255));
    // imshow("mask", mask);
    // waitKey();
    for(size_t i=0; i < contours.size(); i++){
        double area = contourArea(contours[i]);
        if(area > size_thresh){
            drawContours(mask, contours, i, Scalar(255));
            RotatedRect minRect = minAreaRect(contours[i]);
            Point2f fourPoint2f[4];
            minRect.points(fourPoint2f);
            double lowest = 0, logo_theta = 0;
            int x1,x2,y1,y2;
            Point low;
            for (int j = 0; j < 4; j++){
                if (fourPoint2f[j].y > lowest) {
                    lowest = fourPoint2f[j].y;
                    low = fourPoint2f[j];
                }
            }
            if (low.x < minRect.center.x) {
                // 中点在左侧，图像顺时针旋转
                logo_theta = minRect.angle;
                x1 = minRect.center.x - minRect.size.width/2;
                x2 = x1 + minRect.size.width;
                y1 = minRect.center.y - minRect.size.height/2;
                y2 = y1 + minRect.size.height;
            }else{
                // 中点在右侧，图像逆时针旋转
                logo_theta = minRect.angle + 90;
                x1 = minRect.center.x - minRect.size.height/2;
                x2 = x1 + minRect.size.height;
                y1 = minRect.center.y - minRect.size.width/2;
                y2 = y1 + minRect.size.width;
            }
            Mat rot_mat = getRotationMatrix2D(minRect.center, logo_theta, 1);
            warpAffine(img,dst,rot_mat,img.size());
            dst = dst(Range(y1,y2),Range(x1,x2));
            return dst;
        }
    }
}

//-------------------------------------------------
/**
\brief LOG卷积
\param [in] img 待测图
\return Mat 输出图像
*/ 
//-------------------------------------------------
Mat Detector::LOG(Mat img) {
    // Mat kern = (Mat_<char>(5,5) << 0, 0, -1, 0, 0,
    //                                0, 0, -2, 0, 0,
    //                               -1,-2, 16,-2,-1,
    //                                0, 0, -2, 0, 0,
    //                                0, 0, -1, 0, 0);
    Mat kern = (Mat_<char>(3,3) << 0, -1 ,0,
                                   -1, 5, -1,
                                   0, -1, 0);
    Mat dst;
    cv::filter2D(img,dst,img.depth(),kern);
    // namedWindow("dstImage",CV_WINDOW_NORMAL);
    // imshow("dstImage",dst);
    return dst;
}

//-------------------------------------------------
/**
\brief 保留图像高对比度区域
\param [in] img 图像
\param [out] dst 输出图像
\param [in] r 参数
\return void
*/ 
//-------------------------------------------------
void Detector::highConstract(Mat img, Mat & dst, int r) {
    Mat dil,ero,temp,diff1,diff;
    Dilation(img, dil,0,2);
    Erosion(dil, ero,0,2);
    absdiff(dil,ero,diff1);
    adaptiveThreshold(img, temp, 255, CV_ADAPTIVE_THRESH_MEAN_C, CV_THRESH_BINARY_INV, 27, 15);
    GaussianBlur(temp, temp, Size(11,11),1.4,1.4);
    dst = img + r*(img-temp);

    /********white********/ 
}

//-------------------------------------------------
/**
\brief 搜索和原图中相同的区域
\param [in] img 待测图
\param [in] template_img 模板图
\return Mat 标签图像
*/ 
//-------------------------------------------------
cv::Mat Detector::search(Mat img, Mat template_img){
    Mat comm;
    bitwise_and(img,template_img,comm);
    // imshow("comm",comm);
    return comm;
}

//-------------------------------------------------
/**
\brief 模切偏移检测
\param [in] template_img 模板图像
\param [in] det_img 待测图像
\param [in] adjust_match 结果图像
\return int 0:ok 1:fault
*/ 
//-------------------------------------------------
int Detector::checkPos() {
    // 偏差
    auto t_checkPos_bef = chrono::system_clock::now();
    cv::Mat adjust_match;
    cv::Mat diff;
    vector<Mat> mats;
    int i,j;
    for(i=0;i<3;i++){
        for(j=0;j<3;j++){
            Mat mat;
            mat= Mat::zeros(img_gray_.rows, img_gray_.cols, CV_8U);
            image_transfrom(img_gray_, mat,j-1,i-1);
            absdiff(template_img_, mat, diff);
            diff.convertTo(diff,CV_64F);
            diff=diff.mul(diff);
            // diff= diff.mul(diff);
            normalize(diff,diff,0,255,NORM_MINMAX);
            diff.convertTo(diff,CV_8U);
            mats.push_back(diff.clone());
            // cout << mats[12] << endl;
        }
    }
    // cout <<mats[4]<< endl;
    adjust_match=mats[4].clone();
    // 25个矩阵求最小值
    image_min(mats,adjust_match);
    // imshow("adjust_match",adjust_match);
    Scalar diff_sum = sum(adjust_match);
    if(save_img_switch_){
        saveImg("pos",255-adjust_match);
    }
    if(save_result_switch_){
        result_log_ << diff_sum[0]/255 << ",";
        if(diff_sum[0]/255 < pos_thresh_){
            result_log_ << ",";
            if(show_time_switch_){
                auto t_checkPos_end = chrono::system_clock::now();
                cout<< "check Position time " << chrono::duration_cast<chrono::milliseconds>(t_checkPos_end-t_checkPos_bef).count() << "ms\n";
            }
            return 0;
        }else{
            result_log_<<"position fault,";
            if(show_time_switch_){
                auto t_checkPos_end = chrono::system_clock::now();
                cout<< "check Position time " << chrono::duration_cast<chrono::milliseconds>(t_checkPos_end-t_checkPos_bef).count() << "ms\n";
            }
            return 1;
        }
    }
    // return diff_sum[0]/255;
}

//-------------------------------------------------
/**
\brief 尺寸检测
\param 
\return int 0 ok 1 fault
*/ 
//-------------------------------------------------
int Detector::checkSize() {
    // 1 2
    // 4 3
    auto t_size_bef = chrono::system_clock::now();
    float width2 = 0;
    float height2 = 0;
    int i;
    for(i = 0; i < 4; i+=2){
        float xw = img_points_[i+1].x - img_points_[i].x;
        float yw = img_points_[i+1].y - img_points_[i].y;
        width2 = max(width2,xw * xw + yw * yw);//123212.64286916\120909
    }
    for(i = 0; i < 2; i++){
        float xw = img_points_[3-i].x - img_points_[i].x;
        float yw = img_points_[3-i].y - img_points_[i].y;
        height2 = max(height2,xw * xw + yw * yw);
    }
    std::cout << "width2: "<<width2<<" height2: "<<height2 << '\n';
    if( abs(width2 - d_width_ * d_width_) > size_thresh_ && abs(height2 - d_height_ * d_height_) > size_thresh_ ){
        std::cout << "size fault" << '\n';
        if(save_result_switch_){
            result_log_ << "size fault,";
        }
        if(show_time_switch_){
            auto t_size_end = chrono::system_clock::now();
            cout<< "size time " << chrono::duration_cast<chrono::milliseconds>(t_size_end-t_size_bef).count() << "ms\n";
        }
        return 1;
    }else{
        if(save_result_switch_){
            result_log_ << "size ok,";
        }
        if(show_time_switch_){
            auto t_size_end = chrono::system_clock::now();
            cout<< "size time " << chrono::duration_cast<chrono::milliseconds>(t_size_end-t_size_bef).count() << "ms\n";
        }
        return 0;
    }
    //369330,370568
}

//-------------------------------------------------
/**
\brief 划痕检测
\param 
\return int 0: ok 1: fault
*/ 
//-------------------------------------------------
int Detector::checkScratch() {
    auto t_scratch_bef = chrono::system_clock::now();
    show_time_switch_;
    Mat common,A,B,template_label_bin,diff_white,diff;
    threshold(template_label_,template_label_bin,240,255,0);
    /****************** white ******************/
    // 检测有颜色部分的瑕疵
    highConstract(255 - template_label_,A,3);
    highConstract(255 - label_,B,3);
    threshold(A,A,250,255,1);
    threshold(B,B,250,255,1);
    // namedWindow("template_label_",CV_WINDOW_NORMAL);
    // imshow("template_label_",A);
    // namedWindow("label_",CV_WINDOW_NORMAL);
    // imshow("label_",B);
    common = getCommon(A,B,3);
    // namedWindow("common",CV_WINDOW_NORMAL);
    // imshow("common",common);
    bitwise_xor(B,common,diff_white);
    diff_white = search(diff_white,255-template_label_bin);
    // namedWindow("diff",CV_WINDOW_NORMAL);
    // imshow("diff",diff);
    /****************** black ******************/
    // 检测白色部分的瑕疵
    highConstract(template_label_,A,3);
    highConstract(label_,B,3);
    // namedWindow("template_label_",CV_WINDOW_NORMAL);
    // imshow("template_label_",A);
    // namedWindow("label_",CV_WINDOW_NORMAL);
    // imshow("label_",B);
    threshold(A,A,250,255,1);
    threshold(B,B,250,255,1);

    common = getCommon(A,B,3);
    // namedWindow("common",CV_WINDOW_NORMAL);
    // imshow("common",common);
    bitwise_xor(B,common,diff);
    diff = search(diff,template_label_bin);
    bitwise_or(diff,diff_white,diff);
    // absdiff(template_label_,label_,diff);
    Scalar diff_sum = sum(diff);
    std::cout << "diff score"<<diff_sum[0]/255 << '\n';
    // namedWindow("diff",CV_WINDOW_NORMAL);
    // imshow("diff",diff);

    // log
    if(save_img_switch_){
       saveImg("scratch", diff); 
    }
    if(save_result_switch_){
        result_log_ << diff_sum[0]/255;
        if(diff_sum[0]/255 < scratch_thresh_){
            result_log_ << '\n';
            if(show_time_switch_){
                auto t_scratch_end = chrono::system_clock::now();
                cout<< "scratch time " << chrono::duration_cast<chrono::milliseconds>(t_scratch_end-t_scratch_bef).count()<< "ms\n";
            }
            return 0;
        }else{
            result_log_ << ",scratch fault" << '\n';
            if(show_time_switch_){
                auto t_scratch_end = chrono::system_clock::now();
                cout<< "scratch time " << chrono::duration_cast<chrono::milliseconds>(t_scratch_end-t_scratch_bef).count()<< "ms\n";
            }
            return 1;
        }
    }
    // return diff_sum[0]/255;
}

//-------------------------------------------------
/**
\brief 显著瑕疵检测
\param 
\return int 0:ok 1:fault
*/ 
//-------------------------------------------------
int Detector::checkBigProblem(){
    auto t_bp_bef = chrono::system_clock::now();
    Mat res;
    int noise_thresh = 4;
    Mat diff, temp, common;
    vector<Mat> mats;
    Mat T = template_label_.clone();
    Mat P = label_.clone();
    // waitKey();
    cv::Size sizeT = T.size();
    cv::Size sizeP = P.size();
    cout<<sizeT<<endl;
    cout<<sizeP<<endl;

    adaptiveThreshold(P, P, 255, CV_ADAPTIVE_THRESH_MEAN_C, CV_THRESH_BINARY_INV, 97, 5);
    threshold(T,T,240,255,1);
    // adaptiveThreshold(T, T, 255, CV_ADAPTIVE_THRESH_MEAN_C, CV_THRESH_BINARY_INV, 11, 10);
    // imshow("Pthresh",P);
    removeSmallRegion(P,P,noise_thresh,1,1);

    common = getCommon(T,P,5);
    // namedWindow("wlcommon",CV_WINDOW_NORMAL);
    // imshow("wlcommon",common);
    bitwise_xor(P,common,res);
    Scalar diff_sum = sum(res);
    std::cout << "bp score"<<diff_sum[0]/255 << '\n';
    
    if(save_img_switch_){
        saveImg("bp", res);
    }
    // log
    if(save_result_switch_){
        result_log_ << diff_sum[0]/255;
        if(diff_sum[0]/255 < bigpro_thresh_){
            result_log_ << ",";
            if(show_time_switch_){
                auto t_bp_end = chrono::system_clock::now();
                cout<< "big problerm time " << chrono::duration_cast<chrono::milliseconds>(t_bp_end-t_bp_bef).count() << "ms\n";
            }
            return 0;
        }else{
            result_log_<<",bigpro fault"<<',';
            if(show_time_switch_){
                auto t_bp_end = chrono::system_clock::now();
                cout<< "big problerm time " << chrono::duration_cast<chrono::milliseconds>(t_bp_end-t_bp_bef).count() << "ms\n";
            }
            return 1;
        }
    }
    return diff_sum[0]/255;
}

//-------------------------------------------------
/**
\brief 设置原图
\param [in] filename 地址
\return void 
*/ 
//-------------------------------------------------
void Detector::setOriginImg(string filename){
    string file = template_dir_ + filename;
    template_img_ = cv::imread(file,0);
    cout<<file<<endl;
    
    if(input_type_ == DETECTA4){
        template_label_ = template_img_;
    }else if(input_type_ == DETECTEACH){
        template_label_ = getLabelImg(template_img_);
    }
    // cv::cvtColor(template_img_,template_label_,CV_BGR2GRAY);
    setThresh();
    count_ = 1;
}

//-------------------------------------------------
/**
\brief 设置原图
\param [in] img 原图
\return void 
*/ 
//-------------------------------------------------
void Detector::setOriginImg(cv::Mat img){
    template_img_ = img;
    if(input_type_ == DETECTA4){
        template_label_ = template_img_;
    }else if(input_type_ == DETECTEACH){
        template_label_ = getLabelImg(template_img_);
    }
    setThresh();
    count_ = 1;
}

//-------------------------------------------------
/**
\brief 设置待测图像
\param [in] filename 文件地址
\return void 
*/ 
//-------------------------------------------------
void Detector::setImg(string filename){
    
    string file = img_dir_ + filename;
    Mat src = cv::imread(file,0);
    Mat img;
    transpose(src, img);
    flip(img,img,0);
    img_gray_ = Mat::zeros(template_img_.rows, template_img_.cols,CV_8U);
    findLabel(img, img_gray_, img_points_); 
    label_ = Mat::zeros(template_label_.rows, template_label_.cols,CV_8U);
    // label_ = getLabelImg(img_gray_);
    // imshow("img",img_gray_);
    // waitKey();
    adjustSize(label_, template_label_);
    
    // imshow("label_",label_);
    // waitKey();
    // getROI(img_gray_, label_);
}

//-------------------------------------------------
/**
\brief 设置待测图像
\param [in] img 拍到的ROI图像
\return void 
*/ 
//-------------------------------------------------
void Detector::setImg(cv::Mat img){
    img_gray_ = Mat::zeros(template_img_.rows, template_img_.cols,CV_8U);
    findLabel(img, img_gray_, img_points_);
    
    //label_ = Mat::zeros(template_label_.rows, template_label_.cols,CV_8U);
    label_ = getLabelImg(img_gray_);
    adjustSize(label_, template_label_);
    // imshow("label_",label_);
    // waitKey();
    // getROI(img_gray_, label_);
}

//-------------------------------------------------
/**
\brief 保存图片
\param [in] pre 前缀
\param [in] img 图片
\return void 
*/ 
//-------------------------------------------------
void Detector::saveImg(string pre, cv::Mat img){
    string Output_Path = "../Output/images/";
    string suffix = ".jpg";
    string Output_name = Output_Path + pre + to_string(id_) + suffix; 
    imwrite(Output_name, img);
}

void Detector::setCameraPtr(Camera* camera){
    camera_ = camera;
}

int Detector::launchThread(){
    int ret = pthread_create(&detection_thread_, NULL, detector_pth, this);
    if(ret != 0)
    {
        printf("<Failed to create the detection thread>\n");
        return -1;
    }
    pthread_detach(detection_thread_);
    return 0;
}

int Detector::setParam(){
	std::ifstream paramfile;
	paramfile.open("../settings.json", std::ios::binary);
    if(!paramfile){
        printf("[ERROR] failed to open settings.json\n");
        return -1;
    }else{
        Json::CharReaderBuilder builder;
	    Json::Value root;
        builder["collectComments"] = false;
        JSONCPP_STRING errs;
        if (parseFromStream(builder, paramfile, &root, &errs)){
            bin_thresh_ = root["detection"]["thresh"]["bin thresh"].empty()  ?  bin_thresh_  :  root["detection"]["thresh"]["bin thresh"].asFloat();
            k_pos_      = root["detection"]["thresh"]["k_pos"].empty()       ?  k_pos_       :  root["detection"]["thresh"]["k_pos"].asFloat();
            k_scratch_  = root["detection"]["thresh"]["k_scratch"].empty()   ?  k_scratch_   :  root["detection"]["thresh"]["k_scratch"].asFloat();
            k_bigpro_   = root["detection"]["thresh"]["k_bigpro"].empty()    ?  k_bigpro_    :  root["detection"]["thresh"]["k_bigpro"].asFloat();
            save_img_switch_ = root["detection"]["switch"]["save img"].empty()           ? save_img_switch_    : root["detection"]["switch"]["save img"].asBool();
            save_result_switch_ = root["detection"]["switch"]["save result log"].empty() ? save_result_switch_ : root["detection"]["switch"]["save result log"].asBool();
            show_time_switch_ = root["detection"]["switch"]["show time"].empty()         ? show_time_switch_   : root["detection"]["switch"]["show time"].asBool();
            template_dir_ = root["detection"]["file"]["template directory"].empty()      ? template_dir_       : root["detection"]["file"]["template directory"].asString();
            img_dir_ = root["detection"]["file"]["image directory"].empty()              ? img_dir_            : root["detection"]["file"]["image directory"].asString();
            input_type_ = root["detection"]["switch"]["detect each"].empty() ? input_type_ : root["detection"]["switch"]["detect each"].asInt();
        }else{
            cout << "[ERROR] Jsoncpp error: " << errs << endl;
        }
        paramfile.close();
    }
    // setThresh();
    return 0;
}

void Detector::setThresh(){

    Mat temp = template_img_ > bin_thresh_;
    Scalar temp_sum = sum(temp);
    cout<<"temp sum: "<<temp_sum[0]/255<<endl;

    Mat temp_label = template_label_ > bin_thresh_;
    Scalar label_sum = sum(temp_label);
    cout<<"temp label sum: "<<label_sum[0]/255<<endl;

    pos_thresh_ = temp_sum[0]/255 * k_pos_;
    scratch_thresh_ = label_sum[0]/255 * k_scratch_;
    bigpro_thresh_ = label_sum[0]/255 * k_bigpro_;
    size_thresh_ = 3600;
    
    // d_width_ = 1900;
    // d_height_ = 1300;
}

int Detector::detect(){
    if(work_name_list_->empty()){
        return -10;
    }
    int size_res = checkSize();
    int pos_res = checkPos();
    int bigpro_res = checkBigProblem();
    int scratch_res = checkScratch();
    
    int8_t result = 0;
    if(pos_res == 1){
        // 模切偏移
        result = result | 1;
    }
    if(bigpro_res && scratch_res){
        // 墨迹
        result = result | 2;
    }
    if(scratch_res){
        // 划痕，折痕
        result = result | 4;
    }
    // 保存json文件
    if(result != 0){
        writeResJson(result);
    }
    id_++;
    count_++;
    return 0;
}

void Detector::ProcDetect(){
    start_detect_ = true;
    while(start_detect_){
        pthread_mutex_lock(mutex_);
        if(unsolved_list_->empty()){
            // 没有待处理文件
            pthread_mutex_unlock(mutex_);
            // sleep for 2ms
            struct timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = 2 * 1000;
            select(0,NULL,NULL,NULL,&tv);
            continue;
        }
        string unsolved_filename = unsolved_list_->front();
        pthread_mutex_unlock(mutex_);
        cout<<"unsolved filename:"<<unsolved_filename<<endl;
        std::vector<std::string> split_name;
        SplitString(unsolved_filename, split_name, "_");
        if(!batch_origin_list_->empty() && false == origin_flag){
            
            std::string origin_name = split_name[1];
            origin_name.append("/");
            origin_name.append(batch_origin_list_->front());
            setOriginImg(origin_name);
            printf("<----------- set new origin image ----------->\n");
            origin_flag = true;
        }
        setImg(unsolved_filename);
        int ret = detect();
        if(ret != 0){

        }
        
        if(!batch_count_list_->empty() && count_ > batch_count_list_->front()){
            batch_origin_list_->pop_front();
            batch_count_list_->pop_front();
            if(!batch_origin_list_->empty()){
                setOriginImg(batch_origin_list_->front());
            }else{
                printf("[WARN] origin image list end.\n");
                origin_flag = false;
            }
            
            if(input_type_ == DETECTEACH){
                desired_size_iter_++;
                cout<<(*desired_size_iter_).x<<endl;
                if(desired_size_iter_ != desired_size_list_->end()){
                    setDesiredSize(*desired_size_iter_);
                    cout<<"test"<<endl;
                }else{
                    printf("[WARN] desired size list end.\n");
                }
            }
        }
        if(!work_count_list_->empty() && id_ > work_count_list_->front()){
            id_ = 1;
            std::cout << "detection pop" << std::endl;
            work_count_list_->pop_front();
            work_name_list_->pop_front();
            if(camera_ != NULL)
                camera_->popList();
        }

        pthread_mutex_lock(mutex_);
        unsolved_list_->pop();
        pthread_mutex_unlock(mutex_);

        // 发送消息

        // sleep for 2ms
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 500;
        select(0,NULL,NULL,NULL,&tv);
    }
}

void* Detector::detector_pth(void* args){
    Detector *_this = (Detector *)args;
    _this->ProcDetect();
}

int64_t Detector::getCount(){
    return count_;
}

void Detector::setDesiredSize(cv::Point2i desired_size){
    d_width_ = desired_size.x;
    d_height_ = desired_size.y;
}

void Detector::writeResJson(int8_t result){
    printf("<-------------------Write json file------------------------>");
    // Json::Value root;
    Json::Value batch_item;
    Json::StreamWriterBuilder wbuilder;
    wbuilder["indentation"] = "";
    std::unique_ptr<Json::StreamWriter> writer(wbuilder.newStreamWriter());

    // To Do: 修改返回内容
    std::string work_name = work_name_list_->front();
    std::vector<std::string> split_name;
    SplitString(work_name,split_name,"_");
    std::cout<<split_name[0]<<std::endl;
    if(split_name[0] == "reprint"){
        // 重打
        int work_id  = std::stoi(split_name[1]);
        batch_item["printJobID"] = work_id;
        int work_number = std::stoi(split_name[2]);
        batch_item["printWorkNumber"] = work_number;
    }else{
        // 新打
        int work_id  = std::stoi(split_name[1]);
        batch_item["printJobID"] = work_id;
        batch_item["printWorkNumber"] = id_;
    }
    batch_item["faultType"] = result;
    
    pthread_mutex_unlock(result_mutex_);
    result_root_->append(batch_item);
    batch_item.clear();
    std::cout << "'" << result_root_->toStyledString() << "'" << std::endl;
    pthread_mutex_unlock(result_mutex_);
    // char* url = "http://127.0.0.1:7999/api/result";
    // int ret = http_post_json(url, *result_root_);
    // writer->write(root, &file);
    // resFile.close();

    // return 0;
}

int Detector::stopThread(){
    start_detect_ = false;
    pthread_cancel(this->detection_thread_);
}