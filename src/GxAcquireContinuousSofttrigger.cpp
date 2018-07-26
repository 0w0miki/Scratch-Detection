//-------------------------------------------------------------
/**
\file      GxAcquireContinuousSofttrigger.cpp
\brief     sample to show how to acquire image by softtrigger.
\version   1.0.1605.9041
\date      2016-05-04
*/
//--------------------------------------------------------------
#include "GxIAPI.h"
#include "DxImageProc.h"
#include "CTimeCounter.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "CameraUtils.h"

#define FRAMEINFOOFFSET 14
#define MEMORY_ALLOT_ERROR -1

GX_DEV_HANDLE g_device = NULL;                                    ///< 设备句柄
GX_FRAME_DATA g_frame_data = { 0 };                               ///< 采集图像参数
void *g_raw8_buffer = NULL;                                       ///< 将非8位raw数据转换成8位数据的时候的中转缓冲buffer
void *g_rgb_frame_data = NULL;                                    ///< RAW数据转换成RGB数据后的存储空间，大小是相机输出数据大小的3倍
int64_t g_pixel_format = GX_PIXEL_FORMAT_BAYER_GR8;               ///< 当前相机的pixelformat格式
int64_t g_color_filter = GX_COLOR_FILTER_NONE;                    ///< bayer插值的参数
pthread_t g_acquire_thread = 0;                                   ///< 采集线程ID
bool g_get_image = false;                                         ///< 采集线程是否结束的标志：true 运行；false 退出
void *g_frameinfo_data = NULL;                                    ///< 帧信息数据缓冲区
size_t g_frameinfo_datasize = 0;                                  ///< 帧信息数据长度

CTimeCounter g_time_counter;                                      ///< 计时器

//获取图像大小并申请图像数据空间
int PreForImage();

//释放资源
int UnPreForImage();

//采集线程函数
void *ProcGetImage(void* param);

//将相机输出的原始数据转换为RGB数据
void ProcessData(void *image_buffer, void *image_raw8_buffer, void *image_rgb_buffer, int image_width, int image_height, int pixel_format, int color_filter);

//获取当前帧号
int GetCurFrameIndex();

