#include "Camera.h"

Camera::Camera():
g_device_(NULL),
g_frame_data_({0}),
g_raw8_buffer_(NULL),
g_rgb_frame_data_(NULL),
g_pixel_format_(GX_PIXEL_FORMAT_BAYER_GR8),
g_color_filter_(GX_COLOR_FILTER_NONE),
g_acquire_thread_(0),
g_get_image_(false),
g_frameinfo_data_(NULL),
g_frameinfo_datasize_(0)
{
}


Camera::Camera(GX_DEV_HANDLE device):
g_device_(NULL),
g_frame_data_({0}),
g_raw8_buffer_(NULL),
g_rgb_frame_data_(NULL),
g_pixel_format_(GX_PIXEL_FORMAT_BAYER_GR8),
g_color_filter_(GX_COLOR_FILTER_NONE),
g_acquire_thread_(0),
g_get_image_(false),
g_frameinfo_data_(NULL),
g_frameinfo_datasize_(0)
{
    g_device_ = device;
}


Camera::~Camera()
{
    stop();
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
    ret = pthread_join(g_acquire_thread_,NULL);
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
                //SaveRawFile(g_frame_data_.pImgBuf, g_frame_data_.nWidth, g_frame_data_.nHeight);

                //将Raw数据处理成RGB数据
                ProcessData(g_frame_data_.pImgBuf, 
                        g_raw8_buffer_, 
                        g_rgb_frame_data_, 
                        g_frame_data_.nWidth, 
                        g_frame_data_.nHeight,
                        g_pixel_format_,
                        g_color_filter_); 
                
                if(ROIs_.size()>0){
                    SavePPMwithROIs(g_rgb_frame_data_, g_frame_data_.nWidth, g_frame_data_.nHeight,ROIs_);
                }else{
                    //保存RGB数据
                    SavePPMFile(g_rgb_frame_data_, g_frame_data_.nWidth, g_frame_data_.nHeight);
                }

                
                
                printf("time of process %ld us\n", g_time_counter_.End());
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
int Camera::init(int cam_id){
    GX_STATUS status = GX_STATUS_SUCCESS;
    int ret = 0;
    GX_OPEN_PARAM open_param;

    //初始化设备打开参数，默认打开序号为１的设备
    char cam_id_string[10];
    sprintf(cam_id_string,"%d",cam_id);
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
        case 0:
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
        case 1:
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
    printf("set blue balance ratio %f, range %f ,%f\n",gain_value, gainRange.dMin, gainRange.dMax);
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
}