#include "Serial.h"
#include <iostream>

// /** 
//     *@brief 设置串口通信速率 
//     *@param fd 类型 int 打开串口的文件句柄 
//     *@param speed 类型 int 串口速度 
//     *@return void 
// **/ 
// int speed_arr[] = { B115200, B38400, B19200, B9600, B4800, B2400, B1200, B300}; 
// int name_arr[] = {115200, 38400, 19200, 9600, 4800, 2400, 1200, 300}; 
// void set_speed(int fd, int speed){ 
//     int i; 
//     int status;
//     struct termios Opt; 
//     tcgetattr(fd, &Opt); 
//     for ( i= 0; i < sizeof(speed_arr) / sizeof(int); i++) { 
//         if (speed == name_arr[i]) { 
//             tcflush(fd, TCIOFLUSH); 
//             cfsetispeed(&Opt, speed_arr[i]); 
//             cfsetospeed(&Opt, speed_arr[i]); 
//             status = tcsetattr(fd, TCSANOW, &Opt); 
//             if (status != 0) { 
//                 perror("tcsetattr fd"); 
//                 return; 
//             } 
//             tcflush(fd,TCIOFLUSH); 
//         } 
//     } 
// }

// /** 
//     *@brief 设置串口数据位，停止位和效验位 
//     *@param fd 类型 int 打开的串口文件句柄 
//     *@param databits 类型 int 数据位 取值 为 7 或者8 
//     *@param stopbits 类型 int 停止位 取值为 1 或者2 
//     *@param parity 类型 int 效验类型 取值为N,E,O,S 
// **/ 
// int set_Parity(int fd,int databits,int stopbits,int parity) { 
//     struct termios options; 
//     if ( tcgetattr( fd,&options) != 0) { 
//         perror("SetupSerial 1"); 
//         return(false); 
//     } 
//     options.c_cflag &= ~CSIZE; 
//     switch (databits) /*设置数据位数*/ { 
//         case 7: 
//             options.c_cflag |= CS7; 
//             break; 
//         case 8: 
//             options.c_cflag |= CS8; 
//             break; 
//         default: 
//             fprintf(stderr,"Unsupported data sizen"); 
//             return (false); 
//     } 
//     switch (parity) { 
//         case 'n': 
//         case 'N': 
//             options.c_cflag &= ~PARENB; /* Clear parity enable */ 
//             options.c_iflag &= ~INPCK; /* Enable parity checking */ 
//             break; 
//         case 'o': 
//         case 'O': 
//             options.c_cflag |= (PARODD | PARENB); /* 设置为奇效验*/ 
//             options.c_iflag |= INPCK; /* Disnable parity checking */ 
//             break; 
//         case 'e': 
//         case 'E': 
//             options.c_cflag |= PARENB; /* Enable parity */ 
//             options.c_cflag &= ~PARODD; /* 转换为偶效验*/ 
//             options.c_iflag |= INPCK; /* Disnable parity checking */ 
//             break; 
//         case 'S':
//         case 's': /*as no parity*/ 
//             options.c_cflag &= ~PARENB; 
//             options.c_cflag &= ~CSTOPB;
//             break; 
//         default: 
//             fprintf(stderr,"Unsupported parityn"); 
//             return (false); 
//     } 
//     /* 设置停止位*/ 
//     switch (stopbits) { 
//         case 1: 
//             options.c_cflag &= ~CSTOPB; 
//             break; 
//         case 2: 
//             options.c_cflag |= CSTOPB; 
//             break; 
//         default: 
//             fprintf(stderr,"Unsupported stop bitsn"); 
//             return (false); 
//     } 
//     /* Set input parity option */ 
//     if (parity != 'n') 
//         options.c_iflag |= INPCK; 
//     tcflush(fd,TCIFLUSH); 
//     options.c_cc[VTIME] = 150; 
//     /* 设置超时15 seconds*/ 
//     options.c_cc[VMIN] = 0; 
//     /* Update the options and do it NOW */ 
//     if (tcsetattr(fd,TCSANOW,&options) != 0) { 
//         perror("SetupSerial 3"); 
//         return (false); 
//     }
//     return (true);
// } 

int main(int argc, char const *argv[])
{
    Serial ss;    
    ss.init("/dev/ttyUSB0");
    ss.setBaudRate(115200);
    ss.setParity(8,2,'o');
    int result = 2;
    char buff[6]={0};
    buff[0] = 0xAA;
    buff[1] = 0x01;
    buff[2] = (char)(result & 0x00ff);
    std::string msg(buff);
    std::cout<<msg<<std::endl;
    printf("send %d byte",ss.sendMsg(msg));
    // int fd; 
    // fd = open( "/dev/ttyUSB0", O_RDWR); /*以读写方式打开串口*/ 
    // if (-1 == fd){ 
    //     /* 不能打开串口一*/ 
    //     perror(" Cannot Open Serial Port! "); 
    // }
    
    // set_speed(fd,115200);
    // if(!set_Parity(fd, 8, 1, 'n')){
    //     perror(" Set Parity Errorn! ");
    //     return -1;
    // }
    
    // char *buffer = "what's up\n";
    // int Length = 10;
    // int nByte;
    // nByte = write(fd, buffer ,Length);
    // printf("send %d bytes\n",nByte);
    // close(fd);   

    return 0;
}
