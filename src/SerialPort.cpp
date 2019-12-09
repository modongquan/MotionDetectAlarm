#include "SerialPort.h"
#include "spdlog.h"
#include <iostream>

/*******************************************************************
* 名称：    SerialPort()
* 功能：    构造函数
* 入口参数: 无
* 出口参数: 无
*******************************************************************/
SerialPort::SerialPort() : strMsgSent("send_message(\"\",\"002000490050573057404E3A7684644450CF673A53D1751F79FB52A84FA66D4B62A58B66\")\r\n")
{
    isnormal = UART_FALSE;
    fd = UART_FALSE;

    memset(ucReadLineBuf, 0, READ_LINE_BUF_SIZE);
    uiReadLineValidSize = 0;
    uiReadLineIdx = 0;

    Utf16beConvertor['0'] = std::string("0030");
    Utf16beConvertor['1'] = std::string("0031");
    Utf16beConvertor['2'] = std::string("0032");
    Utf16beConvertor['3'] = std::string("0033");
    Utf16beConvertor['4'] = std::string("0034");
    Utf16beConvertor['5'] = std::string("0035");
    Utf16beConvertor['6'] = std::string("0036");
    Utf16beConvertor['7'] = std::string("0037");
    Utf16beConvertor['8'] = std::string("0038");
    Utf16beConvertor['9'] = std::string("0039");
    Utf16beConvertor[' '] = std::string("0020");
    Utf16beConvertor['-'] = std::string("002D");
    Utf16beConvertor['.'] = std::string("002E");
    Utf16beConvertor[':'] = std::string("003A");
}

SerialPort::~SerialPort()
{
    Close();
}

void SerialPort::SetPtrLogger(std::shared_ptr<spdlog::logger> logger)
{
    ptrLogger = logger;
}

void SerialPort::ConvertToUtf16Be(std::string &strIn, std::string &strOut)
{
    for(uint32_t i = 0;i < strIn.size();i ++)
    {
        if(0 == Utf16beConvertor.count(strIn.at(i)))
        {
            continue;
        }
        strOut.insert(strOut.size(), Utf16beConvertor[strIn.at(i)]);
    }
}

std::string SerialPort::PackMsgToAlarm(std::string &strDateTime, std::string &strIp, std::string &strPhoneNum)
{
    std::string strMsg;

    if(strIp.empty() || strPhoneNum.empty()) return strMsg;

    std::string strDataTimeUtf16be;
    std::string strIpUtf16be;

    ConvertToUtf16Be(strDateTime, strDataTimeUtf16be);
    ConvertToUtf16Be(strIp, strIpUtf16be);

    strMsg = strMsgSent;

    size_t pos;

    strMsg.insert(14, strPhoneNum);
    strMsg.insert(17 + 11, strDataTimeUtf16be);
    pos = 41 + 11 + strDataTimeUtf16be.size();
    strMsg.insert(pos, strIpUtf16be);

    return strMsg;
}

