#include "Camera.h"

Camera::Camera():
cam_id_(1),
fake_ptr_(0),
g_device_(NULL),
g_frame_data_({0}),
g_raw8_buffer_(NULL),
g_rgb_frame_data_(NULL),
g_pixel_format_(GX_PIXEL_FORMAT_BAYER_GR8),
g_color_filter_(GX_COLOR_FILTER_NONE),
g_acquire_thread_(0),
g_get_image_(false),
g_frameinfo_data_(NULL),
g_frameinfo_datasize_(0),
triger_line_(2),
triger_edge_(1),
file_dir_("../images/test/")
{
    mutex_ = new pthread_mutex_t;
    pthread_mutex_init(mutex_, NULL);
    unsolved_list_ = new std::queue<std::string>;
}

Camera::Camera(pthread_mutex_t* mutex, std::queue<std::string>* unsolved_list):
cam_id_(1),
fake_ptr_(0),
count_(1),
g_device_(NULL),
g_frame_data_({0}),
g_raw8_buffer_(NULL),
g_rgb_frame_data_(NULL),
g_pixel_format_(GX_PIXEL_FORMAT_BAYER_GR8),
g_color_filter_(GX_COLOR_FILTER_NONE),
g_acquire_thread_(0),
g_get_image_(false),
g_frameinfo_data_(NULL),
g_frameinfo_datasize_(0),
triger_line_(2),
triger_edge_(1),
file_dir_("../images/test/")
{
    mutex_ = mutex;
    unsolved_list_ = unsolved_list;
}

Camera::Camera( pthread_mutex_t* mutex, 
                std::queue<std::string>* unsolved_list, 
                std::deque<int64_t>* work_count_list,
                std::vector<std::vector<ROI>>* batch_ROI_list,
                int64_t pixel_format
                ):
cam_id_(1),
fake_ptr_(0),
count_(1),
g_device_(NULL),
g_frame_data_({0}),
g_raw8_buffer_(NULL),
g_rgb_frame_data_(NULL),
g_pixel_format_(GX_PIXEL_FORMAT_BAYER_GR8),
g_color_filter_(GX_COLOR_FILTER_NONE),
g_acquire_thread_(0),
g_get_image_(false),
g_frameinfo_data_(NULL),
g_frameinfo_datasize_(0),
triger_line_(2),
triger_edge_(1),
file_dir_("../images/test/")
{
    mutex_ = mutex;
    unsolved_list_ = unsolved_list;
    work_count_list_ = work_count_list;
    batch_ROI_list_ = batch_ROI_list;
}

Camera::Camera( pthread_mutex_t* mutex, 
                std::queue<std::string>* unsolved_list, 
                std::deque<std::string>* work_name_list, 
                std::deque<int64_t>* work_count_list,
                int64_t pixel_format
                ):
cam_id_(1),
fake_ptr_(0),
count_(1),
g_device_(NULL),
g_frame_data_({0}),
g_raw8_buffer_(NULL),
g_rgb_frame_data_(NULL),
g_pixel_format_(pixel_format),
g_color_filter_(GX_COLOR_FILTER_BAYER_RG),
g_acquire_thread_(0),
g_get_image_(false),
g_frameinfo_data_(NULL),
g_frameinfo_datasize_(0),
triger_line_(2),
triger_edge_(1),
file_dir_("../images/test/")
{
    mutex_ = mutex;
    unsolved_list_ = unsolved_list;
    work_name_list_ = work_name_list;
    work_count_list_ = work_count_list;
}

Camera::~Camera()
{
    // stop();
}

