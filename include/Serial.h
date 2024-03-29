#if !defined(SERIAL_H)
#define SERIAL_H

#include <stdio.h> /*标准输入输出定义*/
#include <stdlib.h> /*标准函数库定义*/
#include <unistd.h> /*Unix 标准函数定义*/
// linux下读取usb就是读取usb文件
#include <sys/types.h> 
#include <sys/stat.h> 
#include <fcntl.h> /*文件控制定义*/
#include <termios.h> /*PPSIX 终端控制定义*/
#include <errno.h> /*错误号定义*/
#include <string>


class Serial
{
private:
    
    int baud_arr_[20] = { B115200, B57600, B38400, B19200, B9600, B4800, B2400, B1200, B300}; 
    int name_arr_[20] = {115200, 57600, 38400, 19200, 9600, 4800, 2400, 1200, 300};
    int fd_;

public:
    Serial(/* args */) { }
    ~Serial() { }
    // 设置波特率
    int setBaudRate(const int baud_rate);
    // 设置奇偶校验位
    int setParity(const int databits, const int stopbits, const int parity);
    // 初始化
    int init(const char* port);
    // 发送消息
    int sendMsg(char* msg, int size);
    // 读取消息
    int readMsg(std::string &msg);
    // 停
    int stop();
};

#endif // SERIAL_H
