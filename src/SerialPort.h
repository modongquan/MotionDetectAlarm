#ifndef USART_H
#define USART_H

//串口相关的头文件
#include <stdio.h>  /*标准输入输出定义*/
#include <stdlib.h> /*标准函数库定义*/
#include <unistd.h> /*Unix 标准函数定义*/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>   /*文件控制定义*/
#include <termios.h> /*PPSIX 终端控制定义*/
#include <errno.h>   /*错误号定义*/
#include <string.h>
#include <string>
#include <map>
#include "global.h"

using namespace std;

#define UART_FALSE -1
#define UART_TRUE 0

#define GNR_COM 1
#define USB_COM 2
#define COM_TYPE USB_COM

#define USB0 0
#define USB1 1
#define USB2 2

#define READ_LINE_BUF_SIZE  256

class SerialPort
{
public:
    int fd; //文件描述符
    int isnormal;
    SerialPort();      //构造函数
    ~SerialPort(void); //析构函数

    void SetPtrLogger(std::shared_ptr<spdlog::logger> logger);
    char ToChar(uint8_t val);
    int Utf8CodeFormat(char CharIn);
    void ConvertToUtf16Be(std::string &strIn, std::string &strOut);
    std::string PackMsgToAlarm(std::string &strDateTime, std::string &strIp, std::string &strName, std::string &strPhoneNum);

    int32_t Open(uint32_t baundrate, uint32_t databite, uint32_t stopbite, uint8_t parity); //打开串口
    void Close(void);                                                                                         //关闭串口

    int32_t readBuffer(uint8_t *read_buf, uint32_t buf_len, uint32_t delay_us, uint32_t &read_len);            //串口接受
    int32_t readLine(uint8_t *read_buf, uint32_t delay_us, uint32_t &read_len);
    int32_t writeBuffer(uint8_t *write_buf, uint32_t buf_len); //串口发送
    int32_t sendWheelSpd(float lspd, float rspd);               //发送轮子速度

protected:

private:
    int32_t Set(uint32_t baundrate, uint32_t flow_ctrl, uint32_t databits, uint32_t stopbits, uint8_t parity); //串口设置参数

    std::shared_ptr<spdlog::logger> ptrLogger;
    std::string strMsgSent;
    std::map<char, std::string> Utf16beConvertor;

    uint8_t ucReadLineBuf[READ_LINE_BUF_SIZE];
    uint32_t uiReadLineValidSize;
    uint32_t uiReadLineIdx;
};
unsigned char XorCode(unsigned char *buf, int len);
unsigned char XorCode(std::string buf, int len);

#endif