//-------------------------------------------------
/**
\brief 获取图像大小并申请图像数据空间
\return void
*/
//-------------------------------------------------
int Camera::PreForImage(){
    GX_STATUS status = GX_STATUS_SUCCESS;

    //为获取的图像分配存储空间
    int64_t payload_size = 0;
    status = GXGetInt(g_device_, GX_INT_PAYLOAD_SIZE, &payload_size); 
    if(status != GX_STATUS_SUCCESS)
    {
        GetErrorString(status);
        return status;
    }

    g_frame_data_.pImgBuf = malloc(payload_size);
    if (g_frame_data_.pImgBuf == NULL)
    {
        printf("<Failed to allocate memory>\n");
        return MEMORY_ALLOT_ERROR;
    }

    //将非8位raw数据转换成8位数据的时候的中转缓冲buffer
    g_raw8_buffer_ = malloc(payload_size);
    if (g_raw8_buffer_ == NULL)
    {
        printf("<Failed to allocate memory>\n");
        return MEMORY_ALLOT_ERROR;
    }

    //RGB数据是RAW数据的3倍大小
    g_rgb_frame_data_ = malloc(payload_size * 3);
    if (g_rgb_frame_data_ == NULL)
    {
        printf("<Failed to allocate memory>\n");
        return MEMORY_ALLOT_ERROR;
    }
    
    //获取帧信息长度并申请帧信息数据空间
    status = GXGetBufferLength(g_device_, GX_BUFFER_FRAME_INFORMATION, &g_frameinfo_datasize_);
    if(status != GX_STATUS_SUCCESS)
    { 
        //用户根据实际情况自行处理
    }
    
    if(g_frameinfo_datasize_ > 0)
    {    
        g_frameinfo_data_ = malloc(g_frameinfo_datasize_);
        if(g_frameinfo_data_ == NULL)
        {
            printf("<Failed to allocate memory>\n");
            return MEMORY_ALLOT_ERROR;
        }
    }

    return 0;
}

//-------------------------------------------------
/**
\brief 释放资源
\return void
*/
//-------------------------------------------------
int Camera::UnPreForImage(){
    GX_STATUS status = GX_STATUS_SUCCESS;
    uint32_t ret = 0;
   
    //发送停采命令
    status = GXSendCommand(g_device_, GX_COMMAND_ACQUISITION_STOP);
    if(status != GX_STATUS_SUCCESS)
    {
        GetErrorString(status);
        return status;
    }

    g_get_image_ = false;
    // ret = pthread_join(g_acquire_thread_, NULL);
    ret = pthread_cancel(g_acquire_thread_);
    if(ret != 0)
    {
        printf("<Failed to release resources>\n");
        return ret;
    }
	

    //释放buffer
    if(g_frame_data_.pImgBuf != NULL)
    {
        free(g_frame_data_.pImgBuf);
        g_frame_data_.pImgBuf = NULL;
    }

    return 0;
}

//-------------------------------------------------
/**
\brief 获取当前帧号
\return 当前帧号
*/
//-------------------------------------------------
int Camera::GetCurFrameIndex(){
    GX_STATUS status = GX_STATUS_SUCCESS;
    status = GXGetBuffer(g_device_, GX_BUFFER_FRAME_INFORMATION, (uint8_t*)g_frameinfo_data_, &g_frameinfo_datasize_);
    if(status != GX_STATUS_SUCCESS)
    {
        //用户可自行处理
    }

    int current_index = 0;
    memcpy(&current_index, (uint8_t*)g_frameinfo_data_+FRAMEINFOOFFSET, sizeof(int));

    return current_index;
}


//-------------------------------------------------
/**
\brief 采集线程入口函数
\param 
\return void
*/
//-------------------------------------------------
void* Camera::ProcGetImage_pth(void* arg){
   Camera *_this = (Camera *)arg;
   _this->ProcGetImage();
}