/*******************************************************************
* 名称：       Open
* 功能：       打开串口并返回串口设备文件描述
* 入口参数：   id     :串口号     
           speed :波特率
* 出口参数：        正确返回为1，错误返回为0
*******************************************************************/
int32_t SerialPort::Open(uint32_t port_id, uint32_t baundrate, uint32_t databite, uint32_t stopbite, uint8_t parity)
{
    const char *dev_t[] = {"/dev/ttyUSB0", "/dev/ttyUSB1", "/dev/ttyUSB2"};

    if (port_id >= sizeof(dev_t))
    {
        ptrLogger->error("serial port id input error");
        return (UART_FALSE);
    }

    fd = open(dev_t[port_id], O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (UART_FALSE == fd)
    {
        ptrLogger->error("can't open serial port : {}", strerror(errno));
        return (UART_FALSE);
    }

    //测试是否为终端设备  确认串口是否打开
    //    if (0 == isatty(STDIN_FILENO))
    //    {
    //        ptrLogger->error("standard input is not a terminal device");
    //        return (UART_FALSE);
    //    }

    //设置串口信息
    isnormal = Set(baundrate, 0, databite, stopbite, parity);

    return isnormal;
}

/*******************************************************************
* 名称：                UART0_Close
* 功能：                关闭串口并返回串口设备文件描述
* 入口参数：        fd    :文件描述符     port :串口号(ttyS0,ttyS1,ttyS2)
* 出口参数：        void
*******************************************************************/

void SerialPort::Close(void)
{
    close(fd);
}

/*******************************************************************
* 名称：                UART0_Set
* 功能：                设置串口数据位，停止位和效验位
* 入口参数：        fd        串口文件描述符
*                              speed     串口速度
*                              flow_ctrl   数据流控制
*                           databits   数据位   取值为 7 或者8
*                           stopbits   停止位   取值为 1 或者2
*                           parity     效验类型 取值为N,E,O,,S
*出口参数：          正确返回为1，错误返回为0
*******************************************************************/
int32_t SerialPort::Set(uint32_t baundrate, uint32_t flow_ctrl, uint32_t databits, uint32_t stopbits, uint8_t parity)
{
    uint32_t speed_arr[] = {B115200, B19200, B9600, B4800, B2400, B1200, B300};
    uint32_t name_arr[] = {115200, 19200, 9600, 4800, 2400, 1200, 300};

    struct termios options;

    /*tcgetattr(fd,&options)得到与fd指向对象的相关参数，并将它们保存于options,该函数还可以测试配置是否正确，该串口是否可用等。
     * 若调用成功，函数返回值为0，若调用失败，函数返回值为1.*/
    if (tcgetattr(fd, &options) != 0)
    {
        ptrLogger->error("tcgetattr error : {}", strerror(errno));
        return (UART_FALSE);
    }

    //设置串口输入波特率和输出波特率
    for (uint32_t i = 0; i < sizeof(speed_arr); i++)
    {
        if (baundrate == name_arr[i])
        {
            cfsetispeed(&options, speed_arr[i]);
            cfsetospeed(&options, speed_arr[i]);
        }
    }

    //修改控制模式，保证程序不会占用串口
    options.c_cflag |= CLOCAL;
    //修改控制模式，使得能够从串口中读取输入数据
    options.c_cflag |= CREAD;

    //设置数据流控制
    switch (flow_ctrl)
    {

    case 0: //不使用流控制
        options.c_cflag &= ~CRTSCTS;
        break;

    case 1: //使用硬件流控制
        options.c_cflag |= CRTSCTS;
        break;
    case 2: //使用软件流控制
        options.c_cflag |= IXON | IXOFF | IXANY;
        break;
    }
    //设置数据位
    //屏蔽其他标志位
    options.c_cflag &= ~CSIZE;
    switch (databits)
    {
    case 5:
        options.c_cflag |= CS5;
        break;
    case 6:
        options.c_cflag |= CS6;
        break;
    case 7:
        options.c_cflag |= CS7;
        break;
    case 8:
        options.c_cflag |= CS8;
        break;
    default:
        ptrLogger->error("Unsupported data size");
        return (UART_FALSE);
    }
    //设置校验位
    switch (parity)
    {
    case 'n':
    case 'N': //无奇偶校验位。
        options.c_cflag &= ~PARENB;
        options.c_iflag &= ~INPCK;
        break;
    case 'o':
    case 'O': //设置为奇校验
        options.c_cflag |= (PARODD | PARENB);
        options.c_iflag |= INPCK;
        break;
    case 'e':
    case 'E': //设置为偶校验
        options.c_cflag |= PARENB;
        options.c_cflag &= ~PARODD;
        options.c_iflag |= INPCK;
        break;
    case 's':
    case 'S': //设置为空格
        options.c_cflag &= ~PARENB;
        options.c_cflag &= ~CSTOPB;
        break;
    default:
        ptrLogger->error("Unsupported parity\n");
        return (UART_FALSE);
    }
    // 设置停止位
    switch (stopbits)
    {
    case 1:
        options.c_cflag &= ~CSTOPB;
        break;
    case 2:
        options.c_cflag |= CSTOPB;
        break;
    default:
        ptrLogger->error("Unsupported stop bits\n");
        return (UART_FALSE);
    }

    //修改输出模式，原始数据输出
    options.c_oflag &= ~OPOST;

    // 屏蔽 0x0d 这些特殊字符 解决这些特殊字符收不到的问题
    options.c_iflag &= ~(ICRNL);

    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); //我加的
    //options.c_lflag &= ~(ISIG | ICANON);

    //设置等待时间和最小接收字符
    options.c_cc[VTIME] = 1; /* 读取一个字符等待1*(1/10)s */
    options.c_cc[VMIN] = 1;  /* 读取字符的最少个数为1 */

    //如果发生数据溢出，接收数据，但是不再读取 刷新收到的数据但是不读
    tcflush(fd, TCIFLUSH);

    //激活配置 (将修改后的termios数据设置到串口中）
    if (tcsetattr(fd, TCSANOW, &options) != 0)
    {
        ptrLogger->error("com set error! : {}", strerror(errno));
        return (UART_FALSE);
    }

    return (UART_TRUE);
}

