#include "Log.hpp"

struct ThreadArg
{
    pthread_t thread_id;
    int dur;
};

void* thread_fun(void* args){
    ThreadArg *ptr;
    ptr = (struct ThreadArg*)args;
    int sleeptime = ptr->dur;
    pthread_t id = ptr->thread_id;
    int n = 1;
    while(true){
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = sleeptime * 1000;
        select(0,NULL,NULL,NULL,&tv);
        
        sLog->logInfo("Test thread %d %d", id, n);
        printf("Thread %d: %d\n", id, n);
        n++;
    }
    
}

int main(int argc, char const *argv[])
{
    for(int i = 1; i < 7; i++)
    {
        pthread_t id = i;
        ThreadArg arg = {i,i*100};
        pthread_create(&id,NULL,thread_fun,(void*)&arg);
    }
#ifdef TEST_SINGLE_THREAD    
    sLog->init("../log","test",Logger::ERROR);
#else
    bool run = true;
    while(run){
        int c = getchar();
        switch (c)
        {
            case 27:
            case 'x':
            case 'X':
                run = false;
                break;        
            default:
                break;
        }
    }
#endif
    return 0;
}