//-------------------------------------------------
/**
\brief 采集线程函数
\param 
\return void
*/
//-------------------------------------------------
void Camera::ProcGetImage(){
    GX_STATUS status = GX_STATUS_SUCCESS;
    bool is_implemented = false;
    //接收线程启动标志
    g_get_image_ = true;

    while(g_get_image_)
    {
        status = GXGetImage(g_device_, &g_frame_data_, 100);
        
        if(status == GX_STATUS_SUCCESS)
        {
            if(g_frame_data_.nStatus == 0)
            {
                
                g_time_counter_.Begin();
                printf("<Successful acquisition: Width: %d Height: %d>\n", g_frame_data_.nWidth, g_frame_data_.nHeight);
                status = GXIsImplemented(g_device_, GX_BUFFER_FRAME_INFORMATION, &is_implemented);
                if(status == GX_STATUS_SUCCESS)
                {
                    if(true == is_implemented)
                    {
                        printf("<Frame number: %d>\n", GetCurFrameIndex());
                    }
                }
                //保存Raw数据
                // SaveRawFile(g_frame_data_.pImgBuf, g_frame_data_.nWidth, g_frame_data_.nHeight);
                status  = DxAutoRawDefectivePixelCorrect(g_frame_data_.pImgBuf,g_frame_data_.nWidth, g_frame_data_.nHeight, 8);
                if (status != DX_OK)
                {
                    GetErrorString(status);
                }
                //将Raw数据处理成RGB数据
                ProcessData(g_frame_data_.pImgBuf, 
                        g_raw8_buffer_, 
                        g_rgb_frame_data_, 
                        g_frame_data_.nWidth, 
                        g_frame_data_.nHeight,
                        g_pixel_format_,
                        g_color_filter_); 
                if(ROIs_.size()>0){
                    SavePPMwithROIs(g_rgb_frame_data_, g_frame_data_.nWidth, g_frame_data_.nHeight, ROIs_);
                }else{
                    //保存RGB数据
                    SavePPMFile(g_rgb_frame_data_, g_frame_data_.nWidth, g_frame_data_.nHeight);
                }
                // cv::Mat image(g_frame_data_.nHeight, g_frame_data_.nWidth, CV_8UC3, (uchar*)g_rgb_frame_data_);
                // cv::cvtColor(image,image,cv::COLOR_RGB2BGR);
                // cv::namedWindow("camera original image", CV_WINDOW_NORMAL);
                // cv::imshow("camera original image",image);
                // cv::waitKey();

                printf("time of process %ld us\n", g_time_counter_.End());
            // }else{
            //     SaveMono(g_frame_data_.pImgBuf, g_frame_data_.nWidth, g_frame_data_.nHeight);
            //     printf("time of process %ld us\n", g_time_counter_.End());
            }
        }
    }
}

//-------------------------------------------------
/**
\brief 相机初始化函数
\param [in] cam_id 相机编号默认为1
\return int
*/
//------------------------------------------------- 
int Camera::init(){
    GX_STATUS status = GX_STATUS_SUCCESS;
    int ret = 0;
    GX_OPEN_PARAM open_param;

    //初始化设备打开参数，默认打开序号为１的设备
    char cam_id_string[10];
    sprintf(cam_id_string,"%d",cam_id_);
    open_param.accessMode = GX_ACCESS_EXCLUSIVE;
    open_param.openMode = GX_OPEN_INDEX;
    open_param.pszContent = cam_id_string;       
    printf("<choose device %s>\n", open_param.pszContent);

    //初始化库
    status = GXInitLib();
    if(status != GX_STATUS_SUCCESS)
    {
        GetErrorString(status);
        return 0;
    }

    //枚举设备个数
    uint32_t device_number = 0;
    status = GXUpdateDeviceList(&device_number, 1000);
    if(status != GX_STATUS_SUCCESS)
    {
        GetErrorString(status);
        return 0;
    }

    if(device_number <= 0)
    {
        printf("<No device>\n");
        return 0;
    }
    else
    {
        //默认打开第1个设备
        status = GXOpenDevice(&open_param, &g_device_);
        if(status == GX_STATUS_SUCCESS)
        {
            printf("<Open device success>\n");
        }
        else
        {
            printf("<Open devide fail>\n");
            return 0;			
        }
    }

    //设置采集模式为连续采集
    status = GXSetEnum(g_device_, GX_ENUM_ACQUISITION_MODE, GX_ACQ_MODE_CONTINUOUS);
    if(status != GX_STATUS_SUCCESS)
    {
        GetErrorString(status);
        status = GXCloseDevice(g_device_);
        if(g_device_ != NULL)
        {
            g_device_ = NULL;
        }
        status = GXCloseLib();
        return 0;
    }

    //获取相机输出数据的颜色格式
    status = GXGetEnum(g_device_, GX_ENUM_PIXEL_FORMAT, &g_pixel_format_);
    if(status != GX_STATUS_SUCCESS)
    {
        GetErrorString(status);
    }
  
    //相机采集图像为彩色还是黑白
    status = GXGetEnum(g_device_, GX_ENUM_PIXEL_COLOR_FILTER, &g_color_filter_);
    if(status != GX_STATUS_SUCCESS)
    {
        GetErrorString(status);
    }

    // status=GXSetEnum(g_device_, GX_ENUM_DEAD_PIXEL_CORRECT, GX_DEAD_PIXEL_CORRECT_ON);
    // if(status != GX_STATUS_SUCCESS)
    // {
    //     GetErrorString(status);
    // }

    //为采集做准备
    ret = PreForImage();
    if(ret != 0)
    {
        printf("<Failed to prepare for acquire image>\n");
        status = GXCloseDevice(g_device_);
        if(g_device_ != NULL)
        {
            g_device_ = NULL;
        }
        status = GXCloseLib();
        return 0;
    }
}