/*******************************************************************
* 名称：readBuffer
* 功能：接收串口数据
* 入口参数：
* fd:文件描述符
* rcv_buf:串口接收数据缓冲区
* buf_len:缓冲区的长度
* delay_us:the time for waitting to recieve data from serial port
* 出口参数：
* 正确返回为-1，错误返回为0
*******************************************************************/
int32_t SerialPort::readBuffer(uint8_t *read_buf, uint32_t buf_len, uint32_t delay_us, uint32_t &read_len)
{
    if(read_buf == nullptr)
    {
        return (UART_FALSE);
    }

    int32_t fs_sel;
    fd_set fs_read;

    struct timeval time;

    if (isnormal == UART_FALSE)
        return UART_FALSE;

    FD_ZERO(&fs_read);
    FD_SET(fd, &fs_read);

    time.tv_sec = 0;
    time.tv_usec = delay_us;

    fs_sel = select(fd + 1, &fs_read, nullptr, nullptr, &time);
    if (fs_sel < 0)
    {
        ptrLogger->error("select serial port error : {}", strerror(errno));
        ptrLogger->flush();
        return UART_FALSE;
    }
    else if(0 == fs_sel)
    {
        return UART_FALSE;
    }

    ssize_t ret_len;
    ret_len = read(fd, read_buf, buf_len);
    if(ret_len > 0)
    {
        read_len = static_cast<uint32_t>(ret_len);
        return UART_TRUE;
    }
    else
    {
        ptrLogger->error("read serial port error : {}", strerror((errno)));
        ptrLogger->flush();
        return UART_FALSE;
    }
}
#if 1
int32_t SerialPort::readLine(uint8_t *read_buf, uint32_t delay_us, uint32_t &read_len)
{
    if(read_buf == nullptr)
    {
        return (UART_FALSE);
    }

    bool is_need_return = false;

read_line_handle:
    if(uiReadLineValidSize > 0)
    {
        for(uint32_t i = uiReadLineIdx;i < uiReadLineValidSize;i ++)
        {
            if(uiReadLineIdx > 0)
            {
               ucReadLineBuf[i - uiReadLineIdx] = ucReadLineBuf[i];
            }

            if(0x0A != ucReadLineBuf[i])
            {
                if(i == uiReadLineValidSize - 1)
                {
                    if(uiReadLineIdx > 0)
                    {
                        uiReadLineValidSize -= uiReadLineIdx;
                        uiReadLineIdx = 0;
                    }
                    memset(ucReadLineBuf + uiReadLineValidSize, 0, READ_LINE_BUF_SIZE - uiReadLineValidSize);

                    if(false == is_need_return)
                    {
                        break;
                    }
                    else
                    {
                        return UART_FALSE;
                    }
                }
                continue;
            }

            read_len = i + 1 - uiReadLineIdx;
            memcpy(read_buf, ucReadLineBuf, read_len);
            uiReadLineIdx = i + 1;
            if(uiReadLineIdx >= uiReadLineValidSize)
            {
                uiReadLineValidSize = 0;
                uiReadLineIdx = 0;
            }
            return UART_TRUE;
        }
    }

    if(UART_FALSE == readBuffer(ucReadLineBuf + uiReadLineValidSize, READ_LINE_BUF_SIZE - uiReadLineValidSize, delay_us, read_len))
    {
        return UART_FALSE;
    }

    uiReadLineValidSize += read_len;

    is_need_return = true;

    goto read_line_handle;
}
#else
int32_t SerialPort::readLine(uint8_t *read_buf, uint32_t delay_us, uint32_t &read_len)
{
    if(nullptr == read_buf)
    {
        ptrLogger->error("{} : input null", __FUNCTION__);
        return UART_FALSE;
    }

    read_len = 0;
    uint32_t delay_count = 0;

    while(delay_count < 2)
    {
        uint8_t read_char;
        uint32_t read_char_len;

        if(UART_FALSE == readBuffer(&read_char, 1, 0, read_char_len))
        {
            usleep(delay_us);
            delay_count++;
            continue;
        }

        delay_count = 0;

        ucReadLineBuf[uiReadLineValidSize++] = read_char;
        if(0x0A == read_char || uiReadLineValidSize >= READ_LINE_BUF_SIZE)
        {
            memcpy(read_buf, ucReadLineBuf, uiReadLineValidSize);
            read_len = uiReadLineValidSize;

            uiReadLineValidSize = 0;

            return UART_TRUE;
        }
    }

    return UART_FALSE;
}
#endif
/*******************************************************************
* 名称：writeBuffer
* 功能：发送串口数据
* 入口参数：
* fd:文件描述符
* write_buf:串口发送数据缓冲区
* buf_len:缓冲区的长度
* 出口参数：
* 正确返回为-1，错误返回为0
*******************************************************************/
int32_t SerialPort::writeBuffer(uint8_t *write_buf, uint32_t buf_len)
{
    if (isnormal == UART_FALSE)
        return UART_FALSE;

    ssize_t len = 0;

    len = write(fd, write_buf, buf_len);
    if (len == buf_len)
    {
        return UART_TRUE;
    }
    else
    {
        ptrLogger->error("write serial port error : {}", strerror((errno)));
        ptrLogger->flush();
        tcflush(fd, TCOFLUSH);
        return UART_FALSE;
    }
}

