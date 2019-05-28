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
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include "Log.hpp"
#include "CameraUtils.h"
#include "json/json.h"
#include "utils.h"

#define FRAMEINFOOFFSET 14
#define MEMORY_ALLOT_ERROR -1
#define CAM_RED_BALANCE_CHANNEL (1)
#define CAM_GREEN_BALANCE_CHANNEL (2)
#define CAM_BLUE_BALANCE_CHANNEL (3)

#define SOFT_TRIGGER 21
#define HARD_TRIGGER 20

#define TRIGER_LINE0 0
#define TRIGER_LINE1 1
#define TRIGER_LINE2 2
#define TRIGER_LINE3 3

#define TRIGER_FALLING 0
#define TRIGER_RISING  1

class Camera
{
enum CAMERA_STATE{
    CAMERA_STOP,                                                // 停止
    CAMERA_RUN,                                                 // 运行中
    CAMERA_READY_TO_PAUSE,                                      // 准备暂停
    CAMERA_PAUSE                                                // 暂停中
};

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

    CTimeCounter g_time_counter_;                               ///< 计时器

    std::vector<ROI> ROIs_;                                     ///< ROI
    std::queue<std::string>* unsolved_list_;                    ///< 未处理图像文件名列表
    std::deque<std::string>* work_name_list_;                   ///< 作业名列表
    std::deque<int>* work_count_list_;                          ///< 作业数列表
    std::vector<std::vector<ROI>>* batch_ROI_list_;             ///< ROI列表
    std::vector<std::vector<ROI>>::iterator batch_ROI_iter_;    ///< ROI列表迭代器

    // param
    double shutter_time_;                                       ///< 曝光时间
    double red_balance_;                                        ///< 白平衡 红
    double green_balance_;                                      ///< 白平衡 绿
    double blue_balance_;                                       ///< 白平衡 蓝
    double gain_value_;                                         ///< 增益(调节亮度变化)

    int triger_line_;                                           ///< 触发线
    int triger_edge_;                                           ///< 上升沿/下降沿

    std::string file_dir_;                                      ///< 保存图片路径
    int64_t count_;                                             ///< 当前拍摄了的计数

    int fake_ptr_;                                              ///< 指示当前处理的是工作队列的哪个

    CAMERA_STATE state_ = CAMERA_STOP;                          ///< 运行状态

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

    // 保存黑白图
    void SaveMono(void *image_buffer, size_t width, size_t height);

    // 保存ROI中的图像到PPM
    void SavePPMwithROIs(void *image_buffer, size_t width, size_t height, std::vector<ROI> ROIs);

public:
    Camera();
    Camera( pthread_mutex_t* mutex, 
            std::queue<std::string>* unsolved_list);
    Camera( pthread_mutex_t* mutex, 
            std::queue<std::string>* unsolved_list, 
            std::deque<int>* work_count_list, 
            std::vector<std::vector<ROI>>* work_ROI_list,
            int64_t pixel_format = GX_PIXEL_FORMAT_BAYER_GR8);
    Camera( pthread_mutex_t* mutex,
            std::queue<std::string>* unsolved_list, 
            std::deque<std::string>* work_name_list, 
            std::deque<int>* work_count_list,
            int64_t pixel_format = GX_PIXEL_FORMAT_BAYER_GR8);
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
    int setTrigger(int type = SOFT_TRIGGER);
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
    // 读取相机拍摄计数
    int64_t getCount();
    // 修正fake_ptr位置
    int popList();
    // 重置fake_ptr
    inline void resetBias(){fake_ptr_ = 0;}
    // 设置计数器
    void setCount(int64_t count){count_ = count;}
    // 是否成功暂停
    inline bool isPause(){return CAMERA_PAUSE == state_;}
    // 暂停
    void callPause(){state_ = CAMERA_READY_TO_PAUSE;}
    // 继续
    void restart(){state_ = CAMERA_RUN;}
    // 返回是否是软触发 通知网页用
    bool isSoftTrigger();
};



#endif // CAMERA_H