//-------------------------------------------------
/**
\brief 相机开采并开启线程
\param 
\return int
*/
//------------------------------------------------- 
int Camera::start(){
    //发送开采命令
    GX_STATUS status = GXSendCommand(g_device_, GX_COMMAND_ACQUISITION_START);
    if(status != GX_STATUS_SUCCESS)
    {
        GetErrorString(status);
        status = GXCloseDevice(g_device_);
        if(g_device_ != NULL)
        {
            g_device_ = NULL;
        }
        status = GXCloseLib();
        return -1;
    }
    
    int ret = pthread_create(&g_acquire_thread_, NULL, ProcGetImage_pth, this);
    if(ret != 0)
    {
        printf("<Failed to create the collection thread>\n");
        return -1;
    }
    pthread_detach(g_acquire_thread_);
    return 0;
}

//-------------------------------------------------
/**
\brief 停止相机
\param 
\return int 
*/
//-------------------------------------------------
int Camera::stop(){
    //为停止采集做准备
    GX_STATUS status = GX_STATUS_SUCCESS;
    int ret = UnPreForImage();
    if(ret != 0)
    {
        status = GXCloseDevice(g_device_);
        if(g_device_ != NULL)
        {
            g_device_ = NULL;
        }
        status = GXCloseLib(); 
        return 0;
    }

    //关闭设备
    printf("<close device>\n");
    status = GXCloseDevice(g_device_);
    if(status != GX_STATUS_SUCCESS)
    {    
        return 0;
    }

    //释放库
    status = GXCloseLib();
    return status;
}

//-------------------------------------------------
/**
\brief 保存内存数据到ppm格式文件中
\param image_buffer RAW数据经过插值换算出的RGB数据
\param width 图像宽
\param height 图像高
\return void
*/ 
//-------------------------------------------------
void Camera::SavePPMFile(void *image_buffer, size_t width, size_t height){
    char name[64] = {0};
    char filename[64];
    FILE* ff = NULL;
    std::string work_name;
    bool push_unsolved_list = true;
    printf("[===DEBUG===] fake_ptr is: %d, size: %d\n", fake_ptr_, work_name_list_->size());
    if(!work_name_list_->empty() && fake_ptr_ < work_name_list_->size()){
        work_name = work_name_list_->at(fake_ptr_);
    }else{
        // 等detection pop掉
        printf("[WARN] List out of range!!! ptr:%d, size:%d\n",fake_ptr_,work_name_list_->size());
        work_name = "unlisted_";
        push_unsolved_list = false;
    }
    if(work_name.front() == 'r')
        // 重打
        sprintf(name, "%s.ppm", work_name.c_str());
    else
        sprintf(name, "%s_%ld.ppm", work_name.c_str(), count_);
    sprintf(filename,"%s%s",file_dir_.data(),name);
    ff=fopen(filename,"wb");
    if(ff != NULL)
    {
        fprintf(ff, "P6\n" "%zu %zu\n255\n", width, height);
        fwrite(image_buffer, 1, width * height * 3, ff);
        fclose(ff);
        ff = NULL;
        if(push_unsolved_list){
            pthread_mutex_lock(mutex_);
            // test
            // unsolved_list_->push("print_1_0_test.ppm");
            unsolved_list_->push(name);
            pthread_mutex_unlock(mutex_);
        }
        printf("<Save %s success>\n", name);
    }
    if(!work_count_list_->empty() && fake_ptr_ < work_count_list_->size()){
        count_++;
        if(count_ > work_count_list_->at(fake_ptr_)){
            if(!work_count_list_->empty()){
                fake_ptr_++;
                count_ = 1;
            }else{
                fake_ptr_ = 0;
                printf("[WARN] work end!");
            }
        }
    }
}