int main()
{
    uid_t user = 0;
    user = geteuid();
    if(user != 0)
    {
        printf("\n");
        printf("Please run this application with 'sudo -E ./GxAcquireContinuousSofttrigger' or"
                              " Start with root !\n");
        printf("\n");
        return 0;
    }

    printf("\n");
    printf("-------------------------------------------------------------\n");
    printf("sample to show how to acquire image by softtrigger.\n");
    #ifdef __x86_64__
    printf("version: 1.0.1605.8041\n");
    #elif __i386__
    printf("version: 1.0.1605.9041\n");
    #endif
    printf("-------------------------------------------------------------\n");
    printf("\n");
 
    GX_STATUS status = GX_STATUS_SUCCESS;
    int ret = 0;
    GX_OPEN_PARAM open_param;

    //初始化设备打开参数，默认打开序号为１的设备
    int cam_id = 1;
    char cam_id_string[10];
    sprintf(cam_id_string,"%d",cam_id);
    open_param.accessMode = GX_ACCESS_EXCLUSIVE;
    open_param.openMode = GX_OPEN_INDEX;
    // open_param.pszContent = "1";
    open_param.pszContent = cam_id_string;
    printf("camera id %s", open_param.pszContent);
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
        status = GXOpenDevice(&open_param, &g_device);
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
    status = GXSetEnum(g_device, GX_ENUM_ACQUISITION_MODE, GX_ACQ_MODE_CONTINUOUS);
    if(status != GX_STATUS_SUCCESS)
    {
        GetErrorString(status);
        status = GXCloseDevice(g_device);
        if(g_device != NULL)
        {
            g_device = NULL;
        }
        status = GXCloseLib();
        return 0;
    }

    //设置触发开关为ON
    status = GXSetEnum(g_device, GX_ENUM_TRIGGER_MODE, GX_TRIGGER_MODE_ON);
    if(status != GX_STATUS_SUCCESS)
    {
        GetErrorString(status);
        status = GXCloseDevice(g_device);
        if(g_device != NULL)
        {
            g_device = NULL;
        }
        status = GXCloseLib();
        return 0;
    }
	

    //设置触发源为软触发
    status = GXSetEnum(g_device, GX_ENUM_TRIGGER_SOURCE, GX_TRIGGER_SOURCE_SOFTWARE);
    if(status != GX_STATUS_SUCCESS)
    {
        GetErrorString(status);
        status = GXCloseDevice(g_device);
        if(g_device != NULL)
        {
            g_device = NULL;
        }
        status = GXCloseLib();
        return 0;
    }
	
    //设置触发源为外触发lin0
    /*
        status = GXSetEnum(g_device, GX_ENUM_TRIGGER_SOURCE, GX_TRIGGER_SOURCE_LINE0);
        if(status != GX_STATUS_SUCCESS)
        {
            GetErrorString(status);
            status = GXCloseDevice(g_device);
            if(g_device != NULL)
            {
                g_device = NULL;
            }
            status = GXCloseLib();
            return 0;
        }

        //设置触发激活方式为上升沿
        status = GXSetEnum(g_device, GX_ENUM_TRIGGER_ACTIVATION, GX_TRIGGER_ACTIVATION_RISINGEDGE);
        //滤波
        //获取上升沿滤波设置范围
        GX_FLOAT_RANGE raisingRange;
        status = GXGetFloatRange(g_device, GX_FLOAT_TRIGGER_FILTER_RAISING, &raisingRange);
        if(status != GX_STATUS_SUCCESS)
        {
            GetErrorString(status);
        }

        //设置上升沿滤波最小值
        status = GXSetFloat(g_device, GX_FLOAT_TRIGGER_FILTER_RAISING, raisingRange.dMin);
        if(status != GX_STATUS_SUCCESS)
        {
            GetErrorString(status);
        }

        //设置上升沿滤波最大值
        status = GXSetFloat(g_device, GX_FLOAT_TRIGGER_FILTER_RAISING, raisingRange.dMax);
        if(status != GX_STATUS_SUCCESS)
        {
            GetErrorString(status);
        }

        //获取当前上升沿滤波值
        double dRaisingValue = 0;
        status = GXGetFloat(g_device, GX_FLOAT_TRIGGER_FILTER_RAISING, &dRaisingValue);
        if(status != GX_STATUS_SUCCESS)
        {
            GetErrorString(status);
        }

    */

    //获取相机输出数据的颜色格式
    status = GXGetEnum(g_device, GX_ENUM_PIXEL_FORMAT, &g_pixel_format);
    if(status != GX_STATUS_SUCCESS)
    {
        GetErrorString(status);
    }
        
    //相机采集图像为彩色还是黑白
    status = GXGetEnum(g_device, GX_ENUM_PIXEL_COLOR_FILTER, &g_color_filter);
    if(status != GX_STATUS_SUCCESS)
    {
        GetErrorString(status);
    }

    //为采集做准备
    ret = PreForImage();
    if(ret != 0)
    {
        printf("<Failed to prepare for acquire image>\n");
        status = GXCloseDevice(g_device);
        if(g_device != NULL)
        {
            g_device = NULL;
        }
        status = GXCloseLib();
        return 0;
    }
 
    //发送开采命令
    status = GXSendCommand(g_device, GX_COMMAND_ACQUISITION_START);
    if(status != GX_STATUS_SUCCESS)
    {
        GetErrorString(status);
        status = GXCloseDevice(g_device);
        if(g_device != NULL)
        {
            g_device = NULL;
        }
        status = GXCloseLib();
        return 0;
    }
    
    //启动接收线程
    ret = pthread_create(&g_acquire_thread, 0, ProcGetImage, 0);
    if(ret != 0)
    {
        printf("<Failed to create the collection thread>\n");
        return 0;
    }

    printf("====================Menu================\n");
    printf("[X or x]:Exit\n");
    printf("[S or s]:Send softtrigger command\n");

    bool run = true;
    while(run == true)
    {
        int c = getchar();
        switch(c)
        {
            //退出程序
            case 'X':
            case 'x':
                run = false;
                break;

            //发送一次软触发命令
            case 'S':
            case 's':
                status = GXSendCommand(g_device, GX_COMMAND_TRIGGER_SOFTWARE);
                printf("<The return value of softtrigger command: %d>\n", status);
                break;

            default:
                break;
        }	
    }
        
    //为停止采集做准备
    ret = UnPreForImage();
    if(ret != 0)
    {
        status = GXCloseDevice(g_device);
        if(g_device != NULL)
        {
            g_device = NULL;
        }
        status = GXCloseLib(); 
        return 0;
    }

    //关闭设备
    printf("<close device>\n");
    status = GXCloseDevice(g_device); 
    if(status != GX_STATUS_SUCCESS)
    {    
        return 0;
    }

    //释放库
    status = GXCloseLib();

    return 0;
}

