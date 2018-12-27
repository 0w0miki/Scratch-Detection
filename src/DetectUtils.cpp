#include "DetectUtils.h"

//-------------------------------------------------
/**
\brief 腐蚀
\param [in] src 原图
\param [out] dst 腐蚀后图像
\param [in] erosion_elem 核类型
\param [in] erosion_size 核尺寸
\return void
*/ 
//-------------------------------------------------
void Erosion(const Mat& src, Mat& dst, int erosion_elem, int erosion_size )
{
  int erosion_type;
  if( erosion_elem == 0 ){ erosion_type = MORPH_RECT; }
  else if( erosion_elem == 1 ){ erosion_type = MORPH_CROSS; }
  else if( erosion_elem == 2) { erosion_type = MORPH_ELLIPSE; }

  Mat element = getStructuringElement( erosion_type,
                                       Size( 2*erosion_size + 1, 2*erosion_size+1 ),
                                       Point( erosion_size, erosion_size ) );

  /// 腐蚀操作
  erode( src, dst, element );
//   imshow( "Erosion Demo", dst );
}

//-------------------------------------------------
/**
\brief 膨胀
\param [in] src 原图
\param [out] dst 腐蚀后图像
\param [in] erosion_elem 核类型
\param [in] erosion_size 核尺寸
\return void
*/ 
//-------------------------------------------------
void Dilation( const Mat& src, Mat& dst, int dilation_elem, int dilation_size )
{
  int dilation_type;
  if( dilation_elem == 0 ){ dilation_type = MORPH_RECT; }
  else if( dilation_elem == 1 ){ dilation_type = MORPH_CROSS; }
  else if( dilation_elem == 2) { dilation_type = MORPH_ELLIPSE; }

  Mat element = getStructuringElement( dilation_type,
                                       Size( 2*dilation_size + 1, 2*dilation_size+1 ),
                                       Point( dilation_size, dilation_size ) );
  ///膨胀操作
  dilate( src, dst, element );
//   imshow( "Dilation Demo", dst );
}

//-------------------------------------------------
/**
\brief 获取偏差
\param [in] src 原图
\param [in] point 腐蚀后图像
\param [in] direction 方向
\param [in] first 
\param [in] second
\return int
*/ 
//-------------------------------------------------
int getDiff(const Mat & src,Point & point,const int direction,const int first,const int second){

    if (direction == 0){
        // 上
        point.x=first;
        point.y=second;
        return abs(src.at<uchar>(first, second-1) - src.at<uchar>(first,second));
    }else if (direction ==1 ){
        point.x=src.cols - second;
        point.y=first;
        // 右
        return abs(src.at<uchar>(src.cols - second+1, first) - src.at<uchar>(src.cols - second,first));
    }else if (direction ==2 ){
        point.x=first;
        point.y=src.rows-second;
        // 下
        return abs(src.at<uchar>(first, src.rows-second-1) - src.at<uchar>(first,src.rows-second));
    }else if (direction ==3 ){
        point.x=second;
        point.y=first;
        // 左
        return abs(src.at<uchar>(second,first-1 ) - src.at<uchar>(second,first));
    }
}