void Camera::SaveMono(void *image_buffer, size_t width, size_t height){
    char name[64] = {0};
    char filename[64];
    FILE* ff = NULL;
    std::string work_name;
    bool push_unsolved_list = true;
    printf("[===DEBUG===] fake_ptr is: %d, size: %d\n", fake_ptr_, work_name_list_->size());
    if(!work_name_list_->empty() && fake_ptr_ < work_name_list_->size()){
        work_name = work_name_list_->at(fake_ptr_);
    }else{
        // 等detection pop掉
        printf("[WARN] List out of range!!! ptr:%d, size:%d\n",fake_ptr_,work_name_list_->size());
        work_name = "unlisted_";
        push_unsolved_list = false;
    }
    if(work_name.front() == 'r')
        // 重打
        sprintf(name, "%s.ppm", work_name.c_str());
    else
        sprintf(name, "%s_%ld.ppm", work_name.c_str(), count_);
    sprintf(filename,"%s%s",file_dir_.data(),name);
    ff=fopen(filename,"wb");
    if(ff != NULL)
    {
        fprintf(ff, "P6\n" "%zu %zu\n255\n", width, height);
        fwrite(image_buffer, 1, width * height, ff);
        fclose(ff);
        ff = NULL;
        if(push_unsolved_list){
            pthread_mutex_lock(mutex_);
            // test
            // unsolved_list_->push("print_1_0_test.ppm");
            unsolved_list_->push(name);
            pthread_mutex_unlock(mutex_);
        }
        printf("<Save %s success>\n", name);
    }
    if(!work_count_list_->empty() && fake_ptr_ < work_count_list_->size()){
        count_++;
        if(count_ > work_count_list_->at(fake_ptr_)){
            if(!work_count_list_->empty()){
                fake_ptr_++;
                count_ = 1;
            }else{
                fake_ptr_ = 0;
                printf("[WARN] work end!");
            }
        }
    }
}


//-------------------------------------------------
/**
\brief 保存ROI内的内存数据到ppm格式文件中
\param image_buffer RAW数据经过插值换算出的RGB数据
\param width 图像宽
\param height 图像高
\param ROIs 各ROI
\return void
*/ 
//-------------------------------------------------
void Camera::SavePPMwithROIs(void *image_buffer, size_t width, size_t height, std::vector<ROI> ROIs){
    char name[64] = {0};
    char filename[64];
    // static int rgb_file_index = 1;
    FILE* ff = NULL;
    
    if(ROIs.empty()){
        SavePPMFile(image_buffer, width, height);
        return;
    }
    void* ROI_buffer = image_buffer;
    for(size_t i = 0;i<ROIs.size();i++){
        sprintf(name, "%s_%ld.ppm", work_name_list_->at(fake_ptr_).c_str(), count_);
        sprintf(filename,"%s%s",file_dir_.data(),name);
        printf("filename:%s\n",filename);
        ff=fopen(filename,"wb");
        if(ff != NULL)
        {
            fprintf(ff, "P6\n" "%d %d\n255\n", ROIs[i].w, ROIs[i].h);
            ROI_buffer = image_buffer + (ROIs[i].y * width + ROIs[i].x) * 3;
            printf("x,y %d\n",(ROIs[i].y * ROIs[i].w + ROIs[i].x) * 3);
            for(size_t j = 0; j < ROIs[i].h; j++){
                ROI_buffer = ROI_buffer + width * 3;
                fwrite(ROI_buffer, 1, ROIs[i].w * 3, ff);
            }
            fclose(ff);
            ff = NULL;
            pthread_mutex_lock(mutex_);
            unsolved_list_->push(name);
            pthread_mutex_unlock(mutex_);
            printf("<Save with ROI %s success>\n", name);
        }
        count_++;
        if(count_ > work_count_list_->at(fake_ptr_)){
            if(!work_name_list_->empty()){
                fake_ptr_++;
                count_ = 1;
            }
            if(batch_ROI_iter_ != batch_ROI_list_->end()){
                batch_ROI_iter_++;
                setROI(*batch_ROI_iter_);
            }else{
                printf("[WARN] the batch is end\n");
            }
            break;
        }
    }
}

