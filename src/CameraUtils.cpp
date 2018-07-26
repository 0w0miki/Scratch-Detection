#include "CameraUtils.h"

//----------------------------------------------------------------------------------
/**
\brief  double类型限幅
\param  [in,out]dvalue double值
\param  [in]range 上下界

\return void
*/
//----------------------------------------------------------------------------------
void limitDoubleValue(double & dvalue, GX_FLOAT_RANGE range)
{
    if(dvalue < range.dMin){
        dvalue = range.dMin;
    }
    if(dvalue > range.dMax){
        dvalue = range.dMax;
    }
}

//----------------------------------------------------------------------------------
/**
\brief  获取错误信息描述
\param  error_status  错误码

\return void
*/
//----------------------------------------------------------------------------------
void GetErrorString(GX_STATUS error_status){
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
void ProcessData(void *image_buffer, void *image_raw8_buffer, void *image_rgb_buffer, int image_width, int image_height, int pixel_format, int color_filter){
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
            //g_time_counter.Begin();
            DxRaw8toRGB24(image_buffer,image_rgb_buffer, image_width, image_height,RAW2RGB_NEIGHBOUR,
                 DX_PIXEL_COLOR_FILTER(color_filter),false);	
            //printf("<DxRaw8toRGB24 time consuming: %ld us>\n", g_time_counter.End());
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

//-------------------------------------------------
/**
\brief 保存raw数据文件
\param image_buffer raw图像数据
\param width 图像宽
\param height 图像高
\return void
*/
//-------------------------------------------------
void SaveRawFile(void *image_buffer, size_t width, size_t height){
    char name[64] = {0};

    static int raw_file_index = 1;
    FILE* ff = NULL;

    sprintf(name, "RAW%d.pgm", raw_file_index++);
    ff=fopen(name,"wb");
    if(ff != NULL)
    {
        fprintf(ff, "P5\n" "%zu %zu 255\n", width, height);
        fwrite(image_buffer, 1, width * height, ff);
        fclose(ff);
        ff = NULL;
        printf("<Save %s success>\n", name);
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
void SavePPMFile(void *image_buffer, size_t width, size_t height){
    char name[64] = {0};

    // static int rgb_file_index = 1;
    FILE* ff = NULL;

    sprintf(name, "RGB%d.ppm", rgb_file_index++);
    ff=fopen(name,"wb");
    if(ff != NULL)
    {
        fprintf(ff, "P6\n" "%zu %zu\n255\n", width, height);
        fwrite(image_buffer, 1, width * height * 3, ff);
        fclose(ff);
        ff = NULL;
        printf("<Save %s success>\n", name);
    }
}

void SavePPMwithROIs(void *image_buffer, size_t width, size_t height, std::vector<ROI> ROIs){
    char name[64] = {0};

    // static int rgb_file_index = 1;
    FILE* ff = NULL;
    
    if(ROIs.empty()){
        SavePPMFile(image_buffer, width, height);
        return;
    }

    for(size_t i = 0;i<ROIs.size();i++){
        sprintf(name, "RGB%d.ppm", rgb_file_index++);
        ff=fopen(name,"wb");
        if(ff != NULL)
        {
            fprintf(ff, "P6\n" "%d %d\n255\n", ROIs[i].w, ROIs[i].h);
            for(size_t j = 0; j < ROIs[i].h; j++){
                fwrite(image_buffer, 1, ROIs[i].w * 3, ff);
            }
            fclose(ff);
            ff = NULL;
            printf("<Save %s success>\n", name);
        }
    }
    
}