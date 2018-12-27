#include "GxIAPI.h"
#include "DxImageProc.h"
#include "CTimeCounter.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

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

//保存数据到PPM文件
void SavePPMFile(void *image_buffer, size_t width, size_t height);

//保存raw数据文件
void SaveRawFile(void *image_buffer, size_t width, size_t height);

//将相机输出的原始数据转换为RGB数据
void ProcessData(void *image_buffer, void *image_raw8_buffer, void *image_rgb_buffer, int image_width, int image_height, int pixel_format, int color_filter);

//获取错误信息描述
void GetErrorString(GX_STATUS error_status);

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
    open_param.accessMode = GX_ACCESS_EXCLUSIVE;
    open_param.openMode = GX_OPEN_INDEX;
    open_param.pszContent = "1";       

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
                // SaveRawFile(g_frame_data.pImgBuf, g_frame_data.nWidth, g_frame_data.nHeight);

                //将Raw数据处理成RGB数据
                ProcessData(g_frame_data.pImgBuf, 
                        g_raw8_buffer, 
                        g_rgb_frame_data, 
                        g_frame_data.nWidth, 
                        g_frame_data.nHeight,
                        g_pixel_format,
                        g_color_filter);
                
                cv::Mat img(g_frame_data.nHeight, g_frame_data.nWidth, CV_8UC3, (uchar*)g_rgb_frame_data);
                cv::imshow("original image",img);
                cv::waitKey(1);
                //保存RGB数据
                SavePPMFile(g_rgb_frame_data, g_frame_data.nWidth, g_frame_data.nHeight);
            }
        }
    }
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
void SavePPMFile(void *image_buffer, size_t width, size_t height)
{
    char name[64] = {0};

    static int rgb_file_index = 1;
    FILE* ff = NULL;

    sprintf(name, "RGB%d.ppm", rgb_file_index++);
    ff=fopen(name,"wb");
    if(ff != NULL)
    {
        fprintf(ff, "P6\n" "%d %d\n255\n", width, height);
        fwrite(image_buffer, 1, width * height * 3, ff);
        fclose(ff);
        ff = NULL;
        printf("<Save %s success>\n", name);
    }    
}

//-------------------------------------------------
/**
\brief 保存raw数据文件
\param image_buffer raw图像数据
\param width 图像宽
\param height 图像高
\return void
*/
//-------------------------------------------------
void SaveRawFile(void *image_buffer, size_t width, size_t height)
{
    char name[64] = {0};

    static int raw_file_index = 1;
    FILE* ff = NULL;

    sprintf(name, "RAW%d.pgm", raw_file_index++);
    ff=fopen(name,"wb");
    if(ff != NULL)
    {
        fprintf(ff, "P5\n" "%u %u 255\n", width, height);
        fwrite(image_buffer, 1, width * height, ff);
        fclose(ff);
        ff = NULL;
        printf("<Save %s success>\n", name);
    }
}

