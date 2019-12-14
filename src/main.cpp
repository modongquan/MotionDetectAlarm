#include "HikDev.h"
#include "DataBase.h"
#include "CsvParse.h"
#include "SerialPort.h"
#include "global.h"
#include <signal.h>

bool ServiceRun = true;

void ProcessQuit(int pid)
{
    if (SIGTERM == pid)
    {
        ServiceRun = false;
    }
}

int CreatePidFile(char *pMsg, uint32_t u32MsgSize)
{
    int32_t s32ServicePid = getpid();

    if (access("/var/run/mdalarm", F_OK) < 0)
    {
        if (mkdir("/var/run/mdalarm", S_IRWXU | S_IRWXG | S_IRWXO) < 0)
        {
            snprintf(pMsg, u32MsgSize, "mkdir /var/run/mdalarm fail : %s\n", strerror(errno));
            return -1;
        }
    }

    int32_t s32PidFd = open("/var/run/mdalarm/mdalarm.pid", O_CREAT | O_RDWR | O_NONBLOCK);
    if (s32PidFd < 0)
    {
        snprintf(pMsg, u32MsgSize, "open /var/run/mdalarm/mdalarm pid file fail : %s\n", strerror(errno));
        return -1;
    }
    if (ftruncate(s32PidFd, 0) < 0)
    {
        snprintf(pMsg, u32MsgSize, "clear /var/run/mdalarm/mdalarm pid file fail : %s\n", strerror(errno));
        return -1;
    }

    char u8PidBuf[64];
    snprintf(u8PidBuf, 64, "%d\n", s32ServicePid);
    size_t u32WrSize = strlen(u8PidBuf);
    if (write(s32PidFd, u8PidBuf, u32WrSize) < ssize_t(u32WrSize))
    {
        snprintf(pMsg, u32MsgSize, "write pid to /var/run/mdalarm/mdalarm file fail : %s\n", strerror(errno));
        return -1;
    }
    close(s32PidFd);

    return 0;
}