//-------------------------------------------------
/**
\brief 找到图中边界
\param [in,out] points 原图
\param [in] src 原图
\param [in] direction 方向
\param [in] threshold 搜索阈值
\param [in] diff 阈值
\return void
*/ 
//-------------------------------------------------
void findLinePoint(vector<Point>& points,const Mat& src,int direction,int threshold, int diff){
    // 上右下左分对应1234
    uint iter_num;
    int a;
    int i,j;
    int faster=1;

    Point pre_p;
    // 上,迭代长边
    // 其实是左边
    if(direction==0){
        for( i=1;i<src.rows-1;i++){
            // 迭代短边
            for( j=faster;j<threshold-1;j++){
                if(src.at<uchar>(i, j+1) - src.at<uchar>(i,j)>diff){
                    // cout << i <<"," <<j << endl;
                    points.push_back(Point(i,j));
                    faster= ((j>100)?(j-99):1);
                    break;
                }
            }
        }
    }
    // 右,迭代长边
    // 其实是上边
    if(direction==1){
        for( i=1;i<src.cols-1;i++){
            // 迭代短边
            for( j=faster;j<threshold-1;j++){
                if(abs(src.at<uchar>(j, i) - src.at<uchar>(j-1,i))>diff){
                    // cout << j <<"," <<i << endl;
                    points.push_back(Point(j,i));
                    faster= ((j>100)?(j-99):1);
                    break;
                }
            }
        }
    }

    // 下,迭代长边
    // 其实是右边, 从右往左找,改变的是u,也就是第二个
    if(direction==2){
        for( i=1;i<src.rows-1;i++){
            // 迭代短边
            for( j=faster;j<threshold-1;j++){
                if(abs(src.at<uchar>(i, src.cols-j) - src.at<uchar>(i,src.cols-j+1))>diff){
                    // cout << i<<"," <<src.cols-j<< endl;
                    points.push_back(Point(i,src.cols-j));
                    faster= ((j>100)?(j-99):1);
                    break;
                }
            }
        }
    }
    // 下,迭代长边
    if(direction==3){
        for( i=1;i<src.cols-1;i++){
            // 迭代短边
            for( j=faster;j<threshold-1;j++){
                if(abs(src.at<uchar>(src.rows-j,i) - src.at<uchar>(src.rows-j+1,i))>diff){
                    // cout << j <<"," <<i << endl;
                    points.push_back(Point(src.rows-j,i));
                    faster= ((j>100)?(j-99):1);
                    break;
                }
            }
        }
    }

}

//-------------------------------------------------
/**
\brief 画出边线
\param [in,out] image 图像
\param [in] theta 直线参数
\param [in] rho 直线参数
\param [in] color 颜色
\return void
*/ 
//-------------------------------------------------
void drawLine(cv::Mat &image, double theta, double rho, cv::Scalar color)
{
    if (theta < CV_PI/4. || theta > 3.*CV_PI/4.)// ~vertical line
    {
        cv::Point pt1(rho/cos(theta), 0);
        cv::Point pt2((rho - 2*image.rows * sin(theta))/cos(theta), 2*image.rows);
        // cv::Point pt1(rho/cos(theta), 0);
        // cv::Point pt2((rho - image.rows * sin(theta))/cos(theta), image.rows);
        cv::line( image, pt1, pt2, cv::Scalar(255), 10);
    }
    else
    {
        cv::Point pt1(rho/sin(theta),0);
        cv::Point pt2( (rho - 2*image.cols * cos(theta))/sin(theta),2*image.cols);
        // cv::Point pt1(0, rho/sin(theta));
        // cv::Point pt2(image.cols, (rho - image.cols * cos(theta))/sin(theta));
        cv::line(image, pt1, pt2, cv::Scalar(255), 10);
    }
}

typedef vector<Point2f> LINE;

//-------------------------------------------------
/**
\brief 得到两直线交点
\param [out] pt 交点
\param [in] line1 直线1
\param [in] line2 直线2
\return void
*/ 
//-------------------------------------------------
void CrossPoint(Point2f & pt,const LINE & line1, const LINE & line2)
{
    //    if(!SegmentIntersect(line1->pStart, line1->pEnd, line2->pStart, line2->pEnd))
    //    {// segments not cross
    //        return 0;
    //    }
    // line1's cpmponent
    pt.x=0;
    pt.y=0;
    double X1 = line1[1].x - line1[0].x;//b1
    double Y1 = line1[1].y - line1[0].y;//a1
    // line2's cpmponent
    double X2 = line2[1].x - line2[0].x;//b2
    double Y2 = line2[1].y - line2[0].y;//a2
    // distance of 1,2
    double X21 = line2[0].x - line1[0].x;
    double Y21 = line2[0].y - line1[0].y;
    // determinant
    double D = Y1*X2 - Y2*X1;// a1b2-a2b1
    //
    if (D == 0) return ;
    // cross point
    pt.x = (X1*X2*Y21 + Y1*X2*line1[0].x - Y2*X1*line2[0].x) / D;
    // on screen y is down increased !
    pt.y = -(Y1*Y2*X21 + X1*Y2*line1[0].y - X2*Y1*line2[0].y) / D;
    // segments intersect.
    if ((fabs(pt.x - line1[0].x - X1 / 2) <= fabs(X1 / 2)) &&
        (fabs(pt.y - line1[0].y - Y1 / 2) <= fabs(Y1 / 2)) &&
        (fabs(pt.x - line2[0].x - X2 / 2) <= fabs(X2 / 2)) &&
        (fabs(pt.y - line2[0].y - Y2 / 2) <= fabs(Y2 / 2)))
    {
        return;
    }
}