//-------------------------------------------------
/**
\brief 发送软触发信号
\param 

\return int 
*/
//-------------------------------------------------
int Camera::sendSoftTrigger(){
    GX_STATUS status = GXSendCommand(g_device_, GX_COMMAND_TRIGGER_SOFTWARE);
    //printf("<The return value of softtrigger command: %d>\n", status);
    return status;
}

//-------------------------------------------------
/**
\brief 设置触发方式
\param [in] type 触发类型 0软触发 1外触发

\return int 
*/
//-------------------------------------------------
int Camera::setTrigger(int type){
    //设置触发开关为ON
    GX_STATUS status = GXSetEnum(g_device_, GX_ENUM_TRIGGER_MODE, GX_TRIGGER_MODE_ON);
    if(status != GX_STATUS_SUCCESS)
    {
        GetErrorString(status);
        status = GXCloseDevice(g_device_);
        if(g_device_ != NULL)
        {
            g_device_ = NULL;
        }
        status = GXCloseLib();
        return 0;
    }

    switch(type){
        case SOFT_TRIGGER:
            //设置触发源为软触发
            status = GXSetEnum(g_device_, GX_ENUM_TRIGGER_SOURCE, GX_TRIGGER_SOURCE_SOFTWARE);
            if(status != GX_STATUS_SUCCESS)
            {
                GetErrorString(status);
                status = GXCloseDevice(g_device_);
                if(g_device_ != NULL)
                {
                    g_device_ = NULL;
                }
                status = GXCloseLib();
                return 0;
            }
            break;
        case HARD_TRIGGER:
            printf("set triger: hardware ");
            //设置触发源为外触发
            switch(triger_line_){
                case TRIGER_LINE0:
                    status = GXSetEnum(g_device_, GX_ENUM_TRIGGER_SOURCE, GX_TRIGGER_SOURCE_LINE0);
                    printf("line0");
                    break;
                case TRIGER_LINE1:
                    status = GXSetEnum(g_device_, GX_ENUM_TRIGGER_SOURCE, GX_TRIGGER_SOURCE_LINE1);
                    printf("line1");
                    break;
                case TRIGER_LINE2:
                    status = GXSetEnum(g_device_, GX_ENUM_TRIGGER_SOURCE, GX_TRIGGER_SOURCE_LINE2);
                    printf("line2");
                    break;
                case TRIGER_LINE3:
                    status = GXSetEnum(g_device_, GX_ENUM_TRIGGER_SOURCE, GX_TRIGGER_SOURCE_LINE3);
                    printf("line3");
                    break;
            }
            if(status != GX_STATUS_SUCCESS)
            {
                GetErrorString(status);
                status = GXCloseDevice(g_device_);
                if(g_device_ != NULL)
                {
                    g_device_ = NULL;
                }
                status = GXCloseLib();
                return 0;
            }
            switch(triger_edge_){
                case TRIGER_FALLING:
                    //设置触发激活方式为下降沿
                    status = GXSetEnum(g_device_, GX_ENUM_TRIGGER_ACTIVATION, GX_TRIGGER_ACTIVATION_FALLINGEDGE);
                    status = GXSetFloat(g_device_, GX_FLOAT_TRIGGER_FILTER_FALLING, 0);
                    printf(" fall");
                    break;
                case TRIGER_RISING:
                    //设置触发激活方式为上升沿
                    status = GXSetEnum(g_device_, GX_ENUM_TRIGGER_ACTIVATION, GX_TRIGGER_ACTIVATION_RISINGEDGE);
                    status = GXSetFloat(g_device_, GX_FLOAT_TRIGGER_FILTER_RAISING, 0);
                    printf(" rise");
                    break;
            }
            printf("\n");
            if(status != GX_STATUS_SUCCESS)
            {
                GetErrorString(status);
                status = GXCloseDevice(g_device_);
                if(g_device_ != NULL)
                {
                    g_device_ = NULL;
                }
                status = GXCloseLib();
                return 0;
            }
            break;
    }

}

