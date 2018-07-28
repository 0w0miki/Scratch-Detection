#include "detector.h"

Detector::Detector(/* args */)
{
    std::vector<cv::Point2f> points(4);
    img_points_ = points;
}

Detector::~Detector()
{
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
    cv::equalizeHist(image_gray,image_gray);
    // adaptiveThreshold(image_gray, image_bin, 255, CV_ADAPTIVE_THRESH_MEAN_C, CV_THRESH_BINARY, 27, 5);

    image_bin = image_gray > 40;
    Erosion(image_bin, image_bin,0,12);
    Dilation(image_bin, image_bin,0,12);
    // namedWindow("gray",CV_WINDOW_NORMAL);
    // imshow("gray", image_bin);
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
    // imshow("origin", image_bin);
    // waitKey(0);
    // 找四条直线
    int i,j;
    std::vector<std::vector<cv::Point2f> > linePoints(4);
    std::vector<std::vector<cv::Point> > lines(4);
    image_bin2=image_bin.clone();

    for(i=0;i<4;i++){
        cv::Vec4f line;
        findLinePoint(lines[i],image_bin,i,200,1);
        // std::cout << lines[0]<<"," <<lines[1]<<"," <<lines[2]<<"," <<lines[3]<< std::endl;
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
        // std::cout <<point1.x<<","<< point1.y<<"  ,  "<<point2.x<<","<<point2.y<< std::endl;
        cv::line(image_bin2, p1, p2, cv::Scalar(255), 10, 8, 0);
    }
    
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
cv::Mat Detector::getLabelImg(Mat img, int width, int height){
    float size_thresh_k = 0.6;
    Mat image_bin, dst;
    int size_thresh = size_thresh_k * width * height;
    cout<<"width, height"<<width<<","<<height<<endl;
    cout<<"thresh"<<size_thresh<<endl;
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
\return int 
*/ 
//-------------------------------------------------
int Detector::checkPos(Mat & adjust_match) {
    // 偏差
    cv::Mat diff;
    vector<Mat> mats;
    int i,j;
    for(i=0;i<3;i++){
        for(j=0;j<3;j++){
            Mat mat;
            mat= Mat::zeros(img_.rows, img_.cols, CV_8U);
            image_transfrom(img_, mat,j-1,i-1);
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
    return diff_sum[0];
}

//-------------------------------------------------
/**
\brief 尺寸检测
\param [in] points 角点
\param [in] d_width 期望宽
\param [in] d_height 期望高
\param [in] size_thresh 阈值
\return void
*/ 
//-------------------------------------------------
void Detector::checkSize(std::vector<Point2f> & points, float d_width, float d_height, float size_thresh) {
    // 1 2
    // 4 3
    float width = 0;
    float height = 0;
    int i;
    for(i = 0; i < 4; i+=2){
        float xw = points[i+1].x - points[i].x;
        float yw = points[i+1].y - points[i].y;
        width = max(width,xw * xw + yw * yw);//123212.64286916\120909
    }
    for(i = 0; i < 2; i++){
        float xw = points[3-i].x - points[i].x;
        float yw = points[3-i].y - points[i].y;
        height = max(height,xw * xw + yw * yw);
    }
    std::cout << "width:"<<width<<"height:"<<height << '\n';
    if(abs(width - d_width*d_width)>size_thresh && abs(height - d_height*d_height)>size_thresh){
        std::cout << "size fault" << '\n';
    }
    //369330,370568
}

//-------------------------------------------------
/**
\brief 划痕检测
\param [in] template_img 模板图像
\param [in] det_img 待测图像
\param [in] diff 结果图像
\return int 
*/ 
//-------------------------------------------------
int Detector::checkScratch(Mat & diff) {
    Mat common,A,B,template_label_bin,diff_white;
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
    std::cout << "diff score"<<diff_sum[0] << '\n';
    // namedWindow("diff",CV_WINDOW_NORMAL);
    // imshow("diff",diff);
    return diff_sum[0];
}

//-------------------------------------------------
/**
\brief 显著瑕疵检测
\param [in] template_img 模板图像
\param [in] det_img 待测图像
\param [in] diff 结果图像
\return int 
*/ 
//-------------------------------------------------
int Detector::checkBigProblem(Mat &bpres){
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
    bitwise_xor(P,common,bpres);
    Scalar diff_sum = sum(bpres)/255;
    std::cout << "bp score"<<diff_sum[0] << '\n';
    
    // namedWindow("wldiff",CV_WINDOW_NORMAL);
    // imshow("wldiff",bpres);
    // imshow("P",P);
    // imshow("T",T);
    return diff_sum[0];
}

void Detector::crossIntegral(cv::Mat M1, cv::Mat M2, cv::Mat& crosssum) {
    Mat product = Mat();
    Mat M1_64 = Mat();
    Mat M2_64 = Mat();
    // check size
    if (M1.size()!=M2.size()) {
        return;
    }

    M1.convertTo(M1_64,CV_64F);
    M2.convertTo(M2_64,CV_64F);
    // 对应元素相乘 注意需要转成CV_64F
    product = M1_64.mul(M2_64);
    cv::integral(product, crosssum, CV_64F);
}

void Detector::setOriginImg(cv::Mat img){
    template_img_ = img;
    template_label_ = getLabelImg(template_img_, template_img_.size().width, template_img_.size().height);
}

void Detector::setImg(cv::Mat img){
    img_ = Mat::zeros(template_img_.rows, template_img_.cols,CV_8U);
    findLabel(img, img_, img_points_);
    
    //label_ = Mat::zeros(template_label_.rows, template_label_.cols,CV_8U);
    label_ = getLabelImg(img_, img_.size().width, img_.size().height);
    adjustSize(label_, template_label_);
    // imshow("label_",label_);
    // waitKey();
    // getROI(img_, label_);
}