//-------------------------------------------------
/**
\brief 图像变换
\param [in,out] src 图像
\param [in] du 
\param [in] dv 
\return void
*/ 
//-------------------------------------------------
void image_transfrom1(Mat & src, int du,int dv){
    int i,j;
    for(i=abs(du);i<src.cols-abs(du);i++){
        for(j=abs(dv);j<src.rows-abs(dv);j++){
            if(du<0){
                if(dv<0){
                    src.at<uchar>(src.rows-j,src.cols-i)=src.at<uchar>(src.rows-j+dv,src.cols-i+du);
                }else{
                    src.at<uchar>(j,src.cols-i)=src.at<uchar>(j+dv,src.cols-i+du);
                }
            }else if(dv<0){
                src.at<uchar>(src.rows-j,i)=src.at<uchar>(src.rows-j+dv,i+du);
            }else{
                src.at<uchar>(j,i)=src.at<uchar>(j+dv,i+du);
            }
        }
    }
}

//-------------------------------------------------
/**
\brief 图像变换
\param [in] src 输入图像
\param [out] dst 输出图像
\param [in] du
\param [in] dv 
\return void
*/ 
//-------------------------------------------------
void image_transfrom(const Mat & src,Mat & dst, int du, int dv){
    int i,j;
    for(i=abs(du);i<src.cols-abs(du);i++){
        for(j=abs(dv);j<src.rows-abs(dv);j++){
            if(du<0){
                if(dv<0){
                    dst.at<uchar>(src.rows-j,src.cols-i)=src.at<uchar>(src.rows-j+dv,src.cols-i+du);
                }else{
                    dst.at<uchar>(j,src.cols-i)=src.at<uchar>(j+dv,src.cols-i+du);
                }
            }else if(dv<0){
                dst.at<uchar>(src.rows-j,i)=src.at<uchar>(src.rows-j+dv,i+du);
            }else{
                dst.at<uchar>(j,i)=src.at<uchar>(j+dv,i+du);
            }
        }
    }
}

//-------------------------------------------------
/**
\brief 得到偏差最小图像
\param [in] mats 偏移后的图像
\param [in] templ 原图
\return void
*/ 
//-------------------------------------------------
void image_min(vector<Mat>& mats, Mat& templ){
    int i,j,k;
    for(i=0;i<mats.size();i++){
        for(j=0;j<templ.cols;j++){
            for(k=0;k<templ.rows;k++){
                templ.at<uchar>(k,j) = min(abs(templ.at<uchar>(k,j)),abs(mats[i].at<uchar>(k,j)));
            }
        }
    }
}

//-------------------------------------------------
/**
\brief 得到共同区域
\param [in] origin_img 原图
\param [in] img 待测图
\param [in] step
\return Mat
*/ 
//-------------------------------------------------
Mat getCommon(Mat origin_img, Mat img, int step) {
    // 找到两图中的共同区域
    Mat A = img;
    Mat B = Mat::zeros(A.size(),CV_8U);
    int i,j;
    for (i = -step; i <= step; i++) {
        for (j = -step; j <= step; j++) {
            A = img;
            // 扩充边界为0
            copyMakeBorder(A,A,step,step,step,step,BORDER_CONSTANT,Scalar(0));
            // 移动
            A = A(Range(step+i,img.rows+step+i),Range(step+j,step+img.cols+j));
            // 与操作找白色交集
            bitwise_and(A,origin_img,A);
            copyMakeBorder(A,A,step,step,step,step,BORDER_CONSTANT,Scalar(0));
            // 反向移回
            A = A(Range(step-i,img.rows+step-i),Range(step-j,step-j+img.cols));
            // 对结果找白色并集
            bitwise_or(B,A,B);
        }
    }
    return B;
}

