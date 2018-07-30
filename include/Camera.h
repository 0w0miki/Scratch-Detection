#ifndef CAMERA_H
#define CAMERA_H

#include "GxIAPI.h"
#include "DxImageProc.h"
#include "CTimeCounter.h"
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string>
#include <queue>
#include "CameraUtils.h"
#include "json/json.h"

#define FRAMEINFOOFFSET 14
#define MEMORY_ALLOT_ERROR -1
#define CAM_RED_BALANCE_CHANNEL (1)
#define CAM_GREEN_BALANCE_CHANNEL (2)
#define CAM_BLUE_BALANCE_CHANNEL (3)

class Camera
{

private:
    GX_DEV_HANDLE g_device_;                                    ///< 设备句柄
    GX_FRAME_DATA g_frame_data_;                                ///< 采集图像参数
    void *g_raw8_buffer_;                                       ///< 将非8位raw数据转换成8位数据的时候的中转缓冲buffer
    void *g_rgb_frame_data_;                                    ///< RAW数据转换成RGB数据后的存储空间，大小是相机输出数据大小的3倍
    int64_t g_pixel_format_;                                    ///< 当前相机的pixelformat格式
    int64_t g_color_filter_;                                    ///< bayer插值的参数
    pthread_t g_acquire_thread_;                                ///< 采集线程ID
    bool g_get_image_;                                          ///< 采集线程是否结束的标志：true 运行；false 退出
    void *g_frameinfo_data_;                                    ///< 帧信息数据缓冲区
    size_t g_frameinfo_datasize_;                               ///< 帧信息数据长度
    pthread_mutex_t* mutex_;                                    ///< 线程互斥锁

    int cam_id_;                                                ///< 相机编号

    CTimeCounter g_time_counter_;                                ///< 计时器

    std::vector<ROI> ROIs_;                                      ///< ROI
    std::queue<std::string>* unsolved_list_;                          ///< 未处理图像文件名列表

    // param
    double shutter_time_;
    double red_balance_;
    double green_balance_;
    double blue_balance_;
    double gain_value_;

    std::string file_dir_;

protected:

    //释放资源
    int UnPreForImage();

    //获取当前帧号
    int GetCurFrameIndex();

    // 线程入口
    static void* ProcGetImage_pth(void* arg);
    
    //采集线程函数
    void ProcGetImage();

    //保存数据到PPM文件
    void SavePPMFile(void *image_buffer, size_t width, size_t height);

    // 保存ROI中的图像到PPM
    void SavePPMwithROIs(void *image_buffer, size_t width, size_t height, std::vector<ROI> ROIs);

public:
    Camera();
    Camera(pthread_mutex_t* mutex, std::queue<std::string>* unsolved_list);
    ~Camera();
    
    // 相机初始化
    int init();
    // 获取图像大小并申请图像数据空间
    int PreForImage();
    // 开始采集
    int start();
    // 停止相机
    int stop();
    // 设置触发
    int setTrigger(int type = 0);
    // 设置曝光
    int setShutter(double shutter_time);
    // 设置白平衡
    int setBalance(double ratio, int channel);
    // 设置增益
    int setGain(double gain_value);
    // 进行软触发
    int sendSoftTrigger();
    // 设置ROI
    void setROI(std::vector<ROI> rois);
    // 读取并设置参数
    int applyParam();
};



#endif // CAMERA_H
