#include "Camera.h"
#include "Detector.h"

int main(int argc, char const *argv[])
{
    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, NULL);

    std::queue<string> unsolved_list;

    Camera camera(&mutex, &unsolved_list);
    Detector detector(&mutex, &unsolved_list);

    camera.init();
    camera.setTrigger();
    camera.applyParam();

    detector.setOriginImg("template.JPG");
    // 300 350 2150 1500
    std::vector<ROI> rois;
    struct ROI a = {
        .x=300,
        .y=350,
        .w=2150,
        .h=1500
    };
    rois.push_back(a);
    camera.setROI(rois);

    //发送开采命令
    int ret = camera.start();
    if(ret != 0) printf("[WARN] failed to start camera");

    ret = detector.launchThread();
    if(ret != 0) printf("[WARN] failed to launch detection");

    printf("====================Menu================\n");
    printf("[X or x]:Exit\n");
    printf("[S or s]:Send softtrigger command\n");

    bool run = true;
    while(run == true)
    {
        int64_t detect_count = detector.getCount();
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

    // end process
    ret = camera.stop();
    if(ret != 0) printf("[WARN] failed to stop camera");

    cv::destroyAllWindows();
    return 0;
}