//-------------------------------------------------
/**
\brief 移除小区域
\param [in] src 原图
\param [out] dst 输出图像
\param [in] areaLimit 区域大小阈值
\param [in] checkMode 检验模式 1:去除小区域 other:去除孔洞
\param [in] neighborMode 邻域模式 1:4邻域 other:8邻域
\return Mat
*/ 
//-------------------------------------------------
void removeSmallRegion(Mat& src, Mat& dst,int areaLimit, int checkMode, int neighborMode)
{
    int removeCount=0;
    Mat pointLabel = Mat::zeros(src.size(), CV_8UC1);
    if(checkMode==1)
    {
        // cout<<"Mode: 去除小区域. ";
        for(int i = 0; i < src.rows; ++i)
        {
            uchar* iData = src.ptr<uchar>(i);
            uchar* iLabel = pointLabel.ptr<uchar>(i);
            for(int j = 0; j < src.cols; ++j)
            {
                if (iData[j] < 10)
                {
                    iLabel[j] = 3;
                }
            }
        }
    }
    else
    {
        // cout<<"Mode: 去除孔洞. ";
        for(int i = 0; i < src.rows; ++i)
        {
            uchar* iData = src.ptr<uchar>(i);
            uchar* iLabel = pointLabel.ptr<uchar>(i);
            for(int j = 0; j < src.cols; ++j)
            {
                if (iData[j] > 10)
                {
                    iLabel[j] = 3;
                }
            }
        }
    }

    vector<Point2i> neighborPos;  //记录邻域点位置
    neighborPos.push_back(Point2i(-1, 0));
    neighborPos.push_back(Point2i(1, 0));
    neighborPos.push_back(Point2i(0, -1));
    neighborPos.push_back(Point2i(0, 1));
    if (neighborMode==1)
    {
        // cout<<"Neighbor mode: 8邻域."<<endl;
        neighborPos.push_back(Point2i(-1, -1));
        neighborPos.push_back(Point2i(-1, 1));
        neighborPos.push_back(Point2i(1, -1));
        neighborPos.push_back(Point2i(1, 1));
    }
    // else cout<<"Neighbor mode: 4邻域."<<endl;
    int NeihborCount=4+4*neighborMode;
    int CurrX=0, CurrY=0;
    //开始检测
    for(int i = 0; i < src.rows; ++i)
    {
        uchar* iLabel = pointLabel.ptr<uchar>(i);
        for(int j = 0; j < src.cols; ++j)
        {
            if (iLabel[j] == 0)
            {
                //********开始该点处的检查**********
                vector<Point2i> GrowBuffer;                                      //堆栈，用于存储生长点
                GrowBuffer.push_back( Point2i(j, i) );
                pointLabel.at<uchar>(i, j)=1;
                int CheckResult=0;                                               //用于判断结果（是否超出大小），0为未超出，1为超出

                for ( int z=0; z<GrowBuffer.size(); z++ )
                {

                    for (int q=0; q<NeihborCount; q++)                                      //检查四个邻域点
                    {
                        CurrX=GrowBuffer.at(z).x+neighborPos.at(q).x;
                        CurrY=GrowBuffer.at(z).y+neighborPos.at(q).y;
                        if (CurrX>=0&&CurrX<src.cols&&CurrY>=0&&CurrY<src.rows)  //防止越界
                        {
                            if ( pointLabel.at<uchar>(CurrY, CurrX)==0 )
                            {
                                GrowBuffer.push_back( Point2i(CurrX, CurrY) );  //邻域点加入buffer
                                pointLabel.at<uchar>(CurrY, CurrX)=1;           //更新邻域点的检查标签，避免重复检查
                            }
                        }
                    }

                }
                if (GrowBuffer.size()>areaLimit) CheckResult=2;                 //判断结果（是否超出限定的大小），1为未超出，2为超出
                else {CheckResult=1;   removeCount++;}
                for (int z=0; z<GrowBuffer.size(); z++)                         //更新Label记录
                {
                    CurrX=GrowBuffer.at(z).x;
                    CurrY=GrowBuffer.at(z).y;
                    pointLabel.at<uchar>(CurrY, CurrX) += CheckResult;
                }
                //********结束该点处的检查**********
            }
        }
    }

    checkMode=255*(1-checkMode);
    //开始反转面积过小的区域
    for(int i = 0; i < src.rows; ++i)
    {
        uchar* iData = src.ptr<uchar>(i);
        uchar* iDstData = dst.ptr<uchar>(i);
        uchar* iLabel = pointLabel.ptr<uchar>(i);
        for(int j = 0; j < src.cols; ++j)
        {
            if (iLabel[j] == 2)
            {
                iDstData[j] = checkMode;
            }
            else if(iLabel[j] == 3)
            {
                iDstData[j] = iData[j];
            }
        }
    }

    // cout<<removeCount<<" objects removed."<<endl;

}