//----------------------------------------------------------------------------------
/**
\brief  将相机输出的原始数据转换为RGB数据
\param  [in] image_buffer  指向图像缓冲区的指针
\param  [in] image_raw8_buffer  将非8位的Raw数据转换成8位的Raw数据的中转缓冲buffer
\param  [in,out] image_rgb_buffer  指向RGB数据缓冲区的指针
\param  [in] image_width 图像宽
\param  [in] image_height 图像高
\param  [in] pixel_format 图像的格式
\param  [in] color_filter Raw数据的像素排列格式
\return 无返回值
*/
//----------------------------------------------------------------------------------
void ProcessData(void *image_buffer, void *image_raw8_buffer, void *image_rgb_buffer, int image_width, int image_height, int pixel_format, int color_filter)
{
    switch(pixel_format)
    {
        //当数据格式为12位时，位数转换为4-11
        case GX_PIXEL_FORMAT_BAYER_GR12:
        case GX_PIXEL_FORMAT_BAYER_RG12:
        case GX_PIXEL_FORMAT_BAYER_GB12:
        case GX_PIXEL_FORMAT_BAYER_BG12:
            //将12位格式的图像转换为8位格式
            DxRaw16toRaw8(image_buffer, image_raw8_buffer, image_width, image_height, DX_BIT_4_11);	
            //将Raw8图像转换为RGB图像以供显示
            DxRaw8toRGB24(image_raw8_buffer, image_rgb_buffer, image_width, image_height, RAW2RGB_NEIGHBOUR,
                DX_PIXEL_COLOR_FILTER(color_filter), false);		        
            break;

        //当数据格式为10位时，位数转换为2-9
        case GX_PIXEL_FORMAT_BAYER_GR10:
        case GX_PIXEL_FORMAT_BAYER_RG10:
        case GX_PIXEL_FORMAT_BAYER_GB10:
        case GX_PIXEL_FORMAT_BAYER_BG10:
            //将10位格式的图像转换为8位格式,有效位数2-9
            DxRaw16toRaw8(image_buffer, image_raw8_buffer, image_width, image_height, DX_BIT_2_9);
            //将Raw8图像转换为RGB图像以供显示
            DxRaw8toRGB24(image_raw8_buffer, image_rgb_buffer, image_width, image_height,RAW2RGB_NEIGHBOUR,
                DX_PIXEL_COLOR_FILTER(color_filter),false);	
            break;

        case GX_PIXEL_FORMAT_BAYER_GR8:
        case GX_PIXEL_FORMAT_BAYER_RG8:
        case GX_PIXEL_FORMAT_BAYER_GB8:
        case GX_PIXEL_FORMAT_BAYER_BG8:
            //将Raw8图像转换为RGB图像以供显示
            g_time_counter.Begin();
            DxRaw8toRGB24(image_buffer,image_rgb_buffer, image_width, image_height,RAW2RGB_NEIGHBOUR,
                 DX_PIXEL_COLOR_FILTER(color_filter),false);	
            printf("<DxRaw8toRGB24 time consuming: %ld us>\n", g_time_counter.End());
            break;

        case GX_PIXEL_FORMAT_MONO12:
            //将12位格式的图像转换为8位格式
            DxRaw16toRaw8(image_buffer, image_raw8_buffer, image_width, image_height, DX_BIT_4_11);	
            //将Raw8图像转换为RGB图像以供显示
            DxRaw8toRGB24(image_raw8_buffer, image_rgb_buffer, image_width, image_height, RAW2RGB_NEIGHBOUR,
                DX_PIXEL_COLOR_FILTER(NONE),false);		        
            break;

        case GX_PIXEL_FORMAT_MONO10:
            //将10位格式的图像转换为8位格式
            DxRaw16toRaw8(image_buffer, image_raw8_buffer, image_width, image_height, DX_BIT_4_11);	
            //将Raw8图像转换为RGB图像以供显示
            DxRaw8toRGB24(image_raw8_buffer, image_rgb_buffer, image_width, image_height,RAW2RGB_NEIGHBOUR,
                DX_PIXEL_COLOR_FILTER(NONE),false);		        
            break;

        case GX_PIXEL_FORMAT_MONO8:
            //将Raw8图像转换为RGB图像以供显示
            DxRaw8toRGB24(image_buffer, image_rgb_buffer, image_width, image_height,RAW2RGB_NEIGHBOUR,
            DX_PIXEL_COLOR_FILTER(NONE),false);		        
            break;

        default:
            break;
    }
}

//----------------------------------------------------------------------------------
/**
\brief  获取错误信息描述
\param  error_status  错误码

\return void
*/
//----------------------------------------------------------------------------------
void GetErrorString(GX_STATUS error_status)
{
    char *error_info = NULL;
    size_t    size         = 0;
    GX_STATUS status      = GX_STATUS_SUCCESS;
	
    // 获取错误描述信息长度
    status = GXGetLastError(&error_status, NULL, &size);
    error_info = new char[size];
    if (error_info == NULL)
    {
        printf("<Failed to allocate memory>\n");
        return ;
    }
	
    // 获取错误信息描述
    status = GXGetLastError(&error_status, error_info, &size);
    if (status != GX_STATUS_SUCCESS)
    {                                        
        printf("<GXGetLastError call fail>\n");
    }
    else
    {
        printf("%s\n", (char*)error_info);
    }

    // 释放资源
    if (error_info != NULL)
    {
        delete[]error_info;
        error_info = NULL;
    }
}