/********************************************************************
* 名称：       XorCode
* 功能：       加密数据
* 入口参数：   *buf:待加密的数据
*              len: 带加密数据长度
*    
* 出口参数：    加密过后的值
*******************************************************************/
unsigned char XorCode(unsigned char *buf, int len)
{
    unsigned char ans = buf[0];
    for (int i = 1; i < len; i++)
        ans = ans ^ buf[i];
    return ans & 0X7F;
}

unsigned char XorCode(string buf, int len)
{
    int ans = buf[0];
    for (int i = 1; i < len; i++)
        ans ^= buf[i];
    return ans & 0x7f;
}

int32_t SerialPort::sendWheelSpd(float lspd, float rspd)
{
    unsigned char buff[20];
    int lspeed, rspeed;
    lspeed = (int)lspd;
    rspeed = (int)rspd;

    buff[0] = 'S';

    buff[1] = lspeed;
    buff[2] = lspeed >> 8;
    buff[3] = lspeed >> 16;
    buff[4] = lspeed >> 24;

    buff[5] = rspeed;
    buff[6] = rspeed >> 8;
    buff[7] = rspeed >> 16;
    buff[8] = rspeed >> 24;

    buff[9] = XorCode(buff + 1, 8);
    buff[10] = 'E';
    int len = writeBuffer(buff, 11);
    return len;
}