//-------------------------------------------------
/**
\brief 快速傅里叶变换
\param [in] src 原图
\param [out] dst 输出图像
\param [in] show_dst 
\return Mat
*/ 
//-------------------------------------------------
void myDft(Mat& src, Mat& dst, Mat& show_dst){
  Mat src_copy;
  src.convertTo(src_copy, CV_32FC1 );

  Mat padded;                            //将输入图像延扩到最佳的尺寸
  int m = getOptimalDFTSize( src_copy.rows );
  int n = getOptimalDFTSize( src_copy.cols ); // 在边缘添加0
  copyMakeBorder(src_copy, padded, 0, m - src_copy.rows, 0, n - src_copy.cols, BORDER_CONSTANT, Scalar::all(0));

  Mat planes[] = {Mat_<float>(padded), Mat::zeros(padded.size(), CV_32F)};
  merge(planes, 2, src_copy);         // 为延扩后的图像增添一个初始化为0的通道

  // 需要转换颜色空间
  cv::dft(src_copy,src_copy);
  dst=src_copy.clone();
  split(src_copy, planes);                   // planes[0] = Re(DFT(I), planes[1] = Im(DFT(I))
  magnitude(planes[0], planes[1], planes[0]);// planes[0] = magnitude
  Mat magI = planes[0];

  magI += Scalar::all(1);                    // 转换到对数尺度
  log(magI, magI);

  int cx = magI.cols/2;
  int cy = magI.rows/2;

  Mat q0(magI, Rect(0, 0, cx, cy));   // Top-Left - 为每一个象限创建ROI
  Mat q1(magI, Rect(cx, 0, cx, cy));  // Top-Right
  Mat q2(magI, Rect(0, cy, cx, cy));  // Bottom-Left
  Mat q3(magI, Rect(cx, cy, cx, cy)); // Bottom-Right

  Mat tmp;                           // 交换象限 (Top-Left with Bottom-Right)
  q0.copyTo(tmp);
  q3.copyTo(q0);
  tmp.copyTo(q3);

  q1.copyTo(tmp);                    // 交换象限 (Top-Right with Bottom-Left)
  q2.copyTo(q1);
  tmp.copyTo(q2);

  normalize(magI, magI, 0, 1, CV_MINMAX); // 将float类型的矩阵转换到可显示图像范围
  // (float [0， 1]).
  show_dst=magI.clone();

}

//-------------------------------------------------
/**
\brief 傅里叶反变换
\param [in] src 原图
\param [out] dst 输出图像
\return Mat
*/ 
//-------------------------------------------------
void myIdft(Mat& src, Mat& dst){
  cv::idft(src,dst,DFT_SCALE | DFT_REAL_OUTPUT );
}

//-------------------------------------------------
/**
\brief Sobel边缘检测
\param [in,out] src_gray 原图
\param [in] grad 输出
\param [in] order 阶数
\return Mat
*/ 
//-------------------------------------------------
void getSobel(Mat src_gray, Mat & grad, int order) {
    Mat grad_x, grad_y;
    Mat abs_grad_x, abs_grad_y;

    /// 求 X方向梯度
    Sobel( src_gray, grad_x, -1, order, 0);
    convertScaleAbs( grad_x, abs_grad_x );

    /// 求Y方向梯度
    Sobel( src_gray, grad_y, -1, 0, order);
    convertScaleAbs( grad_y, abs_grad_y );

    /// 合并梯度(近似)
    addWeighted( abs_grad_x, 0.5, abs_grad_y, 0.5, 0, grad );
}

//-------------------------------------------------
/**
\brief 调整图像到同一尺寸
\param [in,out] img1 原图
\param [in] img2 输出
\return Mat
*/ 
//-------------------------------------------------
void adjustSize(Mat &img1, Mat img2){
    std::vector<cv::Point2f> origin_points(4);
    origin_points[0].x=0;
    origin_points[0].y=0;
    origin_points[1].x=img1.cols;
    origin_points[1].y=0;
    origin_points[2].x=img1.cols;
    origin_points[2].y=img1.rows;
    origin_points[3].x=0;
    origin_points[3].y=img1.rows;
    // 透视变换
    std::vector<cv::Point2f> perspective_points(4);
    perspective_points[0].x=0;
    perspective_points[0].y=0;
    perspective_points[1].x=img2.cols;
    perspective_points[1].y=0;
    perspective_points[2].x=img2.cols;
    perspective_points[2].y=img2.rows;
    perspective_points[3].x=0;
    perspective_points[3].y=img2.rows;
    cv::Mat transform;
    cv::Size size(img2.cols,img2.rows);
    // 得到矩阵
    transform=cv::getPerspectiveTransform(origin_points,perspective_points);
    // std::cout <<"transform mat"<< transform << std::endl;
    // 然后变换
    cv::warpPerspective(img1, img1, transform, size);
}