//-------------------------------------------------
/**
\brief 获取当前帧号
\return 当前帧号
*/
//-------------------------------------------------
int GetCurFrameIndex()
{
    GX_STATUS status = GX_STATUS_SUCCESS;
    status = GXGetBuffer(g_device, GX_BUFFER_FRAME_INFORMATION, (uint8_t*)g_frameinfo_data, &g_frameinfo_datasize);
    if(status != GX_STATUS_SUCCESS)
    {
        //用户可自行处理
    }

    int current_index = 0;
    memcpy(&current_index, (uint8_t*)g_frameinfo_data+FRAMEINFOOFFSET, sizeof(int));

    return current_index;
}

//-------------------------------------------------
/**
\brief 获取图像大小并申请图像数据空间
\return void
*/
//-------------------------------------------------
int PreForImage()
{
    GX_STATUS status = GX_STATUS_SUCCESS;

    //为获取的图像分配存储空间
    int64_t payload_size = 0;
    status = GXGetInt(g_device, GX_INT_PAYLOAD_SIZE, &payload_size); 
    if(status != GX_STATUS_SUCCESS)
    {
        GetErrorString(status);
        return status;
    }

    g_frame_data.pImgBuf = malloc(payload_size);
    if (g_frame_data.pImgBuf == NULL)
    {
        printf("<Failed to allocate memory>\n");
        return MEMORY_ALLOT_ERROR;
    }

    //将非8位raw数据转换成8位数据的时候的中转缓冲buffer
    g_raw8_buffer = malloc(payload_size);
    if (g_raw8_buffer == NULL)
    {
        printf("<Failed to allocate memory>\n");
        return MEMORY_ALLOT_ERROR;
    }

    //RGB数据是RAW数据的3倍大小
    g_rgb_frame_data = malloc(payload_size * 3);
    if (g_rgb_frame_data == NULL)
    {
        printf("<Failed to allocate memory>\n");
        return MEMORY_ALLOT_ERROR;
    }
    
    //获取帧信息长度并申请帧信息数据空间
    status = GXGetBufferLength(g_device, GX_BUFFER_FRAME_INFORMATION, &g_frameinfo_datasize);
    if(status != GX_STATUS_SUCCESS)
    { 
        //用户根据实际情况自行处理
    }
    
    if(g_frameinfo_datasize > 0)
    {    
        g_frameinfo_data = malloc(g_frameinfo_datasize);
        if(g_frameinfo_data == NULL)
        {
            printf("<Failed to allocate memory>\n");
            return MEMORY_ALLOT_ERROR;
        }
    }


    //设置曝光增益白平衡
    GX_FLOAT_RANGE shutterRange;
    status = GXGetFloatRange(g_device, GX_FLOAT_EXPOSURE_TIME, &shutterRange);
    if(status != GX_STATUS_SUCCESS)
    {
        GetErrorString(status);
    }
    //设置曝光值
    double exposure_time = 10000.0000;
    // 限幅
    limitDoubleValue(exposure_time,shutterRange);
    printf("exposure time %f, range %f, %f \n", exposure_time, shutterRange.dMin, shutterRange.dMax);
    status = GXSetFloat(g_device, GX_FLOAT_EXPOSURE_TIME, exposure_time);
    if(status != GX_STATUS_SUCCESS)
    {
        GetErrorString(status);
    }
    
    //选择R白平衡通道
    status = GXSetEnum(g_device, GX_ENUM_BALANCE_RATIO_SELECTOR, GX_BALANCE_RATIO_SELECTOR_RED);
    if(status != GX_STATUS_SUCCESS)
    {
        GetErrorString(status);
    }
    //获取白平衡调节范围
    GX_FLOAT_RANGE ratioRange;
    status = GXGetFloatRange(g_device, GX_FLOAT_BALANCE_RATIO, &ratioRange);
    if(status != GX_STATUS_SUCCESS)
    {
        GetErrorString(status);
    }
    //设置R白平衡系数
    double Rbalance_ratio = 1.6523;
    limitDoubleValue(Rbalance_ratio,ratioRange);
    status = GXSetFloat(g_device, GX_FLOAT_BALANCE_RATIO, Rbalance_ratio);
    if(status != GX_STATUS_SUCCESS)
    {
        GetErrorString(status);
    }
    printf("set red balance ratio %f, range %f ,%f\n",Rbalance_ratio, ratioRange.dMin, ratioRange.dMax);
    
    //选择G白平衡通道
    status = GXSetEnum(g_device, GX_ENUM_BALANCE_RATIO_SELECTOR, GX_BALANCE_RATIO_SELECTOR_GREEN);
    if(status != GX_STATUS_SUCCESS)
    {
        GetErrorString(status);
    }
    //获取白平衡调节范围
    status = GXGetFloatRange(g_device, GX_FLOAT_BALANCE_RATIO, &ratioRange);
    if(status != GX_STATUS_SUCCESS)
    {
        GetErrorString(status);
    }
    //设置G白平衡系数
    double Gbalance_ratio = 1.0000;
    limitDoubleValue(Gbalance_ratio,ratioRange);
    status = GXSetFloat(g_device, GX_FLOAT_BALANCE_RATIO, Gbalance_ratio);
    if(status != GX_STATUS_SUCCESS)
    {
        GetErrorString(status);
    }
    printf("set green balance ratio %f, range %f ,%f\n",Gbalance_ratio, ratioRange.dMin, ratioRange.dMax);

    //选择B白平衡通道
    status = GXSetEnum(g_device, GX_ENUM_BALANCE_RATIO_SELECTOR, GX_BALANCE_RATIO_SELECTOR_BLUE);
    if(status != GX_STATUS_SUCCESS)
    {
        GetErrorString(status);
    }
    //获取白平衡调节范围
    status = GXGetFloatRange(g_device, GX_FLOAT_BALANCE_RATIO, &ratioRange);
    if(status != GX_STATUS_SUCCESS)
    {
        GetErrorString(status);
    }
    //设置B白平衡系数
    double Bbalance_ratio = 1.9414;
    limitDoubleValue(Rbalance_ratio,ratioRange);
    status = GXSetFloat(g_device, GX_FLOAT_BALANCE_RATIO, Bbalance_ratio);
    if(status != GX_STATUS_SUCCESS)
    {
        GetErrorString(status);
    }
    printf("set blue balance ratio %f, range %f ,%f\n",Bbalance_ratio, ratioRange.dMin, ratioRange.dMax);
    
    // 设置增益
    double gainValue = 20.00;
    //选择增益通道类型
    status = GXSetEnum(g_device, GX_ENUM_GAIN_SELECTOR, GX_GAIN_SELECTOR_ALL);
    if(status != GX_STATUS_SUCCESS)
    {
        GetErrorString(status);
    }
    //获取增益调节范围
    GX_FLOAT_RANGE gainRange;
    status = GXGetFloatRange(g_device, GX_FLOAT_GAIN, &gainRange);
    if(status != GX_STATUS_SUCCESS)
    {
        GetErrorString(status);
    }
    limitDoubleValue(gainValue, gainRange);
    //设置增益值
    status = GXSetFloat(g_device, GX_FLOAT_GAIN, gainValue);
    printf("set blue balance ratio %f, range %f ,%f\n",gainValue, gainRange.dMin, gainRange.dMax);



    return 0;
}