int main(int argc, char *argv[])
{
    printf("argc = %d, argv = %p \n", argc, argv);
    signal(SIGTERM, ProcessQuit);

    auto HikDevLogger = spdlog::basic_logger_mt("HikDevLogger", "/var/log/mdalarm/mdalarm.txt");
    HikDevLogger->info("motion detect service start");

    char sMainBuf[256];

    if (CreatePidFile(sMainBuf, 256) < 0)
    {
        HikDevLogger->error("{}", sMainBuf);
        ServiceRun = false;
    }

    //    while(ServiceRun)
    //    {
    //        sleep(1);
    //    }
    //    remove("/var/run/mdalarm/mdalarm.pid");
    //    return 0;

    CsvParse DbCfg;
    DbCfg.SetPtrLogger(HikDevLogger);
    HikDevOperate::SetPtrLogger(HikDevLogger);
    DataBaseOperate::SetPtrLogger(HikDevLogger);

    if (false == DbCfg.ParseCsvData("/usr/local/mdalarm/DbCfg.csv"))
    {
        ServiceRun = false;
    }

    std::string DbIp, DbPort, DbUserName, DbPasswd, DbName, DbTable;
    DbCfg.GetDbIp(DbIp);
    DbCfg.GetDbPort(DbPort);
    DbCfg.GetDbUserName(DbUserName);
    DbCfg.GetDbPasswd(DbPasswd);
    DbCfg.GetDbName(DbName);
    DbCfg.GetDbTable(DbTable);
    uint32_t port;
    if (DataBaseOperate::StrToUint(DbPort, port) == false)
    {
        HikDevLogger->error("get invalid port from DbCfg.csv");
        ServiceRun = false;
    }

    bool ret = DataBaseOperate::ConnectDataBase(static_cast<const char *>(DbIp.c_str()), port,
                                                static_cast<const char *>(DbUserName.c_str()),
                                                static_cast<const char *>(DbPasswd.c_str()),
                                                static_cast<const char *>(DbName.c_str()),
                                                static_cast<const char *>(DbTable.c_str()));
    if (ret == false)
    {
        ServiceRun = false;
    }

    if (HikDevOperate::InitialSDK() == false)
    {
        ServiceRun = false;
    }

    uint64_t CameraNum = DataBaseOperate::GetCameraNum();
    for (uint64_t i = 0; i < CameraNum; i++)
    {
        HikDevHandl HikDev;

        if (DataBaseOperate::GetCameraIp(i, HikDev.sDevIp) == false)
        {
            HikDevLogger->error("get camera[{0:d}] ip from database fail", i);
            continue;
        }

        uint32_t CameraPort;
        if (DataBaseOperate::GetCameraPort(i, CameraPort) == false)
        {
            HikDevLogger->error("get camera[{0:d}] port from database fail", i);
            continue;
        }
        HikDev.wDevPort = static_cast<uint16_t>(CameraPort);

        if (DataBaseOperate::GetCameraUserName(i, HikDev.sDevUserName) == false)
        {
            HikDevLogger->error("get camera[{0:d}] user name from database fail", i);
            continue;
        }

        if (DataBaseOperate::GetCameraPasswd(i, HikDev.sDevPassword) == false)
        {
            HikDevLogger->error("get camera[{0:d}] password from database fail", i);
            continue;
        }

        HikDev.lUserId = HikDevOperate::HikDevLogin(HikDev);
        if (HikDev.lUserId < 0)
        {
            HikDevLogger->info("camera[{}] login fail", HikDev.sDevIp.c_str());
            continue;
        }
        else
        {
            HikDevLogger->info("camera[{}] login successful", HikDev.sDevIp.c_str());
        }
        HikDevLogger->flush();

        HikDevOperate::HikDevStartAlarm(HikDev.lUserId);
    }

    SerialPort MsgModuleCom;
    MsgModuleCom.SetPtrLogger(HikDevLogger);
    if (MsgModuleCom.Open(115200, 8, 1, 'N') < 0)
    {
        ServiceRun = false;
    }

#if 0
    uint8_t MsgBuf[READ_LINE_BUF_SIZE];
    uint32_t ReadSize;
#endif

    while (ServiceRun)
    {
#if 0
        if(UART_FALSE == MsgModuleCom.readLine(MsgBuf, 0, ReadSize))
        {
            usleep(10000);
            continue;
        }
        if(ReadSize < READ_LINE_BUF_SIZE)
        {
            MsgBuf[ReadSize] = '\0';
        }
        else
        {
            MsgBuf[READ_LINE_BUF_SIZE - 1] = '\0';
        }
        fprintf(stdout, "%s", MsgBuf);
        fflush(stdout);

        continue;
#endif

        uint32_t uiAlarmNum = HikDevOperate::HikDevGetAlarmNum();
        if (0 == uiAlarmNum)
        {
            sleep(1);
            continue;
        }

        for (uint32_t i = 0; i < uiAlarmNum; i++)
        {
            uint32_t uiYear, uiMon, uiDay, uiHour, uiMinute, uiSecond;
            std::string strDataTime;
            char sDataTime[128];
            HikDevOperate::HikDevGetAlarmDateTime(i, uiYear, uiMon, uiDay, uiHour, uiMinute, uiSecond);
            snprintf(sDataTime, 128, "%u-%u-%u %u:%u:%u", uiYear, uiMon, uiDay, uiHour, uiMinute, uiSecond);
            strDataTime.insert(0, sDataTime);

            std::string strAlarmIp;
            HikDevOperate::HikDevGetAlarmIp(i, strAlarmIp);
            if (strAlarmIp.empty())
            {
                HikDevLogger->error("get alarm camera ip null");
                HikDevLogger->flush();
                continue;
            }

            std::string strAlarmDevName;
            if (!DataBaseOperate::GetCameraName(strAlarmIp, strAlarmDevName))
            {
                HikDevLogger->error("get alarm camera name null");
                HikDevLogger->flush();
                continue;
            }
            // std::cout << "alarm camera name : " << strAlarmDevName <<
            //         " name size : " << strAlarmDevName.size() << std::endl;
            // for (uint32_t i = 0; i < strAlarmDevName.size(); i++)
            // {
            //     printf("%x ", strAlarmDevName.at(i));
            // }
            // printf("\n");

            std::vector<std::string> vstrPhone;

            if (false == DataBaseOperate::GetCameraWarnPhone(strAlarmIp, vstrPhone))
            {
                return -1;
            }

            for (uint32_t j = 0; j < vstrPhone.size(); j++)
            {
                std::string strMsgSent = MsgModuleCom.PackMsgToAlarm(strDataTime, strAlarmIp, strAlarmDevName, vstrPhone[j]);
                std::cout << strMsgSent << " size = " << strMsgSent.size() << std::endl;

                uint8_t BufSent[512];
                uint32_t size;
                for (size = 0; size < strMsgSent.size(); size++)
                {
                    BufSent[size] = static_cast<uint8_t>(strMsgSent[size]);
                }
                MsgModuleCom.writeBuffer(BufSent, size);

                std::cout << strAlarmIp << " alarm to " << vstrPhone[j] << std::endl;
            }
        }

        HikDevOperate::HikDevSetAlarmIdxOffset(uiAlarmNum);

        usleep(10000);
    }

    remove("/var/run/mdalarm/mdalarm.pid");
    MsgModuleCom.Close();
    HikDevOperate::DeinitialSDK();

    return 0;
}
