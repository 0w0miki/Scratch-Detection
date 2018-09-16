#include "Camera.h"

CTimeCounter g_time_counter;                                      ///< 计时器

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

    // 新建相机对象
    Camera camera;
    // 初始化相机
    camera.init();
    // 设置触发
    camera.setTrigger();
    
    ret = camera.applyParam();
    if(ret != 0) printf("some thing error with camera parameter");

    //发送开采命令
    ret = camera.start();
    if(ret != 0) printf("[WARN] failed to start camera");

    std::vector<ROI> rois;
    struct ROI a = {
        .x=0,
        .y=0,
        .w=400,
        .h=500
    };
    rois.push_back(a);
    struct ROI b = {
        .x=700,
        .y=800,
        .w=400,
        .h=500
    };
    rois.push_back(b);
    camera.setROI(rois);
    
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
                ret = camera.sendSoftTrigger();
                printf("<The return value of softtrigger command: %d>\n", ret);
                break;

            default:
                break;
        }	
    }

    ret = camera.stop();
    if(ret != 0) printf("[WARN] failed to stop camera");

    return 0;
}