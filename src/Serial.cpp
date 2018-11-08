#include "Serial.h"

/** 
    *@brief 设置串口通信速率 
    *@param baud_rate 类型 int 串口速度 
    *@return 类型 int -1 problem 0 ok 
**/ 
int Serial::setBaudRate(const int baud_rate){
    int status;
    struct termios Opt; 
    tcgetattr(fd_, &Opt); 
    for (int i= 0; i < sizeof(baud_arr_) / sizeof(int); i++) { 
        if (baud_rate == name_arr_[i]) { 
            tcflush(fd_, TCIOFLUSH); 
            cfsetispeed(&Opt, baud_arr_[i]); 
            cfsetospeed(&Opt, baud_arr_[i]); 
            status = tcsetattr(fd_, TCSANOW, &Opt); 
            if (status != 0) { 
                perror("tcsetattr fd"); 
                return -1; 
            } 
            tcflush(fd_,TCIOFLUSH); 
        } 
    }
    return 0;
}

/** 
    *@brief 设置串口数据位，停止位和效验位 
    *@param databits 类型 int 数据位 取值 为 7 或者8 
    *@param stopbits 类型 int 停止位 取值为 1 或者2 
    *@param parity 类型 int 效验类型 取值为N,E,O,S 
**/ 
int Serial::setParity(const int databits, const int stopbits, const int parity){
    struct termios options; 
    if ( tcgetattr( fd_,&options) != 0) { 
        perror("SetupSerial 1"); 
        return -1; 
    } 
    options.c_cflag &= ~CSIZE; 
    switch (databits) /*设置数据位数*/ { 
        case 7: 
            options.c_cflag |= CS7; 
            break; 
        case 8: 
            options.c_cflag |= CS8; 
            break; 
        default: 
            fprintf(stderr,"Unsupported data sizen"); 
            return -1; 
    } 
    switch (parity) { 
        case 'n': 
        case 'N': 
            options.c_cflag &= ~PARENB; /* Clear parity enable */ 
            options.c_iflag &= ~INPCK; /* Enable parity checking */ 
            break; 
        case 'o': 
        case 'O': 
            options.c_cflag |= (PARODD | PARENB); /* 设置为奇效验*/ 
            options.c_iflag |= INPCK; /* Disnable parity checking */ 
            break; 
        case 'e': 
        case 'E': 
            options.c_cflag |= PARENB; /* Enable parity */ 
            options.c_cflag &= ~PARODD; /* 转换为偶效验*/ 
            options.c_iflag |= INPCK; /* Disnable parity checking */ 
            break; 
        case 'S':
        case 's': /*as no parity*/ 
            options.c_cflag &= ~PARENB; 
            options.c_cflag &= ~CSTOPB;
            break; 
        default: 
            fprintf(stderr,"Unsupported parityn"); 
            return -1; 
    } 
    /* 设置停止位*/ 
    switch (stopbits) { 
        case 1: 
            options.c_cflag &= ~CSTOPB; 
            break; 
        case 2: 
            options.c_cflag |= CSTOPB; 
            break; 
        default: 
            fprintf(stderr,"Unsupported stop bitsn"); 
            return -1; 
    } 
    /* Set input parity option */ 
    if (parity != 'n') 
        options.c_iflag |= INPCK; 
    tcflush(fd_,TCIFLUSH); 
    options.c_cc[VTIME] = 150; 
    /* 设置超时15 seconds*/ 
    options.c_cc[VMIN] = 0; 
    /* Update the options and do it NOW */ 
    if (tcsetattr(fd_,TCSANOW,&options) != 0) { 
        perror("SetupSerial 3"); 
        return -1; 
    }
    return 0;
}

int Serial::init(char* port){
    fd_ = open( port, O_RDWR); /*以读写方式打开串口*/ 
    if (-1 == fd_){ 
        /* 不能打开串口一*/ 
        perror(" Cannot Open Serial Port! "); 
        return -1;
    }
    return 0;
}

int Serial::sendMsg(const std::string msg){
    int Length = msg.size();
    printf("length: %d",Length);
    int nByte = write(fd_, msg.c_str(), Length);
    return nByte;
}

int Serial::readMsg(std::string &msg){
    char buff[1024];
    int readByte = read(fd_,buff,1024);
    msg = buff;
    return readByte;
}

int Serial::stop(){
    return close(fd_);
}