//-------------------------------------------------
/**
\brief 设置曝光
\param [in] shutterTime 曝光时间
\return int 
*/
//-------------------------------------------------
int Camera::setShutter(double shutterTime){
    //设置曝光
    GX_FLOAT_RANGE shutterRange;
    GX_STATUS status = GXGetFloatRange(g_device_, GX_FLOAT_EXPOSURE_TIME, &shutterRange);
    if(status != GX_STATUS_SUCCESS)
    {
        GetErrorString(status);
    }
    // 限幅
    limitDoubleValue(shutterTime,shutterRange);
    printf("exposure time %f, range %f, %f \n", shutterTime, shutterRange.dMin, shutterRange.dMax);
    status = GXSetFloat(g_device_, GX_FLOAT_EXPOSURE_TIME, shutterTime);
    if(status != GX_STATUS_SUCCESS)
    {
        GetErrorString(status);
    }
}

//-------------------------------------------------
/**
\brief 设置白平衡
\param [in] ratio 值
\param [in] channel 通道 1R 2G 3B
\return int 
*/
//-------------------------------------------------
int Camera::setBalance(double ratio, int channel){
    GX_STATUS status = GX_STATUS_SUCCESS;
    GX_FLOAT_RANGE ratioRange;
    switch(channel){
        case CAM_RED_BALANCE_CHANNEL:
            //选择R白平衡通道
            status = GXSetEnum(g_device_, GX_ENUM_BALANCE_RATIO_SELECTOR, GX_BALANCE_RATIO_SELECTOR_RED);
            if(status != GX_STATUS_SUCCESS)
            {
                GetErrorString(status);
            }
            //获取白平衡调节范围
            status = GXGetFloatRange(g_device_, GX_FLOAT_BALANCE_RATIO, &ratioRange);
            if(status != GX_STATUS_SUCCESS)
            {
                GetErrorString(status);
            }
            //设置R白平衡系数
            printf("set red balance ratio %f, range %f ,%f\n",ratio, ratioRange.dMin, ratioRange.dMax);
            break;
        case CAM_GREEN_BALANCE_CHANNEL:
            //选择G白平衡通道
            status = GXSetEnum(g_device_, GX_ENUM_BALANCE_RATIO_SELECTOR, GX_BALANCE_RATIO_SELECTOR_GREEN);
            if(status != GX_STATUS_SUCCESS)
            {
                GetErrorString(status);
            }
            //获取白平衡调节范围
            status = GXGetFloatRange(g_device_, GX_FLOAT_BALANCE_RATIO, &ratioRange);
            if(status != GX_STATUS_SUCCESS)
            {
                GetErrorString(status);
            }
            printf("set green balance ratio %f, range %f ,%f\n",ratio, ratioRange.dMin, ratioRange.dMax);
            break;

        case CAM_BLUE_BALANCE_CHANNEL:
            //选择B白平衡通道
            status = GXSetEnum(g_device_, GX_ENUM_BALANCE_RATIO_SELECTOR, GX_BALANCE_RATIO_SELECTOR_BLUE);
            if(status != GX_STATUS_SUCCESS)
            {
                GetErrorString(status);
            }
            //获取白平衡调节范围
            status = GXGetFloatRange(g_device_, GX_FLOAT_BALANCE_RATIO, &ratioRange);
            if(status != GX_STATUS_SUCCESS)
            {
                GetErrorString(status);
            }
            //设置B白平衡系数
            printf("set blue balance ratio %f, range %f ,%f\n",ratio, ratioRange.dMin, ratioRange.dMax);
            break;
    }
    // 设置白平衡系数
    limitDoubleValue(ratio,ratioRange);
    status = GXSetFloat(g_device_, GX_FLOAT_BALANCE_RATIO, ratio);
    if(status != GX_STATUS_SUCCESS)
    {
        GetErrorString(status);
    }
    return status;
}

