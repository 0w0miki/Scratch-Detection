#ifndef CAMERAUTILS_H
#define CAMERAUTILS_H

#include "GxIAPI.h"
#include <vector>
#include "DxImageProc.h"
#include <stdio.h>

//----------------------------------------------------------------------------------
/**
\brief  ROI结构体

x,y     w
   ┌---------┐
   |         |h
   └---------┘
*/
//----------------------------------------------------------------------------------
struct ROI
{
    int x;
    int y;
    int32_t w;
    int32_t h;
};

// 限幅
void limitDoubleValue(double & dvalue, GX_FLOAT_RANGE range);

//获取错误信息描述
void GetErrorString(GX_STATUS error_status);

//将相机输出的原始数据转换为RGB数据
void ProcessData(void *image_buffer, void *image_raw8_buffer, void *image_rgb_buffer, int image_width, int image_height, int pixel_format, int color_filter);

//保存raw数据文件
void SaveRawFile(void *image_buffer, size_t width, size_t height);

//保存数据到PPM文件
void SavePPMFile(void *image_buffer, size_t width, size_t height);

// 保存ROI中的图像到PPM
void SavePPMwithROIs(void *image_buffer, size_t width, size_t height, std::vector<ROI> ROIs);

#endif // CAMERAUTILS_H


