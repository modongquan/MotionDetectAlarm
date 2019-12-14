#ifndef DATA_BASE_H
#define DATA_BASE_H

#include "mysql/mysql.h"
#include <iostream>
#include <string.h>
#include <vector>
#include <map>
#include "global.h"

typedef struct
{
    MYSQL_FIELD_OFFSET IdxInDB;
    std::string InfoData;
}CAMERA_INFO_FROM_DB;

class DataBaseOperate
{
public:
    DataBaseOperate();
    ~DataBaseOperate();

    static void SetPtrLogger(std::shared_ptr<spdlog::logger> logger);

    static bool StrToUint(std::string &str, uint32_t &uint);
    static bool ConnectDataBase(const char *ptrIp, uint32_t port, const char *ptrUserName, const char *ptrPassword,
                                const char *ptrDBName, const char *ptrTableName);
    static uint64_t GetCameraNum(void);
    static bool GetCameraIp(uint64_t idx, std::string &strIp);
    static bool GetCameraPort(uint64_t idx, uint32_t &port);
    static bool GetCameraUserName(uint64_t idx, std::string &strName);
    static bool GetCameraPasswd(uint64_t idx, std::string &strPasswd);
    static bool GetCameraName(std::string &strIp, std::string &strName);
    static bool GetCameraWarnPhone(std::string &strIp, std::vector<std::string> &vecPhone);

private:
    static bool GetCameraInfoFromDB(MYSQL *ptrConnect, const char *ptrTable);
    static void ExtractPhoneNumber(std::string &strPhoneNum, std::vector<std::string> &vectPhoneNum);

    static const std::string CameraIp;
    static const std::string CameraPort;
    static const std::string CameraName;
    static const std::string UserName;
    static const std::string Passwd;
    static const std::string PhonNums;

    static std::map<std::string, MYSQL_FIELD_OFFSET> CameraInfoName;
    static std::vector<std::map<std::string, std::string>> CameraInfoFromDB;
    static std::map<std::string, std::vector<std::string>> IpWarnToPhoneFromDB;

    static std::shared_ptr<spdlog::logger> ptrLogger;
};

#endif