void cutRatio(Mat &img, float ratio, int direction){
    int height = img.rows;
    int width = img.cols;
    Rect part(0,0,width,height);
    switch (direction)
    {
        case CUT_VERTICAL_HEAD:
            part.height = static_cast<int>(height - height*ratio);
            part.y = static_cast<int>(height*ratio);
            break;
        case CUT_VERTICAL_TAIL:
            part.height = static_cast<int>(height - height*ratio);
            break;
        case CUT_HORIZON_LEFT:
            part.width = static_cast<int>(width - width*ratio);
            part.x = static_cast<int>(width*ratio);
            break;
        case CUT_HORIZON_RIGHT:
            part.width = static_cast<int>(width*ratio);
            break;
    }
    img = img(part);
}

/**
 * @brief           获取纸张部分ROI
 * 
 * @param src_img   原始图像
 * @param roi_img   分割后图像
 * @return Mat 
 */
Mat getPaper(Mat src_img, Mat& roi_img)
{
	vector<vector<Point>> roi_contours;
	RotatedRect roi_rect;
	Mat roi;
	Size image_size(src_img.size());

	findContours(roi_img, roi_contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
	for (int i = 0; i < roi_contours.size(); i++)
	{
		roi_rect = minAreaRect(roi_contours[i]);
		if (roi_rect.size.width > src_img.cols/2 && roi_rect.size.height > src_img.rows/2)
		{
			Point2f vertices[4];
			roi_rect.points(vertices);
			for (int k = 0; k < 4; k++) {
				line(roi_img, vertices[k], vertices[(k + 1) % 4], Scalar(255), 3);
			}
			// 旋转使矩形摆正
            // printf("angle %f, width %f, height %f",roi_rect.angle,roi_rect.size.width, roi_rect.size.height);
			Mat rotation;
			Mat rot_img;
			int imgx, imgy, imgwidth, imgheight;

			if (roi_rect.size.width > roi_rect.size.height)
			{
				//     ┌---------┐
                //     |         |
                //     └---------┘
                rotation = getRotationMatrix2D(roi_rect.center, roi_rect.angle, 1.0);
                imgx = 0;
				imgy = roi_rect.center.y - (roi_rect.size.height / 2.0) > 0 ? roi_rect.center.y - (roi_rect.size.height / 2.0) : 0;
				imgwidth = roi_img.cols;
				imgheight = roi_rect.center.y + (roi_rect.size.height / 2.0) < roi_img.rows ? roi_rect.size.height : roi_img.rows - imgy;
			}
			else
			{
                //    ┌----┐
                //    |    |
                //    |    |
                //    |    |
                //    |    |
                //    └----┘
                rotation = getRotationMatrix2D(roi_rect.center, roi_rect.angle+90, 1.0);
				imgx = 0;
				imgy = roi_rect.center.y - (roi_rect.size.width / 2.0) > 0 ? roi_rect.center.y - (roi_rect.size.width / 2.0) : 0;
				imgwidth = roi_img.cols;
				imgheight = roi_rect.center.y + (roi_rect.size.width / 2.0) < roi_img.rows ? roi_rect.size.width : roi_img.rows - imgy;
			}

            warpAffine(src_img, rot_img, rotation, image_size);
			namedWindow("rot_img", WINDOW_NORMAL);
			imshow("rot_img", rot_img);
            waitKey();

            // printf("x %d, y %d, width %d, height %d",imgx,imgy,imgwidth, imgheight);
            cout<<roi_img.size()<<endl;
			// 旋转矩形摆正后的坐标
			roi = rot_img(Rect(imgx, imgy, imgwidth, imgheight));
            // namedWindow("rot_img", WINDOW_NORMAL);
			// imshow("rot_img", roi);
            // waitKey();

			// 统一形状
			// resize(roi, roi, image_size);
		}
	}
	return roi;
}