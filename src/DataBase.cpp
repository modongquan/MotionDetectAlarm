#include "DataBase.h"

const std::string DataBaseOperate::CameraIp("ip");
const std::string DataBaseOperate::CameraPort("login_port");
const std::string DataBaseOperate::UserName("username");
const std::string DataBaseOperate::Passwd("password");
const std::string DataBaseOperate::PhonNums("phonenums");

std::map<std::string, MYSQL_FIELD_OFFSET> DataBaseOperate::CameraInfoName;
std::vector<std::map<std::string, std::string>> DataBaseOperate::CameraInfoFromDB;
std::map<std::string, std::vector<std::string>> DataBaseOperate::IpWarnToPhoneFromDB;

std::shared_ptr<spdlog::logger> DataBaseOperate::ptrLogger;

DataBaseOperate::DataBaseOperate()
{
}

DataBaseOperate::~DataBaseOperate()
{
}

void DataBaseOperate::SetPtrLogger(std::shared_ptr<spdlog::logger> logger)
{
    ptrLogger = logger;
}

bool DataBaseOperate::StrToUint(std::string &str, uint32_t &uint)
{
    uint = 0;

    for (size_t i = 0; i < str.size(); i++)
    {
        if (str.at(i) < '0' || str.at(i) > '9')
        {
            ptrLogger->error("string to uint error, string invalid");
            ptrLogger->flush();
            return false;
        }

        uint = uint * 10 + (uint32_t)str.at(i) - '0';
    }

    return true;
}

bool DataBaseOperate::ConnectDataBase(const char *ptrIp, uint32_t port, const char *ptrUserName, const char *ptrPassword,
                                      const char *ptrDBName, const char *ptrTableName)
{
    if (ptrIp == nullptr || ptrUserName == nullptr || ptrPassword == nullptr || ptrDBName == nullptr || ptrTableName == nullptr)
    {
        return false;
    }

    CameraInfoName[CameraIp] = 0xFFFFFFFF;
    CameraInfoName[CameraPort] = 0xFFFFFFFF;
    CameraInfoName[UserName] = 0xFFFFFFFF;
    CameraInfoName[Passwd] = 0xFFFFFFFF;
    CameraInfoName[PhonNums] = 0xFFFFFFFF;

    MYSQL *ptrMysql = mysql_init(nullptr);
    if (ptrMysql == nullptr)
    {
        ptrLogger->error("mysql init fail : {}", mysql_error(ptrMysql));
        ptrLogger->flush();
        return false;
    }

    unsigned int timeout = 7;
    if (mysql_options(ptrMysql, MYSQL_OPT_CONNECT_TIMEOUT, (const char *)&timeout))
    {
        ptrLogger->error("set mysql timeout fail : {}", mysql_error(ptrMysql));
        ptrLogger->flush();
        return false;
    }

    MYSQL *ptrConnect = mysql_real_connect(ptrMysql, ptrIp, ptrUserName, ptrPassword, ptrDBName, port, nullptr, 0);
    if (ptrConnect == nullptr)
    {
        ptrLogger->error("mysql connect fail : {}", mysql_error(ptrMysql));
        ptrLogger->flush();
        return false;
    }

    if (GetCameraInfoFromDB(ptrConnect, ptrTableName) == false)
    {
        return false;
    }

    return true;
}

uint64_t DataBaseOperate::GetCameraNum(void)
{
    return CameraInfoFromDB.size();
}

bool DataBaseOperate::GetCameraIp(uint64_t idx, std::string &strIp)
{
    if (idx >= CameraInfoFromDB.size())
    {
        return false;
    }

    strIp = CameraInfoFromDB[idx][CameraIp];
    if (strIp.empty() == true)
    {
        return false;
    }

    return true;
}

bool DataBaseOperate::GetCameraPort(uint64_t idx, uint32_t &port)
{
    if (idx >= CameraInfoFromDB.size())
    {
        return false;
    }

    return StrToUint(CameraInfoFromDB[idx][CameraPort], port);
}

