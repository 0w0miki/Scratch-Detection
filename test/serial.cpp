#include "Serial.h"
#include <iostream>

Serial ss;
void thread_fun(){
    std::string msg;
    ss.readMsg(msg);
    
}

int main(int argc, char const *argv[])
{
    ss.init("/dev/ttyUSB0");
    ss.setBaudRate(19200);
    ss.setParity(8,1,'n');
    // int result = 2;
    char buff[8]={0};
    buff[0] = 0x06;
    buff[1] = 0x01;
    buff[4] = 0x01;
    buff[6] = 0x88;
    buff[7] = 0x5A;
    std::string msg(buff);
    std::cout<<msg<<std::endl;
    printf("send %d byte",ss.sendMsg(msg));
    return 0;
}