//-------------------------------------------------
/**
\brief 释放资源
\return void
*/
//-------------------------------------------------
int UnPreForImage()
{
    GX_STATUS status = GX_STATUS_SUCCESS;
    uint32_t ret = 0;

    g_get_image = false;
    ret = pthread_join(g_acquire_thread,NULL);
    if(ret != 0)
    {
        printf("<Failed to release resources>");
        return ret;
    }
	
    //发送停采命令
    status = GXSendCommand(g_device, GX_COMMAND_ACQUISITION_STOP);
    if(status != GX_STATUS_SUCCESS)
    {
        return status;
    }

    //释放buffer
    if(g_frame_data.pImgBuf != NULL)
    {
        free(g_frame_data.pImgBuf);
	g_frame_data.pImgBuf = NULL;
    }

    if(g_raw8_buffer != NULL)
    {
        free(g_raw8_buffer);
        g_raw8_buffer = NULL;
    }

    if(g_rgb_frame_data != NULL)
    {
        free(g_rgb_frame_data);
        g_rgb_frame_data = NULL;
    }

    if(g_frameinfo_data != NULL)
    {
        free(g_frameinfo_data);
        g_frameinfo_data = NULL;
    }
 
    return 0;
}


//-------------------------------------------------
/**
\brief 采集线程函数
\param param 线程传入参数
\return void*
*/
//-------------------------------------------------
void *ProcGetImage(void* param)
{
    GX_STATUS status = GX_STATUS_SUCCESS;
    bool is_implemented = false;
    //接收线程启动标志
    g_get_image = true;

    while(g_get_image)
    {

        status = GXGetImage(g_device, &g_frame_data, 100);
        
        if(status == GX_STATUS_SUCCESS)
        {
            if(g_frame_data.nStatus == 0)
            {
                g_time_counter.Begin();
                printf("<Successful acquisition: Width: %d Height: %d>\n", g_frame_data.nWidth, g_frame_data.nHeight);
                status = GXIsImplemented(g_device, GX_BUFFER_FRAME_INFORMATION, &is_implemented);
                if(status == GX_STATUS_SUCCESS)
                {
                    if(true == is_implemented)
                    {
                        printf("<Frame number: %d>\n", GetCurFrameIndex());
                    }
                }

                //保存Raw数据
                //SaveRawFile(g_frame_data.pImgBuf, g_frame_data.nWidth, g_frame_data.nHeight);

                //将Raw数据处理成RGB数据
                ProcessData(g_frame_data.pImgBuf, 
                        g_raw8_buffer, 
                        g_rgb_frame_data, 
                        g_frame_data.nWidth, 
                        g_frame_data.nHeight,
                        g_pixel_format,
                        g_color_filter); 
                
                //保存RGB数据
                SavePPMFile(g_rgb_frame_data, g_frame_data.nWidth, g_frame_data.nHeight);
                
                printf("time of process %ld us\n", g_time_counter.End());
            }
        }
    }
}