//-------------------------------------------------
/**
\brief 设置增益
\param [in] gain_value 值
\return int 
*/
//-------------------------------------------------
int Camera::setGain(double gain_value){
    //选择增益通道类型
    GX_STATUS status = GXSetEnum(g_device_, GX_ENUM_GAIN_SELECTOR, GX_GAIN_SELECTOR_ALL);
    if(status != GX_STATUS_SUCCESS)
    {
        GetErrorString(status);
    }
    //获取增益调节范围
    GX_FLOAT_RANGE gainRange;
    status = GXGetFloatRange(g_device_, GX_FLOAT_GAIN, &gainRange);
    if(status != GX_STATUS_SUCCESS)
    {
        GetErrorString(status);
    }
    limitDoubleValue(gain_value, gainRange);
    //设置增益值
    status = GXSetFloat(g_device_, GX_FLOAT_GAIN, gain_value);
    printf("set gain %f, range %f ,%f\n",gain_value, gainRange.dMin, gainRange.dMax);
    return status;
}

//-------------------------------------------------
/**
\brief 设置ROI
\param [in] rois 
\return void 
*/
//-------------------------------------------------
void Camera::setROI(std::vector<ROI> rois){
    ROIs_ = rois;
    count_ = 0;
}

//-------------------------------------------------
/**
\brief 设置参数
\param [in] 
\return void 
*/
//-------------------------------------------------
int Camera::applyParam(){
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
            shutter_time_  = root["camera"]["shutter time"].empty()        ?  shutter_time_  : root["camera"]["shutter time"].asDouble();
            red_balance_   = root["camera"]["Red Balance ratio"].empty()   ?  red_balance_   : root["camera"]["Red Balance ratio"].asDouble();
            green_balance_ = root["camera"]["Green Balance ratio"].empty() ?  green_balance_ : root["camera"]["Green Balance ratio"].asDouble();
            blue_balance_  = root["camera"]["Blue Balance ratio"].empty()  ?  blue_balance_  : root["camera"]["Blue Balance ratio"].asDouble();
            gain_value_    = root["camera"]["Gain"].empty()                ?  gain_value_    : root["camera"]["Gain"].asDouble();
            cam_id_        = root["camera"]["id"].empty()                  ?  cam_id_        : root["camera"]["id"].asInt();
            triger_line_   = root["camera"]["trigger"]["source"].empty()    ?  triger_line_   : root["camera"]["trigger"]["source"].asInt();
            triger_edge_   = root["camera"]["trigger"]["edge"].empty()      ?  triger_edge_   : root["camera"]["trigger"]["edge"].asInt();
            file_dir_      = root["detection"]["file"]["image directory"].empty() ? file_dir_ : root["detection"]["file"]["image directory"].asString();
        }else{
            std::cout << "[ERROR] Jsoncpp error: " << errs << std::endl;
        }
        paramfile.close();
    }

    // 设置曝光
    int ret = setShutter(shutter_time_);
    if(ret != 0) printf("[WARN] failed to set shutter");

    // 设置白平衡
    ret = setBalance(red_balance_, CAM_RED_BALANCE_CHANNEL);
    if(ret != 0) printf("[WARN] failed to set red balance");
    ret = setBalance(green_balance_, CAM_GREEN_BALANCE_CHANNEL);
    if(ret != 0) printf("[WARN] failed to set green balance");
    ret = setBalance(blue_balance_, CAM_BLUE_BALANCE_CHANNEL);
    if(ret != 0) printf("[WARN] failed to set blue balance");

    // 设置增益
    ret = setGain(gain_value_);
    if(ret != 0) printf("[WARN] failed to set gain");

    printf("set trigger line: %d\n", triger_line_);
    printf("set trigger edge: %d\n", triger_edge_);
    return 0;
}

int64_t Camera::getCount(){
    return count_;
}

int Camera::popList(){
    if(fake_ptr_ > 0){
        fake_ptr_ -= 1;
        printf("============================================================================= pop %d ============================================================================\n",fake_ptr_);
        return 0;
    }else{
        return -1;
    }
}