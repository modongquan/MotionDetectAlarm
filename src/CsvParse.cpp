#include "CsvParse.h"
#include <iostream>

CsvParse::CsvParse()
{

}

CsvParse::~CsvParse()
{

}

void CsvParse::SetPtrLogger(std::shared_ptr<spdlog::logger> logger)
{
    ptrLogger = logger;
}

bool CsvParse::ParseCsvData(const char * ptrCsvPath)
{
    if(ptrCsvPath)
    {
        io::CSVReader<6> CsvParsed(ptrCsvPath);

        CsvParsed.read_header(io::ignore_missing_column, "DbIp", "DbPort", "DbUserName", "DbPasswd", "DbName", "DbTable");

        std::string DbIp, DbPort, DbUserName, DbPasswd, DbName, DbTable;

        if (CsvParsed.read_row(DbIp, DbPort, DbUserName, DbPasswd, DbName, DbTable) == false)
        {
            ptrLogger->error("read csv fail");
            ptrLogger->flush();
            return false;
        }

        if(DbIp.empty() == true || DbPort.empty() == true || DbUserName.empty() == true || DbPasswd.empty() == true ||
                DbName.empty() == true || DbTable.empty() == true)
        {
            ptrLogger->error("read empty data from cvs");
            ptrLogger->flush();
            return false;
        }


        strDbIp = DbIp;
        strDbPort = DbPort;
        strDbUserName = DbUserName;
        strDbPasswd = DbPasswd;
        strDbName = DbName;
        strDbTable = DbTable;
    }
    else
    {
        ptrLogger->error("cvs file path is null");
        ptrLogger->flush();
        return false;
    }

    return true;
}

void CsvParse::GetDbIp(std::string &DbIp)
{
    DbIp = strDbIp;
}

void CsvParse::GetDbPort(std::string &DbPort)
{
    DbPort = strDbPort;
}

void CsvParse::GetDbUserName(std::string &DbUserName)
{
    DbUserName = strDbUserName;
}

void CsvParse::GetDbPasswd(std::string &DbPasswd)
{
    DbPasswd = strDbPasswd;
}

void CsvParse::GetDbName(std::string &DbName)
{
    DbName = strDbName;
}

void CsvParse::GetDbTable(std::string &DbTable)
{
    DbTable = strDbTable;
}