bool DataBaseOperate::GetCameraUserName(uint64_t idx, std::string &strName)
{
    if (idx >= CameraInfoFromDB.size())
    {
        return false;
    }

    strName = CameraInfoFromDB[idx][UserName];

    return true;
}

bool DataBaseOperate::GetCameraPasswd(uint64_t idx, std::string &strPasswd)
{
    if (idx >= CameraInfoFromDB.size())
    {
        return false;
    }

    strPasswd = CameraInfoFromDB[idx][Passwd];

    return true;
}

bool DataBaseOperate::GetCameraWarnPhone(std::string &strIp, std::vector<std::string> &vecPhone)
{
    if (0 == IpWarnToPhoneFromDB.count(strIp))
    {
        ptrLogger->error("the warn ip {} is not exist", strIp.c_str());
        ptrLogger->flush();
        return false;
    }

    vecPhone = IpWarnToPhoneFromDB[strIp];

    return true;
}

bool DataBaseOperate::GetCameraInfoFromDB(MYSQL *ptrConnect, const char *ptrTable)
{
    if (ptrConnect == nullptr || ptrTable == nullptr)
        return false;

    char QueryCmd[64] = "select * from ";

    strcat(QueryCmd, ptrTable);
    if (mysql_real_query(ptrConnect, static_cast<const char *>(QueryCmd), strlen(QueryCmd)))
    {
        ptrLogger->error("{} fail : {}", QueryCmd, mysql_error(ptrConnect));
        ptrLogger->flush();
        return false;
    }

    MYSQL_RES *ptrTables = mysql_store_result(ptrConnect);
    if (ptrTables == nullptr)
    {
        ptrLogger->error("mysql_store_result fail : {}", mysql_error(ptrConnect));
        ptrLogger->flush();
        return false;
    }

    MYSQL_FIELD *ptrFields;
    while ((ptrFields = mysql_fetch_field(ptrTables)))
    {
        std::string key(ptrFields->name);

        if (CameraInfoName.count(key) > 0)
        {
            CameraInfoName[key] = mysql_field_tell(ptrTables) - 1;
        }
    }

    MYSQL_ROW Row;
    while ((Row = mysql_fetch_row(ptrTables)))
    {
        std::map<std::string, std::string> CameraInfo;
        bool IsNeedPush = true;

        for (auto iter = CameraInfoName.begin(); iter != CameraInfoName.end(); iter++)
        {
            if (iter->second == 0xFFFFFFFF)
            {
                ptrLogger->error("{} is missing in DB", iter->first.c_str());
                ptrLogger->flush();
                return false;
            }
            if (Row[iter->second] == nullptr)
            {
                IsNeedPush = false;
                break;
            }
            CameraInfo[iter->first] = std::string(Row[iter->second]);
        }

        if (IsNeedPush)
        {
            CameraInfoFromDB.push_back(CameraInfo);
        }
    }

    for (size_t i = 0; i < CameraInfoFromDB.size(); i++)
    {
        std::vector<std::string> vectPhone;

        ExtractPhoneNumber(CameraInfoFromDB[i][PhonNums], vectPhone);
        IpWarnToPhoneFromDB[CameraInfoFromDB[i][CameraIp]] = vectPhone;

        CameraInfoFromDB[i].erase(PhonNums);
    }

    return true;
}

void DataBaseOperate::ExtractPhoneNumber(std::string &strPhoneNum, std::vector<std::string> &vectPhoneNum)
{
    std::string PhoneNum;

    for (size_t i = 0; i < strPhoneNum.size(); i++)
    {
        if (strPhoneNum.at(i) != ',')
        {
            PhoneNum.push_back(strPhoneNum.at(i));
        }
        else
        {
            vectPhoneNum.push_back(PhoneNum);
            PhoneNum.clear();
        }
    }
    vectPhoneNum.push_back(PhoneNum);
    PhoneNum.clear();
}
